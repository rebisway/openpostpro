// EXTEND ON REAL SHAPES - checks the extend tool on the real Line and Arc objects of the drawing (cad/extend_shape.cpp).
//
// What for:
//   test_extend.cpp checks the geometry with ideal curves. This one builds a drawing with the real shape classes,
//   exactly as the application does (a layer holding lines and arcs), then asks what a click at a given point would
//   extend, as the tool does. It catches what the ideal curves can not show: the way a shape reports its ends, its
//   direction and its radius, which end is chosen from the click, and the order the shapes are searched.
//
// Build: tests/cpp/build_extend_shapes.bat   Run: tests/cpp/test_extend_shapes.exe   (exit code 0 = all checks pass)
#include <extend_shape.h>
#include <line.h>
#include <arc.h>
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

static const float PI = 3.14159265f;
static glm::vec2 on_circle(float r, float degrees) { return r * glm::vec2(std::cos(degrees * PI / 180), std::sin(degrees * PI / 180)); }

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

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);
	Layer layer(&renderer);
	std::vector<Layer*> layers = { &layer };

	Shape* target = nullptr;
	bool at_start = false;
	glm::vec2 result;

	std::puts("arc, counter-clockwise");
	// quarter of circle (10,0) -> (0,10), with a wall on the left and a floor below
	Arc* quarter = arc(&renderer, glm::vec2(0, 0), glm::vec2(10, 0), glm::vec2(0, 10), false);
	layer.add(quarter);
	layer.add(line(&renderer, -5, -20, -5, 20));
	layer.add(line(&renderer, -20, -5, 20, -5));

	int status = cad_extend::find(layers, on_circle(10, 80), target, at_start, result);
	CHECK(status == 2, "click near the stop of the arc: something is found");
	CHECK(target == quarter, "the arc is the target");
	CHECK(!at_start, "the stop is the end prolonged");
	CHECK_POINT(result, -5, 8.660254f, "the arc goes on, counter-clockwise, up to the wall");

	status = cad_extend::find(layers, on_circle(10, 10), target, at_start, result);
	CHECK(status == 2 && target == quarter, "click near the start of the arc");
	CHECK(at_start, "the start is the end prolonged");
	CHECK_POINT(result, 8.660254f, -5, "the start goes on, clockwise, down to the floor");

	std::puts("arc, clockwise");
	Arc* reverse = arc(&renderer, glm::vec2(0, 0), glm::vec2(0, 10), glm::vec2(10, 0), true);
	layer.add(reverse);
	// the same quarter of circle, drawn the other way: the shape under the click is the one above in the layer
	status = cad_extend::find(layers, on_circle(10, 10), target, at_start, result);
	CHECK(status == 2 && target == reverse, "clockwise arc found");
	CHECK(!at_start, "its stop (10,0) is nearest to the click");
	CHECK_POINT(result, 8.660254f, -5, "the clockwise arc goes down to the floor");

	std::puts("line towards an arc");
	Layer second(&renderer);
	std::vector<Layer*> layers2 = { &second };
	Line* l = line(&renderer, -30, 0, -20, 0);
	second.add(l);
	second.add(arc(&renderer, glm::vec2(0, 0), glm::vec2(0, 10), glm::vec2(0, -10), false)); // left half of the circle
	status = cad_extend::find(layers2, glm::vec2(-21, 0), target, at_start, result);
	CHECK(status == 2 && target == l, "line found");
	CHECK_POINT(result, -10, 0, "the line stops on the arc");

	std::puts("locked layer");
	{
		// the locked wall still stops a free line, but a locked line is not prolonged
		Layer locked_layer(&renderer);
		locked_layer.add(line(&renderer, 15, -10, 15, 10));
		Layer free_layer(&renderer);
		Line* free = line(&renderer, 0, 0, 10, 0);
		free_layer.add(free);
		std::vector<Layer*> layers3 = { &locked_layer, &free_layer };
		status = cad_extend::find(layers3, glm::vec2(9, 0), target, at_start, result);
		CHECK(status == 2 && target == free, "a free line is prolonged up to a locked wall");
		CHECK_POINT(result, 15, 0, "it stops on the locked wall");

		locked_layer.locked(false);
		free_layer.locked(true);
		status = cad_extend::find(layers3, glm::vec2(9, 0), target, at_start, result);
		CHECK(status == 0, "a line of a locked layer is not prolonged");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}


