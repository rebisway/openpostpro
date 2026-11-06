// CURVE TESTS - automatic checks of the geometry engine (common/curve.cpp).
//
// What for:
//   Curve is the base of everything in the CAM: offsets (tool radius compensation, pockets, outlines),
//   boolean operations (union, subtract, intersect) and area (used to find the direction of a shape).
//   The README warns that these calculations are not reliable for every shape. These tests compare the
//   results with exact values worked out by hand (a 10x10 square offset by 1 has an 8x8 inside, two
//   overlapping squares have a known union area, etc.), so that fixing or optimising the geometry code
//   cannot silently break cases which used to work.
//
// What is checked: area/perimeter/point-in-shape, offset in both directions (square, circle, vanishing shapes),
//   union / intersection / subtraction of squares and circles. Some tests record the CURRENT behaviour
//   (e.g. a boolean with no crossing returns nothing); if that behaviour is changed on purpose, update the test.
//
// Run it after any change in curve.cpp or geometry.cpp. Exit code 0 = all checks pass.
// Build: tests/cpp/build.bat   Run: tests/cpp/test_curve.exe
#include <curve.h>
#include <geometry.h>
#include <cmath>
#include <cstdio>
#include <vector>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
#define NEAR(a, b, tol, msg) do { double _a=(a), _b=(b); g_run++; if (std::fabs(_a-_b) > (tol)) { g_fail++; std::printf("  FAIL %s:%d %s (got %g, expected %g)\n", __FILE__, __LINE__, msg, _a, _b); } } while (0)

static Curve rect(float x, float y, float w, float h, bool cw = false)
{
	Curve c;
	c.add(glm::vec2(x, y)); c.add(glm::vec2(x + w, y)); c.add(glm::vec2(x + w, y + h)); c.add(glm::vec2(x, y + h)); c.add(glm::vec2(x, y));
	c.cw(cw);
	return c;
}
static Curve circle(float cx, float cy, float r)
{
	Curve c;
	c.add(glm::vec2(cx, cy), r, false);
	return c;
}
static float total_area(std::vector<Curve>& v) { float s = 0; for (auto& c : v) s += std::fabs(c.area()); return s; }

static void test_basic()
{
	std::puts("basic");
	Curve r = rect(0, 0, 10, 4);
	CHECK(r.closed(), "rect closed");
	NEAR(std::fabs(r.area()), 40, 1e-3, "rect area");
	NEAR(r.length(), 28, 1e-3, "rect perimeter");
	CHECK(r.inside(glm::vec2(5, 2)), "point inside");
	CHECK(!r.inside(glm::vec2(15, 2)), "point outside");
	Curve c = circle(0, 0, 5);
	NEAR(std::fabs(c.area()), 3.14159265 * 25, 0.01, "circle area");
	NEAR(c.length(), 2 * 3.14159265 * 5, 0.05, "circle perimeter");
}

static void test_offset()
{
	std::puts("offset");
	for (bool cw : { false, true })
	{
		Curve r = rect(0, 0, 10, 10, cw);
		auto a = r.offset(1);
		auto b = r.offset(-1);
		CHECK(a.size() == 1 && b.size() == 1, "square offset gives one curve each way");
		if (a.size() == 1 && b.size() == 1)
		{
			float aa = std::fabs(a[0].area()), ba = std::fabs(b[0].area());
			float lo = std::min(aa, ba), hi = std::max(aa, ba);
			NEAR(lo, 64, 0.05, "square inward offset area (8x8)");
			// outward: 12x12 with rounded corners = 144 - (4-pi)*1
			NEAR(hi, 144 - (4 - 3.14159265), 0.01, "square outward offset area (rounded corners)");
		}
		auto over = r.offset(-6);
		auto over2 = r.offset(6);
		CHECK(over.empty() || over2.empty(), "square inward offset past half width vanishes");
	}
	Curve c = circle(0, 0, 5);
	auto a = c.offset(1), b = c.offset(-1);
	CHECK(a.size() == 1 && b.size() == 1, "circle offset one curve each way");
	if (a.size() == 1 && b.size() == 1)
	{
		float la = a[0].length(), lb = b[0].length();
		NEAR(std::min(la, lb), 2 * 3.14159265 * 4, 0.1, "circle inward offset length");
		NEAR(std::max(la, lb), 2 * 3.14159265 * 6, 0.1, "circle outward offset length");
	}
	Curve small = rect(0, 0, 1, 1);
	auto gone = small.offset(1);
	auto gone2 = small.offset(-1);
	CHECK(gone.empty() || gone2.empty(), "tiny shape inward offset vanishes");
}

static void test_booleans()
{
	std::puts("booleans");
	Curve a = rect(0, 0, 10, 10), b = rect(5, 5, 10, 10);
	auto u = a.boolean_union(b);
	CHECK(u.size() == 1, "union overlap -> 1 curve");
	NEAR(total_area(u), 175, 0.05, "union area");
	Curve a2 = rect(0, 0, 10, 10), b2 = rect(5, 5, 10, 10);
	auto i = a2.boolean_intersect(b2);
	CHECK(i.size() == 1, "intersect overlap -> 1 curve");
	NEAR(total_area(i), 25, 0.05, "intersect area");
	Curve a3 = rect(0, 0, 10, 10), b3 = rect(5, 5, 10, 10);
	auto s = a3.boolean_substract(b3);
	NEAR(total_area(s), 75, 0.05, "substract area");
	Curve a4 = rect(0, 0, 10, 10), b4 = rect(20, 0, 5, 5);
	auto d = a4.boolean_union(b4);
	CHECK(d.empty(), "union of disjoint curves returns nothing (current behaviour: caller keeps both)");
	Curve a5 = rect(0, 0, 10, 10), hole = rect(3, 3, 4, 4);
	auto h = a5.boolean_substract(hole);
	CHECK(h.empty(), "substract with no boundary crossing returns nothing (current behaviour)");
	Curve ca = circle(0, 0, 5), cb = circle(6, 0, 5);
	auto cu = ca.boolean_union(cb);
	CHECK(cu.size() == 1, "circle union -> 1 curve");
	Curve ca2 = circle(0, 0, 5), cb2 = circle(6, 0, 5);
	auto ci = ca2.boolean_intersect(cb2);
	// lens area = 2 r^2 acos(d/2r) - d/2 sqrt(4r^2-d^2)
	double lens = 2 * 25 * std::acos(0.6) - 3 * std::sqrt(100 - 36);
	NEAR(ci.empty() ? 0 : ci[0].length(), 2 * 5 * 2 * std::acos(0.6), 0.05, "circle lens perimeter");
	NEAR(total_area(ci), lens, 0.02, "circle lens area");
}

int main()
{
	test_basic();
	test_offset();
	test_booleans();
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}


