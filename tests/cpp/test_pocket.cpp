// POCKET TESTS - automatic checks of the pocket (cam/pocket.cpp) toolpath generation.
//
// What for:
//   A pocket is computed from offsets, booleans and area calculations (common/curve.cpp). A small change
//   in that geometry code can silently produce a wrong toolpath (a pass cutting into an island, going outside
//   the part, missing passes...) that nobody notices until a part is machined. These tests rebuild a few known
//   pockets and check the result against what can be worked out by hand, so a regression is caught in seconds
//   instead of on the machine.
//
// What is checked, for each pocket mode (offset, zigzag, spiral):
//   - the outermost pass is inset by exactly the tool radius from the outline;
//   - no pass leaves the pocket, and none enters an island (inner shape) grown by the tool radius;
//   - a pocket smaller than the tool gives no path;
//   - zigzag scan lines are spaced by the stepover; finishing is the last pass, at the tool radius.
//
// Debug runtime:
//   build_pocket.bat compiles with the debug runtime (/MDd), whose checks stop the program on a misuse of the standard
//   containers, such as the comparison of the iterators of two different lists. The release runtime does not check it:
//   a pocket which "works" in release can break in the debug build of the application (gears.opp, radius 7).
//
// How it works:
//   The real Pocket::update() is run without any window or OpenGL: TestRenderer below is a renderer which
//   draws nothing. The shapes are put in a curve tree the same way the application does (ModCad::cad_tree()).
//
// Build: tests/cpp/build_pocket.bat   Run: tests/cpp/test_pocket.exe   (exit code 0 = all checks pass)
#include <pocket.h>
#include <cmath>
#include <cstdio>
#include <vector>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

static Curve rect(float x, float y, float w, float h)
{
	Curve c;
	c.add(glm::vec2(x, y)); c.add(glm::vec2(x + w, y)); c.add(glm::vec2(x + w, y + h)); c.add(glm::vec2(x, y + h)); c.add(glm::vec2(x, y));
	return c;
}
static Curve circle(float cx, float cy, float r)
{
	Curve c; c.add(glm::vec2(cx, cy), r, false); return c;
}

struct Result
{
	std::vector<Curve> curves;
	std::vector<glm::vec2> points;
	glm::vec2 lo = glm::vec2(1e9f), hi = glm::vec2(-1e9f);
};

// headless renderer: buffers are real objects (the toolpath writes to them) but draw nothing
struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

static Result run(std::vector<Curve> shapes, PocketMode mode, float radius, float stepover = 0, float finishing = 0)
{
	static TestRenderer renderer;
	static Camera camera;
	renderer.camera(&camera);
	Pocket pocket(&renderer);
	pocket.radius(radius);
	pocket.mode(mode);
	if (stepover > 0) pocket.stepover(stepover);
	if (finishing > 0) pocket.finishing(finishing);
	for (size_t i = 0; i < shapes.size(); i++) { shapes[i].tag((int)i); shapes[i].reference((int)i); }
	// same sequence as ModCad::cad_tree()
	// the toolpath owns its tree and deletes it
	TreeCurve* tree = new TreeCurve(shapes);
	tree->sort();
	tree->cw_alter();
	pocket.tree(tree);
	pocket.update();
	Result r;
	r.curves = pocket.coordinates();
	for (Curve& c : r.curves)
		for (const glm::vec2& p : c.coordinates())
		{
			r.points.push_back(p);
			r.lo = glm::min(r.lo, p); r.hi = glm::max(r.hi, p);
		}
	return r;
}

static const float EPS = 0.02f;

static void test_offset_rect()
{
	std::puts("offset rectangle");
	Result r = run({ rect(0, 0, 40, 30) }, PocketMode::Offset, 2);
	CHECK(!r.curves.empty(), "pocket produces paths");
	CHECK(std::fabs(r.lo.x - 2) < EPS && std::fabs(r.lo.y - 2) < EPS, "outermost pass is inset by the tool radius (min)");
	CHECK(std::fabs(r.hi.x - 38) < EPS && std::fabs(r.hi.y - 28) < EPS, "outermost pass is inset by the tool radius (max)");
	CHECK(r.curves.size() >= 5, "several concentric passes for a 40x30 pocket");
}

static void test_too_small()
{
	std::puts("too small");
	Result r = run({ rect(0, 0, 3, 3) }, PocketMode::Offset, 2);
	CHECK(r.curves.empty(), "tool larger than the pocket gives no path");
}

static void test_island()
{
	std::puts("island");
	for (PocketMode m : { PocketMode::Offset, PocketMode::Zigzag })
	{
		Result r = run({ rect(0, 0, 40, 30), rect(15, 10, 10, 10) }, m, 2);
		CHECK(!r.curves.empty(), "pocket with island produces paths");
		bool inside = false;
		for (auto& p : r.points)
			{
			// distance from the point to the island rectangle (15..25 x 10..20): the offset of its corners is round
			float dx = std::max(std::max(15 - p.x, p.x - 25), 0.0f), dy = std::max(std::max(10 - p.y, p.y - 20), 0.0f);
			if (std::sqrt(dx * dx + dy * dy) < 2 - EPS) inside = true;
		}
		CHECK(!inside, "no path enters the island grown by the tool radius");
		CHECK(r.lo.x >= 2 - EPS && r.hi.x <= 38 + EPS && r.lo.y >= 2 - EPS && r.hi.y <= 28 + EPS, "paths stay inside the pocket");
	}
}

static void test_zigzag()
{
	std::puts("zigzag");
	Result r = run({ rect(0, 0, 40, 30) }, PocketMode::Zigzag, 2, 3);
	CHECK(!r.curves.empty(), "zigzag produces paths");
	CHECK(r.lo.x >= 2 - EPS && r.hi.x <= 38 + EPS && r.lo.y >= 2 - EPS && r.hi.y <= 28 + EPS, "zigzag stays inside the pocket");
	// scan lines are horizontal : count the distinct y levels, 26 mm of travel every 3 mm
	std::vector<float> ys;
	for (const glm::vec2& p : r.points)
	{
		bool known = false;
		for (float y : ys) if (std::fabs(y - p.y) < 0.05f) known = true;
		if (!known) ys.push_back(p.y);
	}
	CHECK(ys.size() >= 8 && ys.size() <= 12, "about 26/3 scan lines (+ contour)");
}

static void test_circle()
{
	std::puts("circle");
	for (PocketMode m : { PocketMode::Offset, PocketMode::Spiral, PocketMode::Zigzag })
	{
		Result r = run({ circle(50, 50, 20) }, m, 3);
		CHECK(!r.curves.empty(), "circle pocket produces paths");
		bool outside = false;
		for (auto& p : r.points)
			if (glm::distance(p, glm::vec2(50, 50)) > 17 + 0.05f) outside = true;
		CHECK(!outside, "paths stay within radius - tool radius");
	}
}

static void test_finishing()
{
	std::puts("finishing");
	Result r = run({ rect(0, 0, 40, 30) }, PocketMode::Offset, 2, 0, 0.5f);
	CHECK(!r.curves.empty(), "finishing pocket produces paths");
	CHECK(std::fabs(r.lo.x - 2) < EPS && std::fabs(r.hi.x - 38) < EPS, "finishing pass is at the tool radius");
	// the roughing starts farther inside, finishing is machined last
	Curve& last = r.curves.back();
	glm::vec2 lo(1e9f), hi(-1e9f);
	for (auto& p : last.coordinates()) { lo = glm::min(lo, p); hi = glm::max(hi, p); }
	CHECK(std::fabs(lo.x - 2) < EPS && std::fabs(hi.x - 38) < EPS, "last path is the finishing contour");
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	test_offset_rect();
	test_too_small();
	test_island();
	test_zigzag();
	test_circle();
	test_finishing();
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}







