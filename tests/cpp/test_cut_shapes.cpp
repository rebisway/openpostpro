// CUT TESTS - checks which shapes the cut tool cuts, and where, on the real shape classes (cad/cut_shape.cpp).
//
// What for:
//   The cut tool splits the shapes under a click at the clicked point. The risky part is choosing the shapes and the
//   point: the point must lie on the shape even when the magnet puts the click off it (grid), a click on a crossing
//   must cut both shapes at the crossing, and a click on an end of a shape must not cut it (it would create a
//   zero-length piece). The splitting itself is done by cad::split(), which is not covered here.
//
// What is checked: a line cut in the middle; a click off the line (projected onto it); a click on an end (nothing);
//   a crossing of two lines, of a line and an arc; a point on an arc, and on a circle; a hidden layer is ignored;
//   nothing under the click.
//
// Build: tests/cpp/build_cut_shapes.bat   Run: tests/cpp/test_cut_shapes.exe   (exit code 0 = all checks pass)
#include <cut_shape.h>
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
static void check_point(int line, glm::vec2 got, float ex, float ey, const char* msg)
{
	g_run++;
	if (glm::distance(got, glm::vec2(ex, ey)) > 0.02f)
	{
		g_fail++;
		std::printf("  FAIL line %d: %s (got %g,%g expected %g,%g)\n", line, msg, got.x, got.y, ex, ey);
	}
}
#define CHECK_POINT(got, ex, ey, msg) check_point(__LINE__, got, ex, ey, msg)

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
static Arc* arc(Renderer* r, glm::vec2 center, glm::vec2 start, glm::vec2 stop, bool cw)
{
	Arc* a = new Arc(r);
	a->set(start, center, stop, cw);
	return a;
}
static Circle* circle(Renderer* r, glm::vec2 center, float radius)
{
	Circle* c = new Circle(r);
	c->center(center);
	c->radius(radius);
	return c;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::puts("one line");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		Line* l = line(&renderer, 0, 0, 10, 0);
		layer.add(l);

		auto cuts = cad_cut::find(layers, glm::vec2(4, 0), glm::vec2(4, 0));
		CHECK(cuts.size() == 1 && cuts[0].shape == l, "a click on the line cuts it");
		if (cuts.size() == 1)
			CHECK_POINT(cuts[0].point, 4, 0, "cut at the click");

		// the magnet (grid) moved the click off the line: the cut is on the line
		cuts = cad_cut::find(layers, glm::vec2(4.1f, 0.05f), glm::vec2(4, 3));
		CHECK(cuts.size() == 1, "a click moved off the line by the magnet still cuts it");
		if (cuts.size() == 1)
			CHECK_POINT(cuts[0].point, 4, 0, "the cut point is projected onto the line");

		cuts = cad_cut::find(layers, glm::vec2(10, 0), glm::vec2(10, 0));
		CHECK(cuts.empty(), "a click on an end of the line cuts nothing");

		cuts = cad_cut::find(layers, glm::vec2(50, 50), glm::vec2(50, 50));
		CHECK(cuts.empty(), "nothing under the click");
	}

	std::puts("crossing");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(line(&renderer, 0, 0, 10, 10));
		layer.add(line(&renderer, 0, 10, 10, 0));
		auto cuts = cad_cut::find(layers, glm::vec2(5.1f, 5.1f), glm::vec2(5.1f, 5.1f));
		CHECK(cuts.size() == 2, "both lines are cut");
		for (auto& c : cuts)
			CHECK_POINT(c.point, 5, 5, "both are cut at the crossing");

		// a magnet point on one line only, near the crossing: the crossing is still what is aimed at
		cuts = cad_cut::find(layers, glm::vec2(5.05f, 5.0f), glm::vec2(5.5f, 5.5f));
		CHECK(cuts.size() == 2, "the crossing is found from a point near it");
	}

	std::puts("line and arc");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		layer.add(arc(&renderer, glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(0, 10), false));
		layer.add(line(&renderer, 0, 0, 20, 20));
		float r = 10.0f * std::sqrt(0.5f);
		auto cuts = cad_cut::find(layers, glm::vec2(r, r), glm::vec2(r, r));
		CHECK(cuts.size() == 2, "line and arc are cut");
		for (auto& c : cuts)
			CHECK_POINT(c.point, r, r, "both are cut at the crossing");
	}

	std::puts("arc and circle alone");
	{
		Layer layer(&renderer);
		std::vector<Layer*> layers = { &layer };
		Arc* a = arc(&renderer, glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(0, 10), false);
		layer.add(a);
		float r = 10.0f * std::sqrt(0.5f);
		auto cuts = cad_cut::find(layers, glm::vec2(r, r), glm::vec2(r, r));
		CHECK(cuts.size() == 1 && cuts[0].shape == a, "a click on the arc cuts it");
		if (cuts.size() == 1)
			CHECK_POINT(cuts[0].point, r, r, "cut on the arc");
		cuts = cad_cut::find(layers, glm::vec2(10, 0), glm::vec2(10, 0));
		CHECK(cuts.empty(), "the start of the arc can not be cut");

		Layer second(&renderer);
		std::vector<Layer*> layers2 = { &second };
		Circle* c = circle(&renderer, glm::vec2(0, 0), 10);
		second.add(c);
		cuts = cad_cut::find(layers2, glm::vec2(0, 10), glm::vec2(0, 10));
		CHECK(cuts.size() == 1 && cuts[0].shape == c, "a click on a circle");
		if (cuts.size() == 1)
			CHECK_POINT(cuts[0].point, 0, 10, "cut on the circle");
	}

	std::puts("polyline pieces");
	{
		std::vector<glm::vec2> open = { {0, 0}, {10, 0}, {10, 10} };
		auto r = cad_cut::split_points(open, glm::vec2(5, 0));
		CHECK(r.size() == 2, "an open polyline cut inside a segment gives two pieces");
		if (r.size() == 2)
		{
			CHECK(r[0].size() == 2 && r[1].size() == 3, "the cut point ends the first piece and starts the second");
			CHECK_POINT(r[0].back(), 5, 0, "first piece ends at the cut");
			CHECK_POINT(r[1].front(), 5, 0, "second piece starts at the cut");
		}

		// the common case with the magnet: the click is on a vertex, shared by two segments
		r = cad_cut::split_points(open, glm::vec2(10, 0));
		CHECK(r.size() == 2, "a cut on a vertex gives two pieces");
		if (r.size() == 2)
			CHECK(r[0].size() == 2 && r[1].size() == 2, "no duplicated vertex");

		CHECK(cad_cut::split_points(open, glm::vec2(0, 0)).empty(), "the start of an open polyline can not be cut");
		CHECK(cad_cut::split_points(open, glm::vec2(10, 10)).empty(), "the end of an open polyline can not be cut");
		CHECK(cad_cut::split_points(open, glm::vec2(5, 5)).empty(), "a point which is not on the polyline cuts nothing");

		std::vector<glm::vec2> square = { {0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0} };
		r = cad_cut::split_points(square, glm::vec2(5, 0));
		CHECK(r.size() == 1 && r[0].size() == 6, "a closed polyline is opened at the cut");
		if (r.size() == 1 && r[0].size() == 6)
		{
			CHECK_POINT(r[0].front(), 5, 0, "the opened polyline starts at the cut");
			CHECK_POINT(r[0].back(), 5, 0, "the opened polyline ends at the cut");
		}
		r = cad_cut::split_points(square, glm::vec2(10, 0));
		CHECK(r.size() == 1 && r[0].size() == 5, "a closed polyline is opened at a vertex");
		CHECK(cad_cut::split_points(square, glm::vec2(0, 0)).empty(), "a closed polyline is already open at its start");
	}

	std::puts("hidden layer");
	{
		Layer layer(&renderer);
		layer.add(line(&renderer, 0, 0, 10, 0));
		layer.visible(false);
		std::vector<Layer*> layers = { &layer };
		CHECK(cad_cut::find(layers, glm::vec2(4, 0), glm::vec2(4, 0)).empty(), "a hidden layer is ignored");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
