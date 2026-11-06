// EMPTY INPUTS TEST - the geometry functions must accept empty or too short lists (no crash, no endless loop).
//
// What for:
//   A loop written "for (int i = 0; i < list.size() - 1; i++)" is wrong for an empty list: size() is an unsigned number,
//   so size() - 1 becomes a huge value and the loop reads far outside the list (this is what crashed the start of the
//   application, with a language line which was empty). About thirty loops of this kind were rewritten as
//   "i + 1 < list.size()", and the allocations "vector(size() - 1)" for an empty list too. These tests call the
//   functions concerned with 0, 1 and 2 points and check that they return, with nothing to compute.
//
// What is checked: bezier and bspline, the intersections of a segment, a circle, an arc and a polyline with an empty
//   polyline, two empty polylines, Curve::reverse and split_at_intersections on an empty curve.
//
// Build: tests/cpp/build_empty_inputs.bat   Run: tests/cpp/test_empty_inputs.exe   (exit code 0 = all checks pass)
#include <geometry.h>
#include <curve.h>
#include <cstdio>
#include <vector>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	std::vector<glm::vec2> none, one = { {1, 2} }, two = { {0, 0}, {1, 1} };
	std::vector<glm::vec2> hits;

	std::puts("curves");
	CHECK(geometry::bezier(none).empty(), "bezier of nothing");
	CHECK(geometry::bezier(one).empty(), "bezier of one point");
	CHECK(geometry::bezier(two).empty(), "bezier of two points");
	CHECK(geometry::bspline(none).empty(), "bspline of nothing");

	std::puts("intersections with a polyline which has less than two points");
	for (auto* list : { &none, &one })
	{
		hits.clear();
		CHECK(!geometry::segment_polyline_intersect(glm::vec2(0, 0), glm::vec2(5, 5), *list, hits), "segment against a short polyline");
		hits.clear();
		CHECK(geometry::circle_polyline_intersect(glm::vec2(0, 0), 5.0f, *list, hits) == 0, "circle against a short polyline");
		hits.clear();
		CHECK(geometry::arc_polyline_intersect(0.0f, glm::vec2(0, 0), 1.5f, 5.0f, false, *list, hits) == 0, "arc against a short polyline");
		hits.clear();
		CHECK(geometry::polyline_polyline_intersect(*list, two, hits) == 0, "short polyline against a polyline");
		hits.clear();
		CHECK(geometry::polyline_polyline_intersect(two, *list, hits) == 0, "polyline against a short polyline");
	}
	hits.clear();
	CHECK(geometry::polyline_polyline_intersect(none, none, hits) == 0, "two empty polylines");

	std::puts("empty curve");
	{
		Curve c;
		c.reverse();
		CHECK(c.empty(), "reverse of an empty curve");
		auto pieces = c.split_at_intersections();
		CHECK(pieces.empty() || pieces[0].empty(), "split of an empty curve");
		Curve single;
		single.add(glm::vec2(1, 1));
		single.reverse();
		CHECK(single.size() == 1, "reverse of a curve of one point");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
