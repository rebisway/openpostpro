// DIAGNOSTIC TEST - checks the "is the drawing ready for machining" diagnostic (cad/diagnostic.cpp) on the real shape classes.
//
// What for:
//   A contour which does not close, a shape drawn twice or a shape of length zero give a toolpath which is wrong, machined
//   twice or impossible. The diagnostic finds them before the toolpath is made. The risk is in both directions: a problem
//   which is missed (a hole in the contour of a cut), and a false alarm on a good drawing (it would not be trusted).
//
// What is checked: a closed contour of lines (nothing to report); the same contour with a side missing (the two free ends);
//   a line drawn twice, a circle drawn twice, a line drawn in the other direction (the second one is reported, once, and it does
//   not give free ends); a line and a polyline of length zero; a closed polyline and an open one; an arc which joins
//   two lines; shapes of a hidden layer (ignored); the tolerance (a gap smaller than it is not a free end).
//
// Build: tests/cpp/build_diagnostic.bat   Run: tests/cpp/test_diagnostic.exe   (exit code 0 = all checks pass)
#include <diagnostic.h>
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <polyline.h>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

// headless renderer: buffers are real objects but draw nothing
struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

static Line* line(Renderer* r, float x1, float y1, float x2, float y2)
{
	Line* l = new Line(r);
	l->set(glm::vec2(x1, y1), glm::vec2(x2, y2));
	return l;
}
static Circle* circle(Renderer* r, glm::vec2 center, float radius)
{
	Circle* c = new Circle(r);
	c->center(center);
	c->radius(radius);
	return c;
}
static Polyline* polyline(Renderer* r, std::vector<glm::vec2> points)
{
	Polyline* p = new Polyline(r);
	for (glm::vec2 v : points)
		p->point(v);
	p->done(true);
	return p;
}

static int count(const std::vector<cad_diag::Issue>& issues, cad_diag::IssueType type)
{
	int n = 0;
	for (auto& i : issues)
		if (i.type == type)
			n++;
	return n;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);
	using cad_diag::IssueType;

	std::puts("a closed contour of lines");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.add(line(&renderer, 10, 0, 10, 10));
		layer.add(line(&renderer, 10, 10, 0, 10));
		layer.add(line(&renderer, 0, 10, 0, 0));
		CHECK(cad_diag::check(layers).empty(), "nothing to report on a closed square");

		// the same square with a gap: a side is missing
		Layer open(&renderer);
		std::vector<Layer*> open_layers = { &open };
		open.add(line(&renderer, 0, 0, 10, 0));
		open.add(line(&renderer, 10, 0, 10, 10));
		open.add(line(&renderer, 10, 10, 0, 10));
		auto issues = cad_diag::check(open_layers);
		CHECK(issues.size() == 2, "a side missing: two free ends");
		CHECK(count(issues, IssueType::FreeEnd) == 2, "they are free ends");
		for (auto& i : issues)
			CHECK(glm::distance(i.point, glm::vec2(0, 10)) < 0.01f || glm::distance(i.point, glm::vec2(0, 0)) < 0.01f, "at the two ends of the gap");
	}

	std::puts("tolerance");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.add(line(&renderer, 10.005f, 0, 10, 10));
		layer.add(line(&renderer, 10, 10, 0, 10));
		layer.add(line(&renderer, 0, 10, 0, 0));
		CHECK(cad_diag::check(layers, 0.01f).empty(), "a gap under the tolerance is not a free end");
		CHECK(count(cad_diag::check(layers, 0.001f), IssueType::FreeEnd) == 2, "a smaller tolerance finds the gap");
	}

	std::puts("duplicates");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.add(line(&renderer, 10, 0, 10, 10));
		layer.add(line(&renderer, 10, 10, 0, 10));
		layer.add(line(&renderer, 0, 10, 0, 0));
		Line* twice = line(&renderer, 10, 0, 0, 0);	// the first side again, in the other direction
		layer.add(twice);
		auto issues = cad_diag::check(layers);
		CHECK(issues.size() == 1, "one problem: the side drawn twice");
		CHECK(count(issues, IssueType::Duplicate) == 1, "it is a duplicate");
		CHECK(issues.size() == 1 && issues[0].shape == twice, "the second one is the one reported");

		Layer circles(&renderer);
		std::vector<Layer*> circle_layers = { &circles };
		circles.add(circle(&renderer, glm::vec2(5, 5), 3));
		circles.add(circle(&renderer, glm::vec2(5, 5), 3));
		circles.add(circle(&renderer, glm::vec2(5, 5), 4));
		issues = cad_diag::check(circle_layers);
		CHECK(issues.size() == 1 && issues[0].type == IssueType::Duplicate, "a circle drawn twice, not the one with another radius");

		// three times the same line: two to delete
		Layer three(&renderer);
		std::vector<Layer*> three_layers = { &three };
		three.add(line(&renderer, 0, 0, 5, 5));
		three.add(line(&renderer, 0, 0, 5, 5));
		three.add(line(&renderer, 5, 5, 0, 0));
		CHECK(count(cad_diag::check(three_layers), IssueType::Duplicate) == 2, "a line three times: two duplicates");
	}

	std::puts("length zero");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 3, 3, 3, 3));
		layer.add(polyline(&renderer, { glm::vec2(1, 1), glm::vec2(1, 1), glm::vec2(1, 1) }));
		auto issues = cad_diag::check(layers);
		CHECK(count(issues, IssueType::ZeroLength) == 2, "a line and a polyline of length zero");
		CHECK(count(issues, IssueType::FreeEnd) == 0, "a shape of length zero has no free end");
	}

	std::puts("polylines and arcs");
	{
		Layer closed(&renderer);
		std::vector<Layer*> closed_layers = { &closed };
		closed.add(polyline(&renderer, { glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(10, 10), glm::vec2(0, 0) }));
		CHECK(cad_diag::check(closed_layers).empty(), "a closed polyline has no free end");

		Layer open(&renderer);
		std::vector<Layer*> open_layers = { &open };
		open.add(polyline(&renderer, { glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(10, 10) }));
		CHECK(count(cad_diag::check(open_layers), IssueType::FreeEnd) == 2, "an open polyline has two free ends");

		// a polyline closed by a line
		open.add(line(&renderer, 10, 10, 0, 0));
		CHECK(cad_diag::check(open_layers).empty(), "closed by a line: nothing to report");

		// a half circle closed by a line: the ends of the arc touch the ends of the line
		Layer half(&renderer);
		std::vector<Layer*> half_layers = { &half };
		Arc* a = new Arc(&renderer);
		a->set(glm::vec2(5, 0), glm::vec2(0, 0), glm::vec2(-5, 0), false);
		half.add(a);
		half.add(line(&renderer, -5, 0, 5, 0));
		CHECK(cad_diag::check(half_layers).empty(), "an arc and a line which close a half disc");
	}

	std::puts("hidden layer");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.visible(false);
		CHECK(cad_diag::check(layers).empty(), "the shapes of a hidden layer are ignored");
	}

	std::puts("select the contour");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		// a square, a branch which leaves one corner (a line then a polyline), a second square apart, a circle
		Line* a = line(&renderer, 0, 0, 10, 0);
		Line* b = line(&renderer, 10, 0, 10, 10);
		Line* c = line(&renderer, 10, 10, 0, 10);
		Line* d = line(&renderer, 0, 10, 0, 0);
		Line* branch = line(&renderer, 10, 10, 20, 20);
		Polyline* tail = polyline(&renderer, { glm::vec2(20, 20), glm::vec2(30, 20), glm::vec2(30, 30) });
		Line* apart = line(&renderer, 100, 100, 110, 100);
		Circle* disc = circle(&renderer, glm::vec2(0, 0), 5);
		for (Shape* s : std::vector<Shape*>{ a, b, c, d, branch, tail, apart, disc })
			layer.add(s);

		auto result = cad_diag::contour({ a }, layers);
		CHECK(result.size() == 6, "from one side: the square, the branch and its tail");
		CHECK(!result.empty() && result[0] == a, "the selection comes first");
		auto has = [&](Shape* s) { return std::find(result.begin(), result.end(), s) != result.end(); };
		CHECK(has(b) && has(c) && has(d) && has(branch) && has(tail), "all the connected shapes are there");
		CHECK(!has(apart) && !has(disc), "the shapes which are not connected are not");

		// from the end of the tail: the same shapes
		CHECK(cad_diag::contour({ tail }, layers).size() == 6, "the same contour from another shape");

		// a shape without ends is only returned when selected
		result = cad_diag::contour({ disc }, layers);
		CHECK(result.size() == 1 && result[0] == disc, "a circle alone");

		// the shapes are not modified
		CHECK(glm::distance(a->p1(), glm::vec2(0, 0)) < 0.001f && glm::distance(c->p1(), glm::vec2(10, 10)) < 0.001f, "the shapes are left as they are");

		// two selected shapes of different contours: both contours
		result = cad_diag::contour({ a, apart }, layers);
		CHECK(result.size() == 7, "two selected contours");
	}

	std::puts("summary");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.add(line(&renderer, 4, 4, 4, 4));
		auto issues = cad_diag::check(layers);
		std::string text = cad_diag::summary(issues, "zero", "dup", "free");
		CHECK(text == "zero: 1, dup: 1, free: 2", "the summary counts each type");
		CHECK(cad_diag::summary({}, "a", "b", "c").empty(), "no problem, no text");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
