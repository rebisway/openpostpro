// LINE CREATION TEST - checks the way segments follow each other while they are drawn (cad/line.cpp).
//
// What for:
//   With the line tool, the first click places the start of a segment and the second click creates it. The tool then
//   starts the next segment at the point which has just been clicked, so that a chain of segments is drawn with one
//   click per segment. This test replays that sequence on real Line objects: the end of a segment is the start of the
//   next one, and the segment which is being drawn follows the mouse. The tool itself (clicks, double click, right
//   click) is in ModCad and needs the window.
//
// What is checked: the return value of each click, the start and the end of each segment of a chain of three segments,
//   and that the segment which is being drawn follows the mouse.
//
// Build: tests/cpp/build_line_creation.bat   Run: tests/cpp/test_line_creation.exe   (exit code 0 = all checks pass)
#include <line.h>
#include <cmath>
#include <cstdio>
#include <vector>

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

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::puts("chain of segments");
	{
		// what the tool does: a click adds a point, and when the segment is complete a new one is started at the same point
		std::vector<Line*> created;
		Line* current = new Line(&renderer);
		glm::vec2 clicks[] = { {0, 0}, {10, 0}, {10, 10}, {0, 10} };
		bool completed[4];
		for (int i = 0; i < 4; i++)
		{
			current->move(clicks[i]);
			completed[i] = current->point(clicks[i]);
			if (completed[i])
			{
				created.push_back(current);
				current = new Line(&renderer);
				current->point(clicks[i]);		// the next segment starts where the previous one ends
			}
		}
		CHECK(!completed[0] && completed[1] && completed[2] && completed[3], "the first click only places the start, the next ones create a segment");
		CHECK(created.size() == 3, "three segments for four clicks");
		if (created.size() == 3)
		{
			CHECK_POINT(created[0]->p1(), 0, 0, "first segment starts at the first click");
			CHECK_POINT(created[0]->p2(), 10, 0, "first segment ends at the second click");
			CHECK_POINT(created[1]->p1(), 10, 0, "second segment starts where the first one ends");
			CHECK_POINT(created[1]->p2(), 10, 10, "second segment ends at the third click");
			CHECK_POINT(created[2]->p1(), 10, 10, "third segment starts where the second one ends");
			CHECK_POINT(created[2]->p2(), 0, 10, "third segment ends at the fourth click");
		}

		// the segment which is being drawn follows the mouse, from the last click
		CHECK_POINT(current->p1(), 0, 10, "the pending segment starts at the last click");
		current->move(glm::vec2(5, 12));
		CHECK_POINT(current->p2(), 5, 12, "the pending segment follows the mouse");
		CHECK(!current->done(), "the pending segment is not created yet");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
