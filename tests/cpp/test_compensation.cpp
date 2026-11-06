// COMPENSATION TEST - the radius compensation (G41 / G42) of the "follow" toolpath: the side of the tool and the exit.
//
// What for:
//   The user tells where the tool is relative to the shape (outside, inside, or centered). The machine needs the side of
//   the tool relative to the direction of the movement (G41 left, G42 right). A mistake here gives a cut which is too big
//   or too small by the width of the tool (the kerf, for a plasma torch). The path also has an entry and an exit
//   (a line or an arc), the exit being the entry in the other direction, where the compensation is cancelled.
//
// What is checked: the table of the side (direction of the path x outside / inside), the same for an open path (the direction
//   is the one of the path closed by a line, which is what the follow direction gives), the entry and the exit of a path as a
//   line and as an arc, and a path without exit.
//
// Build: tests/cpp/build_compensation.bat   Run: tests/cpp/test_compensation.exe   (exit code 0 = all checks pass)
#include <follow.h>
#include <group.h>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
static void check_point(int line, glm::vec2 got, float ex, float ey, const char* msg)
{
	g_run++;
	if (glm::distance(got, glm::vec2(ex, ey)) > 0.01f)
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

static Curve square()
{
	Curve c;
	c.add(glm::vec2(0, 0)); c.add(glm::vec2(10, 0)); c.add(glm::vec2(10, 10)); c.add(glm::vec2(0, 10)); c.add(glm::vec2(0, 0));
	return c;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::puts("the table");
	// seen from above: the inside of a counter-clockwise contour is on the left of the movement
	CHECK(compensation_for(ToolSide::Outside, false) == ToolRadiusCompensation::Right, "counter-clockwise, outside: right (G42)");
	CHECK(compensation_for(ToolSide::Inside, false) == ToolRadiusCompensation::Left, "counter-clockwise, inside: left (G41)");
	CHECK(compensation_for(ToolSide::Outside, true) == ToolRadiusCompensation::Left, "clockwise, outside: left (G41)");
	CHECK(compensation_for(ToolSide::Inside, true) == ToolRadiusCompensation::Right, "clockwise, inside: right (G42)");
	CHECK(compensation_for(ToolSide::Centered, true) == ToolRadiusCompensation::None, "centered: none");
	CHECK(compensation_for(ToolSide::Centered, false) == ToolRadiusCompensation::None, "centered: none");

	std::puts("follow: the side follows the direction");
	{
		Follow f(&renderer);
		f.add(square());
		f.cw(false);
		f.tool_side(ToolSide::Outside);
		f.update();
		CHECK(f.radius_compensation() == ToolRadiusCompensation::Right, "counter-clockwise follow, tool outside");
		f.cw(true);
		f.update();
		CHECK(f.radius_compensation() == ToolRadiusCompensation::Left, "the compensation changes with the direction");
		f.tool_side(ToolSide::Inside);
		CHECK(f.radius_compensation() == ToolRadiusCompensation::Right, "clockwise follow, tool inside");
		f.tool_side(ToolSide::Centered);
		CHECK(f.radius_compensation() == ToolRadiusCompensation::None, "centered follow");
	}

	std::puts("follow: an open path");
	{
		// an open path (an L) has the direction of the path closed by a straight line: the follow direction gives it
		Curve l;
		l.add(glm::vec2(0, 0)); l.add(glm::vec2(10, 0)); l.add(glm::vec2(10, 10));
		Follow f(&renderer);
		f.add(l);
		f.cw(false);
		f.tool_side(ToolSide::Inside);
		f.update();
		CHECK(f.radius_compensation() == ToolRadiusCompensation::Left, "open path, counter-clockwise, inside");
		auto out = f.coordinates();
		CHECK(out.size() == 1, "one path");
		if (out.size() == 1)
			CHECK(!out[0].cw(), "the path has the requested direction (closed by a line)");
	}

	std::puts("entry as an arc on a circle and on an ellipse");
	{
		Curve circle;
		circle.add(glm::vec2(0, 0), 20.0f, false);
		Curve ellipse;
		for (int i = 0; i <= 120; i++)
		{
			float a = glm::two_pi<float>() * i / 120;
			ellipse.add(glm::vec2(30 * std::cos(a), 15 * std::sin(a)));
		}
		ellipse.back().point = ellipse.front().point;	// exactly closed
		Curve shapes[2] = { circle, ellipse };
		for (int k = 0; k < 4; k++)
		{
			// 2 and 3: the start of the path is moved along the shape
			Follow f(&renderer);
			f.add(shapes[k % 2]);
			f.cw(false);
			if (k >= 2)
				f.start_point_length(k == 2 ? 30.0f : 21.5f);
			f.start_point_type(StartPointType::arc);
			f.start_point_offset(5);
			f.lead_out(k == 0);
			f.update();
			auto out = f.coordinates();
			CHECK(out.size() == 1, "one path");
			if (out.size() == 1)
			{
				Curve& c = out[0];
				// the entry is on the outside of the shape by default: its center is farther from the middle than the contour
				if (k % 2 == 0)
					CHECK(glm::distance(c.front().center, glm::vec2(0, 0)) > 20.0f, "the entry of a circle is on the requested side");
				CHECK(c.front().type == SegmentType::Arc, "the entry is an arc");
				// a quarter of circle of radius 5: the chord is 5 * sqrt(2)
				CHECK(std::fabs(glm::distance(c.front().point, c[1].point) - 5.0f * std::sqrt(2.0f)) < 0.05f, "the entry is a quarter of circle");
			}
		}
	}

	std::puts("entry and exit as a line");
	{
		Follow f(&renderer);
		f.add(square());
		f.cw(false);
		f.start_point_type(StartPointType::line);
		f.start_point_offset(5);
		f.start_point_inside(false);
		f.lead_out(true);
		f.update();
		auto out = f.coordinates();
		CHECK(out.size() == 1, "one path");
		if (out.size() == 1)
		{
			Curve& c = out[0];
			// the square (4 sides + the closing point) with the entry in front and the exit behind: the exit starts on the closing point
			CHECK(c.size() == 7, "the contour, the entry and the exit");
			glm::vec2 source = c.front().point;
			CHECK_POINT(c.back().point, source.x, source.y, "the path ends where the entry starts");
			CHECK(std::fabs(glm::distance(source, c[1].point) - 5.0f) < 0.01f, "the entry is as long as the distance");
			CHECK(c[c.size() - 2].type == SegmentType::Line, "the exit is a line");
			CHECK_POINT(c[1].point, c[c.size() - 2].point.x, c[c.size() - 2].point.y, "the exit starts where the contour ends");
			CHECK(f.lead_out(), "the exit is on");
		}

		f.lead_out(false);
		f.update();
		out = f.coordinates();
		if (out.size() == 1)
		{
			Curve& c = out[0];
			CHECK(c.size() == 7, "without exit: the contour (with its closing point twice) and the entry");
			CHECK_POINT(c.back().point, c[1].point.x, c[1].point.y, "without exit the path ends on the contour");
		}
	}

	std::puts("entry and exit as a quarter of circle");
	{
		Follow f(&renderer);
		f.add(square());
		f.cw(false);
		f.start_point_type(StartPointType::arc);
		f.start_point_offset(6);
		f.start_point_length(5);		// the middle of the first side: the entry does not run along the neighbour side of a corner
		f.lead_out(true);
		f.update();
		auto out = f.coordinates();
		CHECK(out.size() == 1, "one path");
		if (out.size() == 1)
		{
			Curve& c = out[0];
			CHECK(c.size() >= 7, "the contour, the entry and the exit");
			CHECK(c.front().type == SegmentType::Arc, "the entry is an arc");
			CHECK(c[c.size() - 2].type == SegmentType::Arc, "the exit is an arc");
			// counter-clockwise square, tool outside, start in the middle of the bottom side (5,0): the entry is a quarter of circle
			// of radius 6 around (5,-6), which arrives at (5,0) tangent to the side
			CHECK_POINT(c.front().point, -1, -6, "the quarter of circle starts behind its center");
			CHECK_POINT(c.front().center, 5, -6, "its center is on the side of the tool");
			CHECK(std::fabs(c.front().radius - 6.0f) < 0.01f, "its radius is the distance");
			CHECK_POINT(c[1].point, 5, 0, "it ends on the contour");
			CHECK(c.front().cw, "it turns towards the contour");
			// the exit: the same circle goes on after the corner, in the same direction
			CHECK(c[c.size() - 2].cw == c.front().cw, "the exit turns the same way as the entry");
			CHECK_POINT(c[c.size() - 2].center, 5, -6, "the exit is around the same center");
			CHECK_POINT(c.back().point, 11, -6, "the exit ends in front of the center: the contour is left tangent");
		}
	}

	std::puts("the side of the tool gives the side of the entry");
	{
		Follow f(&renderer);
		f.add(square());
		f.cw(false);
		f.start_point_type(StartPointType::line);
		f.start_point_offset(6);
		f.start_point_length(5);		// the middle of the bottom side, which the path follows towards +x
		auto first = [&]() { f.update(); auto o = f.coordinates(); return o[0].front().point; };

		// centered: the side of the entry is free
		f.start_point_inside(false);
		CHECK_POINT(first(), 5, -6, "centered, not inside: the entry is below the bottom side (outside)");
		f.start_point_inside(true);
		CHECK_POINT(first(), 5, 6, "centered, inside: the entry is above the bottom side (inside)");

		// with a compensation, the side of the entry is the side of the tool, whatever the field was
		f.tool_side(ToolSide::Outside);
		CHECK_POINT(first(), 5, -6, "tool outside: the entry is outside");
		f.tool_side(ToolSide::Inside);
		CHECK_POINT(first(), 5, 6, "tool inside: the entry is inside");
		CHECK(f.start_point_inside() != 0, "the field is true with a tool inside");
		f.tool_side(ToolSide::Outside);
		CHECK(f.start_point_inside() == 0, "the field is false with a tool outside");
	}

	std::puts("choosing the compensation moves a start in a corner, the user can change it afterwards");
	{
		// a square of 40, followed clockwise from its bottom-left corner (going up), tool radius 3, entry distance 6
		Curve big;
		big.add(glm::vec2(0, 0)); big.add(glm::vec2(0, 40)); big.add(glm::vec2(40, 40)); big.add(glm::vec2(40, 0)); big.add(glm::vec2(0, 0));
		auto make = [&](Follow& f, StartPointType type, float start)
		{
			f.add(big);
			f.cw(true);
			f.radius(3);
			f.start_point_type(type);
			f.start_point_offset(6);
			f.start_point_length(start);
			f.update();
		};

		for (StartPointType type : { StartPointType::arc, StartPointType::line })
		{
			const char* name = type == StartPointType::arc ? "arc" : "line";
			Follow f(&renderer);
			make(f, type, 0);
			CHECK(f.start_point_length() == 0, "the start is in the corner before the compensation");

			// the tool is inside: the start is moved along the contour, the field shows it
			f.tool_side(ToolSide::Inside);
			f.update();
			// the shift is at least the radius of the tool for a line, the distance of the entry plus the radius for an arc (a step more at most)
			float least = type == StartPointType::arc ? 6.0f + 3.0f : 3.0f;
			CHECK(f.start_point_length() >= least - 0.05f && f.start_point_length() <= least + 2.0f, name);
			auto out = f.coordinates();
			CHECK(out.size() == 1, "one path");
			if (out.size() == 1)
			{
				// where the tool starts is inside of the square, at least the radius of the tool away from the sides
				glm::vec2 p = out[0].front().point;
				CHECK(p.x >= 2.9f && p.x <= 37.1f && p.y >= 2.9f && p.y <= 37.1f, "inside: the entry starts inside the part");
			}

			// the user puts the start back in the corner: it is not moved again
			f.start_point_length(0);
			f.update();
			out = f.coordinates();
			CHECK(f.start_point_length() == 0, "the position chosen by the user is kept");
			if (out.size() == 1)
			{
				CHECK_POINT(out[0][1].point, 0, 0, "the contour starts in the corner again");
				CHECK(out[0].front().point.y < 0.5f, "and the entry comes from outside: it is the choice of the user");
			}

			// outside, the entry already comes from outside: the start is not moved
			Follow g(&renderer);
			make(g, type, 0);
			g.tool_side(ToolSide::Outside);
			g.update();
			CHECK(g.start_point_length() == 0, "outside: the start is not moved");
			out = g.coordinates();
			if (out.size() == 1)
				CHECK(out[0].front().point.x < 0 || out[0].front().point.y < 0, "outside: the entry starts outside the part");
		}

		// the tool inside, and the start already far from the corner: not moved
		Follow g(&renderer);
		make(g, StartPointType::line, 20);
		g.tool_side(ToolSide::Inside);
		g.update();
		CHECK(g.start_point_length() == 20, "the start is not moved when the entry fits");
	}

	std::puts("the exit is managed by the group like the entry");
	{
		Renderer* r = &renderer;
		Group g(r);
		Follow* f = new Follow(r);
		f->add(square());
		g.start_point_type(StartPointType::line);
		g.add(f);
		CHECK(!f->lead_out(), "no exit by default");
		g.lead_out(true);
		CHECK(f->lead_out(), "the exit of the group is given to its toolpaths");
		Follow* later = new Follow(r);
		later->add(square());
		g.add(later);
		CHECK(later->lead_out(), "a toolpath added to the group has the exit of the group");
		g.lead_out(false);
		CHECK(!f->lead_out() && !later->lead_out(), "the exit is removed from all of them");
	}

	std::puts("no exit without an entry");
	{
		Follow f(&renderer);
		f.add(square());
		f.start_point_type(StartPointType::normal);
		f.lead_out(true);
		f.update();
		CHECK(!f.lead_out(), "an exit needs an entry");
		auto out = f.coordinates();
		CHECK(out.size() == 1 && out[0].size() == 6, "the path is the contour only");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail ? 1 : 0;
}
