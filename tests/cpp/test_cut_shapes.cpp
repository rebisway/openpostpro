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

	std::puts("shapes cut");
	{
		// line
		Line* l = line(&renderer, 0, 0, 10, 0);
		auto p = cad_cut::split_shape(l, glm::vec2(4, 0));
		CHECK(p.size() == 2, "a line gives two lines");
		if (p.size() == 2)
		{
			CHECK(p[0]->type() == GraphicType::Line && p[1]->type() == GraphicType::Line, "both are lines");
			CHECK_POINT(((Line*)p[0])->p2(), 4, 0, "first line ends at the cut");
			CHECK_POINT(((Line*)p[1])->p1(), 4, 0, "second line starts at the cut");
			CHECK_POINT(((Line*)p[1])->p2(), 10, 0, "second line keeps the end");
		}
		CHECK(cad_cut::split_shape(l, glm::vec2(10, 0)).empty(), "no cut on an end of a line");

		// arc, both directions: the pieces keep the center and the direction
		for (bool cw : { false, true })
		{
			Arc* a = cw ? arc(&renderer, glm::vec2(0, 0), glm::vec2(0, 10), glm::vec2(10, 0), true) : arc(&renderer, glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(0, 10), false);
			float r = 10.0f * std::sqrt(0.5f);
			auto pieces = cad_cut::split_shape(a, glm::vec2(r, r));
			CHECK(pieces.size() == 2, "an arc gives two arcs");
			if (pieces.size() == 2)
			{
				Arc* a1 = (Arc*)pieces[0];
				Arc* a2 = (Arc*)pieces[1];
				CHECK(a1->cw() == cw && a2->cw() == cw, "the direction is kept");
				CHECK_POINT(a1->start(), a->start().x, a->start().y, "first piece starts at the start of the arc");
				CHECK_POINT(a1->stop(), r, r, "first piece ends at the cut");
				CHECK_POINT(a2->start(), r, r, "second piece starts at the cut");
				CHECK_POINT(a2->stop(), a->stop().x, a->stop().y, "second piece ends at the stop of the arc");
			}
			CHECK(cad_cut::split_shape(a, a->stop()).empty(), "no cut on the end of an arc");
		}

		// circle: one click opens it, a second click cuts the arc which goes all the way around
		Circle* c = circle(&renderer, glm::vec2(0, 0), 10);
		auto opened = cad_cut::split_shape(c, glm::vec2(0, 10));
		CHECK(opened.size() == 1 && opened[0]->type() == GraphicType::Arc, "a circle becomes one arc");
		if (opened.size() == 1)
		{
			Arc* full = (Arc*)opened[0];
			CHECK_POINT(full->start(), 0, 10, "it starts at the cut");
			CHECK_POINT(full->stop(), 0, 10, "it ends at the cut");
			CHECK(!full->cw(), "counter-clockwise");
			CHECK(cad_cut::split_shape(full, glm::vec2(0, 10)).empty(), "no second cut on the open point");

			// second cut, found as the tool does: the point of the full arc nearest to the click
			Layer layer(&renderer);
			layer.add(full);
			std::vector<Layer*> layers = { &layer };
			auto cuts = cad_cut::find(layers, glm::vec2(10, 0), glm::vec2(10, 0));
			CHECK(cuts.size() == 1, "the open circle can be cut again");
			if (cuts.size() == 1)
			{
				CHECK_POINT(cuts[0].point, 10, 0, "at the click");
				auto halves = cad_cut::split_shape(cuts[0].shape, cuts[0].point);
				CHECK(halves.size() == 2, "two arcs");
				if (halves.size() == 2)
				{
					CHECK_POINT(((Arc*)halves[0])->start(), 0, 10, "first arc starts at the first cut");
					CHECK_POINT(((Arc*)halves[0])->stop(), 10, 0, "first arc ends at the second cut");
					CHECK_POINT(((Arc*)halves[1])->start(), 10, 0, "second arc starts at the second cut");
					CHECK_POINT(((Arc*)halves[1])->stop(), 0, 10, "second arc ends at the first cut");
				}
			}
		}
	}

	std::puts("locked layer");
	{
		Layer locked_layer(&renderer);
		locked_layer.add(line(&renderer, 0, 0, 10, 10));
		locked_layer.locked(true);
		Layer free_layer(&renderer);
		Line* free = line(&renderer, 0, 10, 10, 0);
		free_layer.add(free);
		std::vector<Layer*> layers = { &locked_layer, &free_layer };

		// the locked line is never cut
		Layer alone(&renderer);
		alone.add(line(&renderer, 0, 0, 100, 0));
		alone.locked(true);
		std::vector<Layer*> alone_layers = { &alone };
		CHECK(cad_cut::find(alone_layers, glm::vec2(40, 0), glm::vec2(40, 0)).empty(), "a locked line is not cut");

		// but it gives the crossing: the free line is cut there
		auto cuts = cad_cut::find(layers, glm::vec2(5.1f, 5.1f), glm::vec2(5.1f, 5.1f));
		CHECK(cuts.size() == 1 && cuts[0].shape == free, "only the free line is cut at the crossing");
		if (cuts.size() == 1)
			CHECK_POINT(cuts[0].point, 5, 5, "at the crossing with the locked line");
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
