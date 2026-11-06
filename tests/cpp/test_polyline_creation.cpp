// POLYLINE CREATION TEST - checks how a polyline being drawn ends when it is validated (cad/polyline.cpp).
//
// What for:
//   While a polyline is drawn, its last point follows the mouse. Validating it (double click, right click or the space
//   key) must drop that moving point and keep the point of the last click. With a double click, the mouse always moves a
//   little between the two clicks: the polyline must end exactly where the first click was, not where the second one is.
//   The detection of the double click itself (time and distance in pixels) is in ModCad and needs the window.
//
// What is checked: the points after each click and each move of the mouse, then after the validation; a polyline of
//   two points placed; the first point placed twice.
//
// Build: tests/cpp/build_polyline_creation.bat   Run: tests/cpp/test_polyline_creation.exe   (exit code 0 = all checks pass)
#include <polyline.h>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
static void check_point(int line, glm::vec2 got, float ex, float ey, const char* msg)
{
	g_run++;
	if (glm::distance(got, glm::vec2(ex, ey)) > 0.0001f)
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

// what the tool does for a click: the mouse moves to the point, then the point is added
static void click(Polyline& p, glm::vec2 at)
{
	p.move(at);
	p.point(at);
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::puts("double click");
	{
		Polyline p(&renderer);
		click(p, glm::vec2(0, 0));
		click(p, glm::vec2(10, 0));
		click(p, glm::vec2(10, 10));		// the first click of the double click
		CHECK(p.points().size() == 4, "three points placed and the moving one");

		// the mouse moves a little, then the second click comes: it is not added, the polyline is validated
		p.move(glm::vec2(10.4f, 9.7f));
		p.done(true);
		auto pts = p.points();
		CHECK(pts.size() == 3, "the moving point is dropped");
		if (pts.size() == 3)
		{
			CHECK_POINT(pts[0], 0, 0, "first point");
			CHECK_POINT(pts[1], 10, 0, "second point");
			CHECK_POINT(pts[2], 10, 10, "the polyline ends at the first click, not at the moved mouse");
		}
	}

	std::puts("two points");
	{
		Polyline p(&renderer);
		click(p, glm::vec2(0, 0));
		click(p, glm::vec2(5, 5));
		p.move(glm::vec2(5.3f, 4.8f));
		p.done(true);
		auto pts = p.points();
		CHECK(pts.size() == 2, "a polyline of two points");
		if (pts.size() == 2)
			CHECK_POINT(pts[1], 5, 5, "it ends at the second click");
	}

	std::puts("first point");
	{
		Polyline p(&renderer);
		click(p, glm::vec2(0, 0));
		// not enough points: the tool does not validate (the points placed are the first one and the moving one)
		CHECK(p.points().size() == 2, "only the first point and the moving one: the tool ignores the double click");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
