// EXTEND TESTS - automatic checks of the "extend" tool geometry (cad/extend.cpp).
//
// What for:
//   The extend tool prolongs a line or an arc until it meets another entity. Choosing the right intersection is
//   easy to get wrong: the entity behind the end must be ignored, the nearest hit must win, an entity which
//   already touches the end must not stop the prolongation at length zero, and an arc must be followed along its
//   own circle in its own direction. These tests give known drawings and check the exact point reached.
//
// What is checked: a line against a line, a circle, an arc and a polyline; nearest of several boundaries; nothing
//   met (no result); boundary behind the end; boundary touching the end; a diagonal line; counter-clockwise and
//   clockwise arcs prolonged against a line and against another arc.
//
// Build: tests/cpp/build_extend.bat   Run: tests/cpp/test_extend.exe   (exit code 0 = all checks pass)
#include <extend.h>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
static bool near_point(glm::vec2 a, glm::vec2 b, float tol = 0.01f) { return glm::distance(a, b) < tol; }
#define CHECK_POINT(got, ex, ey, msg) do { g_run++; if (!near_point(got, glm::vec2(ex, ey))) { g_fail++; std::printf("  FAIL line %d: %s (got %g,%g expected %g,%g)\n", __LINE__, msg, got.x, got.y, (float)(ex), (float)(ey)); } } while (0)

static const float PI = 3.14159265f;

static Curve segment(float x1, float y1, float x2, float y2)
{
	Curve c; c.add(glm::vec2(x1, y1)); c.add(glm::vec2(x2, y2)); return c;
}
static Curve circle(float cx, float cy, float r)
{
	Curve c; c.add(glm::vec2(cx, cy), r, false); return c;
}
// arc from the angle a1 to the angle a2 (radians), counter-clockwise unless cw
static Curve arc(float cx, float cy, float r, float a1, float a2, bool cw)
{
	Curve c;
	glm::vec2 center(cx, cy);
	c.add(center + r * glm::vec2(std::cos(a1), std::sin(a1)), center, r, cw);
	c.add(center + r * glm::vec2(std::cos(a2), std::sin(a2)));
	return c;
}

static void test_line()
{
	std::puts("line");
	glm::vec2 r;
	std::vector<Curve> wall = { segment(15, -10, 15, 10) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), wall, r), "line meets a vertical wall");
	CHECK_POINT(r, 15, 0, "line stops on the wall");

	// the other end of the same line goes away from the wall
	CHECK(!cad_extend::line(glm::vec2(10, 0), glm::vec2(0, 0), wall, r), "wall behind the end is ignored");

	std::vector<Curve> two = { segment(20, -10, 20, 10), segment(15, -10, 15, 10) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), two, r), "two walls");
	CHECK_POINT(r, 15, 0, "the nearest wall wins");

	std::vector<Curve> short_wall = { segment(15, 5, 15, 10) };
	CHECK(!cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), short_wall, r), "a wall which is not on the way is missed");

	std::vector<Curve> touching = { segment(10, -5, 10, 5), segment(25, -5, 25, 5) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), touching, r), "touching boundary is skipped");
	CHECK_POINT(r, 25, 0, "continues to the next boundary");

	std::vector<Curve> diagonal = { segment(0, 10, 20, 10) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(3, 3), diagonal, r), "diagonal line");
	CHECK_POINT(r, 10, 10, "diagonal line reaches y = 10");

	std::vector<Curve> none;
	CHECK(!cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), none, r), "no boundary, nothing met");
}

static void test_line_other_shapes()
{
	std::puts("line against circle, arc, polyline");
	glm::vec2 r;
	std::vector<Curve> c = { circle(20, 0, 3) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), c, r), "circle");
	CHECK_POINT(r, 17, 0, "line stops on the near side of the circle");

	std::vector<Curve> a = { arc(20, 0, 5, PI * 0.5f, PI * 1.5f, false) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), a, r), "arc");
	CHECK_POINT(r, 15, 0, "line stops on the arc (left half of the circle)");

	// the right half of the circle is not part of the arc: the line must pass through the arc, not stop on the far side
	std::vector<Curve> right_half = { arc(20, 0, 5, -PI * 0.5f, PI * 0.5f, false) };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), right_half, r), "arc on the far side");
	CHECK_POINT(r, 25, 0, "only the real arc counts");

	Curve poly;
	poly.add(glm::vec2(30, -10)); poly.add(glm::vec2(30, 0)); poly.add(glm::vec2(40, 10));
	std::vector<Curve> p = { poly };
	CHECK(cad_extend::line(glm::vec2(0, 0), glm::vec2(10, 0), p, r), "polyline");
	CHECK_POINT(r, 30, 0, "line stops at the polyline");
}

static void test_arc()
{
	std::puts("arc");
	glm::vec2 r;
	// quarter of circle from (10,0) to (0,10), counter-clockwise: it goes on to the left
	glm::vec2 start(10, 0), stop(0, 10), center(0, 0);
	std::vector<Curve> wall = { segment(-5, -20, -5, 20) };
	CHECK(cad_extend::arc(start, center, stop, false, wall, r), "ccw arc meets a wall");
	CHECK_POINT(r, -5, 8.660254f, "ccw arc stops on the wall");

	// the wall on the right is on the circle but behind the start: the long way around is followed
	std::vector<Curve> behind = { segment(5, -20, 5, 20) };
	CHECK(cad_extend::arc(start, center, stop, false, behind, r), "wall met after turning around");
	CHECK_POINT(r, 5, -8.660254f, "the arc turns the whole way, never back");

	// the clockwise arc goes the other way: from (0,10) to (10,0) clockwise, prolonged at (10,0) it goes down
	std::vector<Curve> floor = { segment(-20, -5, 20, -5) };
	CHECK(cad_extend::arc(stop, center, start, true, floor, r), "cw arc meets a floor");
	CHECK_POINT(r, 8.660254f, -5, "cw arc stops on the floor");

	// the start of an arc is prolonged by giving the arc reversed
	CHECK(cad_extend::arc(stop, center, start, true, wall, r), "reversed arc (start prolonged)");
	CHECK_POINT(r, -5, -8.660254f, "start prolonged clockwise: down from (10,0) to the wall at x = -5");

	std::vector<Curve> other_arc = { arc(0, 12, 6, PI, PI * 2.0f, false) };
	CHECK(cad_extend::arc(start, center, stop, false, other_arc, r), "arc against another arc (lower half of a circle centred at 0,12)");
	CHECK(std::fabs(glm::distance(r, center) - 10.0f) < 0.01f && std::fabs(glm::distance(r, glm::vec2(0, 12)) - 6.0f) < 0.01f, "intersection lies on both circles");

	std::vector<Curve> far_away = { segment(100, 100, 120, 100) };
	CHECK(!cad_extend::arc(start, center, stop, false, far_away, r), "nothing met on the circle");
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	test_line();
	test_line_other_shapes();
	test_arc();
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
