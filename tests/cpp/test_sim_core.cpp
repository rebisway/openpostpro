// SIMULATOR CORE TEST - checks the core of the simulator, without a window: paths, times, player, material, loader and the translator in Python.
//
// What for:
//   The simulator shows what a program does to a block. If its core is wrong the picture is wrong, and it looks like a mistake of the
//   program: a time which is not the one of the machine, an arc which goes the long way, a stop which is skipped, a pocket which is
//   too deep or too shallow. This test checks the numbers without drawing anything.
//
// What is checked:
//   - the path of a move: length, points and bounds of lines, arcs (both directions), a full circle and a helix;
//   - the times of a program (feeds, rapids, dwell) and its bounds;
//   - the player: real time with a speed, pause and resume, the stop M0 of the program (and M1 as a setting), the dwell, full speed
//     (the waits are skipped), going to a line, stepping, going back, going to the end, the pieces of path it reports;
//   - the material: the shape of the tools (flat, ball, cone, torus), a plunge, the cut through of a torch, the removed volume, the
//     zone which changed, the step of the grid;
//   - the loader by slices of time, cancelling, a translator which does not start;
//   - the translator in Python (simulator/gcode.py) from a program to the material, an exception in a translator, a missing module.
//
// Build: tests/cpp/build_sim_core.bat   Run: tests/cpp/test_sim_core.exe   (exit code 0 = all checks pass)
#include <sim_geometry.h>
#include <sim_program.h>
#include <sim_player.h>
#include <sim_material.h>
#include <sim_loader.h>
#include <sim_translator.h>
#include <script.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
#define CHECK_NEAR(a, b, tol, msg) do { g_run++; double _a = (a), _b = (b); if (std::fabs(_a - _b) > (tol)) { g_fail++; std::printf("  FAIL line %d: %s (got %g, expected %g)\n", __LINE__, msg, _a, _b); } } while (0)

static const double PI = 3.14159265358979323846;

static SimCommand rapid(double x, double y, double z)
{
	SimCommand c;
	c.type = SimCommandType::Rapid;
	c.target = glm::dvec3(x, y, z);
	return c;
}
static SimCommand linear(double x, double y, double z, double f)
{
	SimCommand c;
	c.type = SimCommandType::Linear;
	c.target = glm::dvec3(x, y, z);
	c.feed = f;
	return c;
}
static SimCommand arc(double x, double y, double z, double cx, double cy, double cz, bool cw, double f, SimPlane plane = SimPlane::XY)
{
	SimCommand c;
	c.type = SimCommandType::Arc;
	c.target = glm::dvec3(x, y, z);
	c.center = glm::dvec3(cx, cy, cz);
	c.cw = cw;
	c.feed = f;
	c.plane = plane;
	return c;
}
static SimCommand dwell(double seconds)
{
	SimCommand c;
	c.type = SimCommandType::Dwell;
	c.seconds = seconds;
	return c;
}
static SimCommand stop(bool optional, bool end)
{
	SimCommand c;
	c.type = SimCommandType::Stop;
	c.optional = optional;
	c.end = end;
	return c;
}
static SimCommand spindle(int state)
{
	SimCommand c;
	c.type = SimCommandType::Spindle;
	c.state = state;
	c.rpm = state ? 10000 : 0;
	return c;
}

static void number_lines(SimProgram& p, std::vector<SimCommand> commands)
{
	int line = 1;
	for (auto& c : commands)
	{
		c.line = line++;
		p.add(c);
	}
}

// a translator written here: one rapid for each line which has a number
struct FakeTranslator : SimTranslator
{
	bool ok = true;
	int started = 0, closed = 0;
	bool start(const std::map<std::string, double>&, std::string& error) override
	{
		started++;
		if (!ok)
			error = "the fake translator does not start";
		return ok;
	}
	void translate(const std::string& text, int number, std::vector<SimCommand>& out) override
	{
		if (!text.empty())
		{
			SimCommand c = rapid(std::atof(text.c_str()), 0, 0);
			c.line = number;
			out.push_back(c);
		}
	}
	void close() override { closed++; }
};

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);

	std::puts("paths");
	{
		SimCommand quarter = arc(0, 10, 0, 0, 0, 0, false, 600);
		SimPath p(glm::dvec3(10, 0, 0), quarter);
		CHECK(p.arc(), "an arc");
		CHECK_NEAR(p.length(), PI * 10 / 2, 1e-6, "a quarter of circle counter-clockwise");
		glm::dvec3 mid = p.at(0.5);
		CHECK_NEAR(mid.x, 10 * std::cos(PI / 4), 1e-6, "the middle of the arc, x");
		CHECK_NEAR(mid.y, 10 * std::sin(PI / 4), 1e-6, "the middle of the arc, y");
		glm::dvec3 low, high;
		p.bounds(low, high);
		CHECK_NEAR(low.x, 0, 1e-6, "bounds, low x"); CHECK_NEAR(high.x, 10, 1e-6, "bounds, high x");
		CHECK_NEAR(low.y, 0, 1e-6, "bounds, low y"); CHECK_NEAR(high.y, 10, 1e-6, "bounds, high y");
		CHECK(p.pieces(0.02) >= 12 && p.pieces(0.02) <= 14, "the pieces which follow the arc at 0.02");
		CHECK(SimPath(glm::dvec3(0), linear(5, 0, 0, 100)).pieces(0.02) == 1, "a line is one piece");

		// a half circle clockwise from (10, 0) to (-10, 0) goes through the bottom
		SimPath half(glm::dvec3(10, 0, 0), arc(-10, 0, 0, 0, 0, 0, true, 600));
		CHECK_NEAR(half.length(), PI * 10, 1e-6, "a half circle");
		CHECK_NEAR(half.at(0.5).y, -10, 1e-6, "clockwise goes through the bottom");
		half.bounds(low, high);
		CHECK_NEAR(low.y, -10, 1e-6, "the bounds hold the lowest point of the arc");
		CHECK_NEAR(high.y, 0, 1e-6, "and not the top");

		// the same points counter-clockwise: the top
		SimPath top(glm::dvec3(10, 0, 0), arc(-10, 0, 0, 0, 0, 0, false, 600));
		CHECK_NEAR(top.at(0.5).y, 10, 1e-6, "counter-clockwise goes through the top");

		// the start and the end are the same point: a full circle
		SimPath circle(glm::dvec3(10, 0, 0), arc(10, 0, 0, 0, 0, 0, false, 600));
		CHECK_NEAR(circle.length(), 2 * PI * 10, 1e-6, "a full circle");
		CHECK_NEAR(circle.at(0.5).x, -10, 1e-6, "the half of the circle is on the other side");

		// a helix
		SimPath helix(glm::dvec3(10, 0, 0), arc(0, 10, -3, 0, 0, 0, false, 600));
		CHECK_NEAR(helix.length(), std::hypot(PI * 10 / 2, 3), 1e-6, "the length of a helix");
		CHECK_NEAR(helix.at(0.5).z, -1.5, 1e-9, "the third axis changes along the arc");

		// the plane XZ (G18): the angle goes from Z to X, so from +X to +Z is clockwise
		SimPath xz(glm::dvec3(10, 0, 0), arc(0, 0, 10, 0, 0, 0, true, 600, SimPlane::XZ));
		CHECK_NEAR(xz.length(), PI * 10 / 2, 1e-6, "an arc in the plane XZ");
		CHECK_NEAR(xz.at(0.5).y, 0, 1e-9, "the other axis does not move");
		CHECK_NEAR(xz.at(0.5).z, 10 * std::sin(PI / 4), 1e-6, "the point is in X and Z");
	}

	std::puts("program: times and bounds");
	{
		SimProgram program;
		number_lines(program, { rapid(100, 0, 0), linear(100, 60, 0, 600), dwell(2.0), arc(40, 60, 0, 70, 60, 0, false, 600), spindle(1) });
		SimSettings settings;
		settings.rapid_speed = 5000;
		program.analyze(settings);

		CHECK_NEAR(program.duration(0), 100.0 / 5000 * 60, 1e-9, "a rapid of 100 mm at 5000 mm/min: 1.2 s");
		CHECK_NEAR(program.duration(1), 6.0, 1e-9, "60 mm at 600 mm/min: 6 s");
		CHECK_NEAR(program.duration(2), 2.0, 1e-9, "a dwell of 2 s");
		CHECK_NEAR(program.duration(3), PI * 30 / 600 * 60, 1e-6, "a half circle of radius 30 at 600 mm/min");
		CHECK_NEAR(program.stats().total_time, 1.2 + 6.0 + 2.0 + PI * 30 / 10.0, 1e-6, "the total time");
		CHECK_NEAR(program.start_time(1), 1.2, 1e-9, "the start time of a command");
		CHECK_NEAR(program.stats().feed_length, 60 + PI * 30, 1e-6, "the length of the feed moves");
		CHECK_NEAR(program.stats().rapid_length, 100, 1e-9, "the length of the rapids");
		CHECK(program.stats().high.x >= 100 && program.stats().low.x <= 0, "the bounds hold the positions");
		CHECK(program.stats().has_cut && program.stats().cut_low.y == 0 && program.stats().cut_low.x == 40, "the box of the moves which cut holds the feed moves and the arc");
		CHECK_NEAR(program.stats().cut_high.y, 90, 1e-6, "the top of the arc is in the bounds");
		CHECK(program.command_at_line(3) == 2 && program.command_at_line(99) == program.size(), "the command of a line");

		// z only: the rapid speed of Z
		SimProgram z;
		z.add(rapid(0, 0, 10));
		SimSettings slow;
		slow.rapid_z_speed = 600;
		z.analyze(slow);
		CHECK_NEAR(z.duration(0), 1.0, 1e-9, "a rapid in Z uses the speed of Z");
	}

	std::puts("player: real time, pause, stop of the program");
	{
		SimProgram program;
		number_lines(program, { rapid(0, 0, 5), linear(60, 0, 5, 600), dwell(2.0), linear(60, 30, 5, 600), stop(false, false), linear(0, 30, 5, 600), stop(false, true), linear(99, 99, 5, 600) });
		SimSettings settings;
		settings.rapid_speed = 6000;
		settings.rapid_z_speed = 6000;
		program.analyze(settings);

		SimPlayer player;
		double moved = 0;
		int resets = 0;
		player.on_move = [&](const SimMove& m) { moved += glm::distance(m.from, m.to); };
		player.on_reset = [&]() { resets++; moved = 0; };
		player.program(&program);
		CHECK(player.state() == SimPlayer::State::Stopped && resets == 1, "the player starts stopped, the program reset it");

		player.play();
		player.update(0.0);
		// the rapid is 5 mm at 6000 mm/min: 0.05 s; then the feed move of 60 mm takes 6 s
		player.update(1.0);
		CHECK(player.playing(), "playing");
		CHECK_NEAR(player.position().x, 9.5, 0.05, "after 1 s: 0.05 s of rapid, then 0.95 s at 10 mm/s");
		CHECK_NEAR(player.elapsed(), 1.0, 0.001, "the time of the machine");
		CHECK(player.line() == 2, "the line which is played");

		player.pause();
		double x = player.position().x;
		player.update(5.0);
		CHECK_NEAR(player.position().x, x, 1e-12, "a pause does not move the tool");
		player.play();
		player.update(5.2);
		CHECK_NEAR(player.position().x, 60, 1e-6, "the end of the first feed move");
		// the dwell is 2 s: the position does not change
		player.update(1.0);
		CHECK_NEAR(player.position().x, 60, 1e-9, "a dwell does not move the tool");
		CHECK_NEAR(player.position().y, 0, 1e-9, "a dwell does not move the tool, y");
		player.update(1.5);
		CHECK(player.position().y > 0, "after the dwell the tool moves again");
		player.update(10.0);
		CHECK(player.state() == SimPlayer::State::Paused && player.held_by_program(), "the stop M0 of the program holds the player");
		CHECK_NEAR(player.position().y, 30, 1e-6, "the tool is at the end of the move before the stop");
		player.update(10.0);
		CHECK_NEAR(player.position().x, 60, 1e-9, "it does not move while it is held");
		player.play();
		CHECK(!player.held_by_program(), "the button goes on");
		player.update(100.0);
		CHECK(player.state() == SimPlayer::State::Finished, "M30 ends the program");
		CHECK_NEAR(player.position().x, 0, 1e-6, "what follows the end is not played");
		CHECK_NEAR(player.fraction(), 1.0, 1e-9, "all of it was played");
		CHECK_NEAR(moved, 5 + 60 + 30 + 60, 1e-6, "the pieces of path which were reported add up to the path");

		// again from the start
		player.play();
		CHECK(player.state() == SimPlayer::State::Playing && resets >= 2 && player.index() == 0, "play after the end begins again");
	}

	std::puts("player: M1 as a setting, full speed, going to");
	{
		SimProgram program;
		number_lines(program, { rapid(0, 0, 5), linear(10, 0, 5, 600), stop(true, false), linear(20, 0, 5, 600), dwell(5.0), linear(30, 0, 5, 600), stop(false, false), linear(40, 0, 5, 600) });
		SimSettings settings;
		program.analyze(settings);

		SimPlayer player;
		program.analyze(settings);
		player.program(&program);
		player.play();
		player.update(100.0);
		CHECK(player.held_by_program() && player.index() > 0, "M1 stops when the optional stop is on");

		settings.optional_stop = false;
		program.analyze(settings);
		player.program(&program);
		player.play();
		player.update(100.0);
		// M1 is skipped, the dwell is waited for (real time), then M0 holds
		CHECK(player.held_by_program(), "M0 holds, M1 is skipped");
		CHECK_NEAR(player.position().x, 30, 1e-6, "the tool went to the end of the move before M0");

		// full speed: no wait, no stop
		player.program(&program);
		player.full_speed(true);
		player.play();
		player.update(0.0, 1.0);
		CHECK(player.state() == SimPlayer::State::Finished, "full speed skips the dwell and the stops");
		CHECK_NEAR(player.position().x, 40, 1e-6, "and plays everything");
		CHECK_NEAR(player.elapsed(), program.stats().total_time, 1e-9, "the time is still the one of the machine");
		player.full_speed(false);

		// going to the end
		settings.optional_stop = true;
		program.analyze(settings);
		player.program(&program);
		player.go_to_end();
		CHECK(player.going(), "going to the end");
		player.update(0.0, 1.0);
		CHECK(player.state() == SimPlayer::State::Finished && !player.going(), "the end is reached, even with stops");
		CHECK_NEAR(player.position().x, 40, 1e-6, "at the end of the program");

		// going to a line: the player pauses there, the tool is where it is after the commands before
		int resets = 0;
		player.on_reset = [&]() { resets++; };
		player.go_to_line(4);
		player.update(0.0, 1.0);
		CHECK(player.state() == SimPlayer::State::Paused, "paused at the line");
		CHECK(resets == 1, "going back resets the material");
		CHECK_NEAR(player.position().x, 10, 1e-6, "the tool is at the end of what comes before the line 4");
		CHECK(player.index() == 3, "the next command is the one of the line");
		player.go_to_line(6);
		player.update(0.0, 1.0);
		CHECK(resets == 1 && player.index() == 5, "going forward does not reset");
		CHECK_NEAR(player.position().x, 20, 1e-6, "the position at the line 6");

		// stepping
		player.go_to_line(1);
		player.update(0.0, 1.0);
		player.step_command();
		player.update(0.0, 1.0);
		CHECK(player.state() == SimPlayer::State::Paused, "a step pauses");
		CHECK_NEAR(player.position().x, 0, 1e-6, "the first step is the first move");
		player.step_command();
		player.update(0.0, 1.0);
		CHECK_NEAR(player.position().x, 10, 1e-6, "the second step is the second move");
		player.step_line();
		player.update(0.0, 1.0);
		CHECK(player.state() == SimPlayer::State::Paused && player.index() >= 3, "a step by line goes past the stop");
		player.stop();
		CHECK(player.state() == SimPlayer::State::Stopped && player.index() == 0 && player.position().x == 0, "stop goes back to the start");

		// the speed
		SimProgram fast;
		number_lines(fast, { linear(100, 0, 0, 600) });
		fast.analyze(SimSettings());
		player.program(&fast);
		player.set_speed(10.0);
		player.play();
		player.update(1.0);
		CHECK_NEAR(player.position().x, 100, 1e-6, "10 times the speed: 10 s of the machine in 1 s");
		CHECK(player.state() == SimPlayer::State::Finished, "the program ended");
	}

	std::puts("player: the pieces of an arc and the state of the machine");
	{
		SimProgram program;
		number_lines(program, { spindle(1), rapid(10, 0, 0), arc(0, 10, 0, 0, 0, 0, false, 600), spindle(0) });
		SimSettings settings;
		settings.arc_tolerance = 0.02;
		program.analyze(settings);

		SimPlayer player;
		std::vector<SimMove> moves;
		player.on_move = [&](const SimMove& m) { moves.push_back(m); };
		player.program(&program);
		player.full_speed(true);
		player.play();
		player.update(0, 1.0);

		CHECK(moves.size() > 10, "an arc is reported by pieces");
		double longest = 0, sum = 0;
		bool on_circle = true, spindle_on = true;
		for (size_t i = 1; i < moves.size(); i++)
		{
			longest = std::max(longest, glm::distance(moves[i].from, moves[i].to));
			sum += glm::distance(moves[i].from, moves[i].to);
			on_circle = on_circle && std::fabs(glm::length(glm::dvec2(moves[i].to)) - 10.0) < 1e-6;
			spindle_on = spindle_on && moves[i].spindle == 1;
		}
		CHECK(on_circle, "every piece ends on the circle");
		CHECK(spindle_on, "the pieces know that the spindle is on");
		CHECK_NEAR(sum, PI * 10 / 2, 0.02, "the pieces follow the arc to the chord");
		CHECK(moves.front().rapid, "the first move is a rapid");
		CHECK(moves.back().rapid == false, "the arc is not a rapid");
		CHECK(player.spindle() == 0, "the spindle is stopped at the end");
	}

	std::puts("material: tools");
	{
		SimMaterial m;
		m.setup(glm::dvec2(-20, -20), glm::dvec2(20, 20), 0.0, -5.0, 0.25);
		CHECK(m.width() == 161 && m.height() == 161, "the number of columns");
		CHECK_NEAR(m.height_at(0, 0), 0.0, 1e-9, "the block is flat at the top");

		SimTool flat;
		flat.shape = SimToolShape::Flat;
		flat.radius = 2.0;
		m.cut(glm::dvec3(-10, 0, -1), glm::dvec3(10, 0, -1), flat);
		CHECK_NEAR(m.height_at(0, 0), -1.0, 1e-6, "a flat tool cuts at the height of its tip");
		CHECK_NEAR(m.height_at(0, 1.9), -1.0, 1e-6, "inside of the radius");
		CHECK_NEAR(m.height_at(0, 2.3), 0.0, 1e-6, "outside of the radius");
		CHECK_NEAR(m.height_at(11.5, 0), -1.0, 1e-6, "the rounded end of the cut");
		CHECK_NEAR(m.height_at(12.6, 0), 0.0, 1e-6, "past the end");
		// stadium: length * width + circle (a finer grid, the columns on the border make the error)
		SimMaterial fine;
		fine.setup(glm::dvec2(-20, -20), glm::dvec2(20, 20), 0.0, -5.0, 0.05);
		fine.cut(glm::dvec3(-10, 0, -1), glm::dvec3(10, 0, -1), flat);
		double expected = 20 * 4 + PI * 4;
		CHECK_NEAR(fine.removed_volume(), expected, expected * 0.02, "the volume taken away is the one of a stadium 1 mm deep");

		int x0, y0, x1, y1;
		CHECK(m.take_dirty(x0, y0, x1, y1), "a zone changed");
		CHECK(x0 <= 40 && x1 >= 120 && y0 <= 72 && y1 >= 88, "the zone holds the cut");
		CHECK(!m.take_dirty(x0, y0, x1, y1), "and it is given once");

		// a second cut which does not go deeper changes nothing
		m.cut(glm::dvec3(-5, 0, -0.5), glm::dvec3(5, 0, -0.5), flat);
		CHECK(!m.take_dirty(x0, y0, x1, y1), "a cut above the surface changes nothing");

		// a plunge: the lowest height of the move is cut
		m.reset();
		m.cut(glm::dvec3(5, 5, 0), glm::dvec3(5, 5, -3), flat);
		CHECK_NEAR(m.height_at(5, 5), -3.0, 1e-6, "a plunge goes down to the lowest point");

		// a ramp: the cut is deeper at the end
		m.reset();
		m.cut(glm::dvec3(-10, 10, 0), glm::dvec3(10, 10, -4), flat);
		// the slope is 0.2 mm for 1 mm: a column is reached by the tool 2 mm (its radius) further, which is 0.4 mm deeper
		CHECK_NEAR(m.height_at(-10, 10), -0.4, 0.05, "a ramp: the column at its start is cut by the tool which goes on 2 mm");
		CHECK_NEAR(m.height_at(0, 10), -2.4, 0.05, "the middle of a ramp");
		CHECK_NEAR(m.height_at(10, 10), -4.0, 1e-3, "the end of a ramp is at its depth");

		// ball: the surface of the tool is a sphere
		m.reset();
		SimTool ball;
		ball.shape = SimToolShape::Ball;
		ball.radius = 2.0;
		m.cut(glm::dvec3(0, 0, -2), glm::dvec3(0, 0, -2), ball);
		CHECK_NEAR(m.height_at(0, 0), -2.0, 1e-6, "the tip of a ball tool");
		CHECK_NEAR(m.height_at(1, 0), -2.0 + (2.0 - std::sqrt(3.0)), 0.01, "a ball tool at 1 mm of its axis");
		CHECK_NEAR(m.height_at(2, 0), 0.0, 0.01, "its radius reaches the surface");

		// cone: a V cutter, 90 degrees
		m.reset();
		SimTool cone;
		cone.shape = SimToolShape::Cone;
		cone.radius = 3.0;
		cone.angle = 90.0;
		cone.small_radius = 0.0;
		m.cut(glm::dvec3(0, 0, -2), glm::dvec3(0, 0, -2), cone);
		CHECK_NEAR(m.height_at(1, 0), -1.0, 0.01, "a V cutter of 90 degrees: the wall rises 1 mm for 1 mm");
		CHECK_NEAR(m.height_at(2, 0), 0.0, 0.01, "and reaches the surface at its depth");

		// torus: a flat tool with a rounded corner
		m.reset();
		SimTool torus;
		torus.shape = SimToolShape::Torus;
		torus.radius = 3.0;
		torus.small_radius = 1.0;
		m.cut(glm::dvec3(0, 0, -2), glm::dvec3(0, 0, -2), torus);
		CHECK_NEAR(m.height_at(1.5, 0), -2.0, 1e-6, "the flat bottom of a torus tool");
		CHECK(m.height_at(2.9, 0) > -1.5 && m.height_at(2.9, 0) < -0.7, "the rounded corner rises toward the radius");

		// a torch cuts through
		m.reset();
		SimTool torch;
		torch.shape = SimToolShape::Plasma;
		torch.radius = 0.8;
		m.cut(glm::dvec3(-10, 0, 0), glm::dvec3(10, 0, 0), torch);
		CHECK_NEAR(m.height_at(0, 0), -5.0, 1e-6, "a torch cuts through the block");
		CHECK_NEAR(m.height_at(0, 0.6), -5.0, 1e-6, "with the width of its kerf");
		CHECK_NEAR(m.height_at(0, 1.5), 0.0, 1e-6, "and no more");

		// the grid
		double step = SimMaterial::choose_step(100, 100, 3.0, 15.0, 4000000);
		CHECK_NEAR(step, 0.45, 1e-9, "the step is 15 percent of the radius");
		double big = SimMaterial::choose_step(10000, 10000, 3.0, 15.0, 4000000);
		CHECK(big > 4.0 && (10000 / big + 1) * (10000 / big + 1) <= 4000000 * 1.001, "a larger step when there would be too many columns");
	}

	std::puts("loader");
	{
		FakeTranslator fake;
		SimProgram program;
		SimLoader loader;
		std::string text;
		for (int i = 1; i <= 100; i++)
			text += std::to_string(i) + "\r\n";
		CHECK(SimLoader::split_lines("a\r\nb\n\nc\n").size() == 4, "the lines of a text: a, b, an empty one, c");
		CHECK(SimLoader::split_lines("").empty(), "an empty text has no line");
		CHECK(loader.begin(&fake, &program, text), "the loader begins");
		int steps = 0;
		while (!loader.step(0.0) && steps < 1000)
			steps++;
		CHECK(steps > 2 && steps < 20, "a long program is translated by several slices");
		CHECK(program.size() == 100 && loader.done() == 100 && loader.progress() == 1.0, "all the lines were translated");
		CHECK(program.at(41).line == 42 && program.at(41).target.x == 42, "the commands keep the number of their line");
		CHECK(fake.started == 1 && fake.closed >= 1, "the translator was started and closed");

		FakeTranslator cancelled;
		SimProgram p2;
		SimLoader l2;
		l2.begin(&cancelled, &p2, text);
		l2.step(0.0);
		l2.cancel();
		CHECK(l2.cancelled() && !l2.active() && p2.size() < 100 && cancelled.closed == 1, "cancel stops the translation and closes the translator");

		FakeTranslator bad;
		bad.ok = false;
		SimLoader l3;
		SimProgram p3;
		CHECK(!l3.begin(&bad, &p3, text) && l3.failed() && l3.error().find("does not start") != std::string::npos, "a translator which does not start gives its error");
	}

	std::puts("translator in Python");
	{
		std::filesystem::path simulator = std::filesystem::path(SIMULATOR_DIR);
		Script::initialize_python();
		Script::add_module_path(simulator.string());

		// the extensions which a translator reads (its method extensions(), optional): lower case, without the dot
		{
			PythonTranslator gcode_translator("gcode"), hpgl_translator("hpgl");
			std::vector<std::string> gcode_extensions = gcode_translator.extensions(), hpgl_extensions = hpgl_translator.extensions();
			CHECK(std::find(gcode_extensions.begin(), gcode_extensions.end(), "nc") != gcode_extensions.end(), "gcode reads .nc");
			CHECK(std::find(hpgl_extensions.begin(), hpgl_extensions.end(), "plt") != hpgl_extensions.end(), "hpgl reads .plt");
			CHECK(std::find(gcode_extensions.begin(), gcode_extensions.end(), "plt") == gcode_extensions.end(), "gcode does not read .plt");
			PythonTranslator missing("module_which_does_not_exist");
			CHECK(missing.extensions().empty(), "a module which is missing reads nothing");
		}

		// the module of a test: a translator which fails on the line 3, one which fails to start
		std::filesystem::path temp = std::filesystem::temp_directory_path() / "opp_sim_test";
		std::filesystem::create_directories(temp);
		{
			std::ofstream f(temp / "failing.py");
			f << "class Translator:\n"
				 "    def start(self, options):\n"
				 "        return {'unit': 'inch'}\n"
				 "    def next(self, line):\n"
				 "        if line == 'boom':\n"
				 "            raise ValueError('boom in the translator')\n"
				 "        if line == 'weird':\n"
				 "            return [{'cmd': 'frobnicate', 'x': 1}, 5]\n"
				 "        return [{'cmd': 'rapid', 'x': 1.0, 'y': 2, 'z': 3}]\n"
				 "    def close(self):\n"
				 "        pass\n";
			std::ofstream g(temp / "refusing.py");
			g << "class Translator:\n"
				 "    def start(self, options):\n"
				 "        raise RuntimeError('no machine')\n"
				 "    def next(self, line):\n"
				 "        return []\n"
				 "    def close(self):\n"
				 "        pass\n";
		}
		{
			// a library and a module with another class are not translators
			std::ofstream l(temp / "library.py");
			l << "def helper():\n    return 1\n";
			std::ofstream o(temp / "other.py");
			o << "class Other:\n    def next(self, line):\n        return []\n";
		}
		Script::add_module_path(temp.string());

		std::vector<std::string> found = PythonTranslator::list(temp.string());
		CHECK(found.size() == 2 && found[0] == "failing" && found[1] == "refusing", "the translators of a folder are the modules which define a class Translator");
		std::vector<std::string> delivered = PythonTranslator::list(simulator.string());
		CHECK(std::find(delivered.begin(), delivered.end(), "gcode") != delivered.end(), "the generic G-code translator is delivered");
		CHECK(PythonTranslator::list((temp / "nothing").string()).empty(), "a folder which does not exist has no translator");

		// from a program to the material
		PythonTranslator gcode("gcode");
		SimProgram program;
		SimLoader loader;
		std::string source =
			"G21 G90\n"
			"T1 M6\n"
			"S10000 M3\n"
			"G0 X0 Y0 Z5\n"
			"G0 X-10 Y0\n"
			"G1 Z-2 F300\n"
			"G1 X10 F600\n"
			"G2 X10 Y0 I-10 J0\n"
			"G0 Z5\n"
			"M5\n"
			"M30\n";
		std::map<std::string, double> options;
		bool begun = loader.begin(&gcode, &program, source, options);
		CHECK(begun, begun ? "the python translator starts" : loader.error().c_str());
		while (!loader.step(1.0)) {}
		CHECK(gcode.unit() == "mm", "the translator says the unit of the program");
		CHECK(program.size() == 11 - 1 + 0 || program.size() >= 9, "the program has its commands");

		SimSettings settings;
		program.analyze(settings);
		CHECK(program.stats().errors == 0 && program.stats().warnings == 0, "no message for a correct program");
		CHECK(program.stats().has_cut, "there are moves which cut");

		SimMaterial material;
		SimTool tool;
		tool.shape = SimToolShape::Flat;
		tool.radius = 2.0;
		material.setup(glm::dvec2(-15, -15), glm::dvec2(15, 15), 0.0, -3.0, 0.25);

		SimPlayer player;
		player.on_move = [&](const SimMove& m) { material.cut(m.from, m.to, tool); };
		player.on_reset = [&]() { material.reset(); };
		player.program(&program);
		player.go_to_end();
		player.update(0.0, 5.0);
		CHECK(player.state() == SimPlayer::State::Finished, "the program was played to its end");
		CHECK_NEAR(material.height_at(0, 0), -2.0, 1e-6, "the line is cut 2 mm deep");
		CHECK_NEAR(material.height_at(0, 1.5), -2.0, 1e-6, "with the width of the tool");
		CHECK_NEAR(material.height_at(0, 10.0), -2.0, 1e-6, "and the circle goes through (0, 10) (a circle of radius 10 around the origin: its top)");
		CHECK_NEAR(material.height_at(0, 5.0), 0.0, 1e-6, "inside of the circle there is no cut");
		CHECK(player.spindle() == 0 && player.tool() == 1, "the tool and the spindle");

		// an exception of the translator: the translation goes on, the error has its line
		PythonTranslator failing("failing");
		SimProgram p2;
		SimLoader l2;
		CHECK(l2.begin(&failing, &p2, "ok\nboom\nweird\nok\n"), "a translator with a failing line starts");
		while (!l2.step(1.0)) {}
		CHECK(failing.unit() == "inch", "its unit");
		CHECK(p2.size() == 5, "every line was translated: 1, the error, the custom command, the not-a-command error, 1");
		CHECK(p2.at(1).type == SimCommandType::Message && p2.at(1).level == SimLevel::Error && p2.at(1).line == 2, "the exception is an error on its line");
		CHECK(p2.at(1).text.find("boom in the translator") != std::string::npos && p2.at(1).text.find("ValueError") != std::string::npos, "with the text of the exception");
		CHECK(p2.at(2).type == SimCommandType::Custom && p2.at(2).text == "frobnicate", "an unknown command is kept as a custom one");
		CHECK(p2.at(3).type == SimCommandType::Message && p2.at(3).level == SimLevel::Error, "something which is not a command is an error");
		CHECK(p2.at(4).type == SimCommandType::Rapid && p2.at(4).target.y == 2 && p2.at(4).line == 4, "the program goes on after the error");
		p2.analyze(settings);
		CHECK(p2.stats().errors == 2 && p2.stats().unknown == 1, "the errors and the unknown commands are counted");

		// a translator which refuses to start, and a module which does not exist
		PythonTranslator refusing("refusing");
		SimProgram p3;
		SimLoader l3;
		CHECK(!l3.begin(&refusing, &p3, "x\n") && l3.error().find("no machine") != std::string::npos, "an exception in start() is the error of the loader");
		PythonTranslator missing("no_such_translator");
		SimLoader l4;
		CHECK(!l4.begin(&missing, &p3, "x\n") && !l4.error().empty(), "a missing module is an error");
		PythonTranslator other("other");
		SimLoader l5;
		CHECK(!l5.begin(&other, &p3, "x\n") && l5.error().find("Translator") != std::string::npos, "a module without the class Translator says it");

		// the encoding of a line: UTF-8 is kept, an 8 bits text (Windows-1252, Latin-1) is converted
		CHECK(SimTranslator::to_utf8("G1 X1") == "G1 X1", "ASCII is not changed");
		CHECK(SimTranslator::to_utf8("(caf\xC3\xA9)") == "(caf\xC3\xA9)", "valid UTF-8 is not changed");
		CHECK(SimTranslator::to_utf8("(caf\xE9)") == "(caf\xC3\xA9)", "a text in Latin-1 becomes UTF-8");
		CHECK(SimTranslator::to_utf8("(\xC3)") == "(\xC3\x83)", "a lone byte is not a UTF-8 sequence");
		PythonTranslator accents("gcode");
		SimProgram p6;
		SimLoader l6;
		l6.begin(&accents, &p6, "G21\n(caf\xE9 \xE0 la carte)\n(MSG, \xE9t\xE9)\nG1 X1 F100\n");
		while (!l6.step(1.0)) {}
		p6.analyze(settings);
		CHECK(p6.stats().errors == 0 && p6.size() == 2, "a program in Windows-1252 is translated without error");
		bool accents_right = false;
		for (const SimCommand& c : p6.commands())
			if (c.type == SimCommandType::Message && c.text == "\xC3\xA9t\xC3\xA9")
				accents_right = true;
		CHECK(accents_right, "and the accents of a message are right");

		std::filesystem::remove_all(temp);
		Script::finalize_python();
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
