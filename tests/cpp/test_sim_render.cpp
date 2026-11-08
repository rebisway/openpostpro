// SIMULATOR RENDER TEST - draws the simulator on a real (hidden) OpenGL 3.3 core context and checks the picture.
//
// What for:
//   The shaders and the textures of the simulator can be wrong without any error: a block which is black, upside down, flat, or
//   which does not show the cut. This test plays a small program in the core of the simulator, draws the result with the renderer
//   of the application, reads the pixels back and saves the picture (sim_render.png, in the folder given by the environment variable
//   SIM_RENDER_OUT, or the temporary folder) so that it can be looked at.
//
// What is checked: no OpenGL error; the shaders sim and line3d link; the block is drawn (its top color is where the camera looks at it);
//   a pocket is darker than the top and at the place where the program put it (the points are projected with the matrices of the
//   camera and the pixel is read); the paths and the tool are drawn with their colors; the sides of the block are seen; a block cut
//   through by a torch shows the background where the torch went; the map is updated by zones (only the columns which changed).
// A machine without OpenGL 3.3 is reported as skipped.
//
// Build: tests/cpp/build_sim_render.bat   Run: tests/cpp/test_sim_render.exe   (exit code 0 = all checks pass)
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "renderer_opengl.h"
#include <sim_view.h>
#include <sim_model_grid.h>
#include <sim_model_tiles.h>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <set>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifndef SHADER_DIR
#error SHADER_DIR must be defined by the build script
#endif

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

static const int W = 900, H = 640;

static unsigned int gl_errors()
{
	unsigned int count = 0;
	while (glGetError() != GL_NO_ERROR && count < 100)
		count++;
	return count;
}

struct Picture
{
	std::vector<unsigned char> rgba;		// from the top row
	glm::vec3 at(int x, int y) const
	{
		x = std::max(0, std::min(W - 1, x));
		y = std::max(0, std::min(H - 1, y));
		const unsigned char* p = &rgba[((size_t)y * W + x) * 4];
		return glm::vec3(p[0], p[1], p[2]) / 255.0f;
	}
};

static Picture read_picture()
{
	Picture picture;
	std::vector<unsigned char> raw((size_t)W * H * 4);
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());
	picture.rgba.resize(raw.size());
	for (int y = 0; y < H; y++)
		std::copy(raw.begin() + (size_t)(H - 1 - y) * W * 4, raw.begin() + (size_t)(H - y) * W * 4, picture.rgba.begin() + (size_t)y * W * 4);
	return picture;
}

// the pixel of a point of the scene
static glm::ivec2 project(SimView& view, glm::vec3 p)
{
	glm::vec4 clip = view.camera().projection() * view.camera().modelview() * glm::vec4(p, 1.0f);
	glm::vec3 ndc = glm::vec3(clip) / clip.w;
	return glm::ivec2((int)((ndc.x * 0.5f + 0.5f) * W), (int)((1.0f - (ndc.y * 0.5f + 0.5f)) * H));
}

static bool near_color(glm::vec3 a, glm::vec3 b, float tolerance)
{
	return glm::distance(a, b) <= tolerance;
}

// a pocket 30 x 20 with rounded corners (2 mm deep), a hole drilled in a corner, and what comes between them
static void program(SimProgram& p)
{
	auto add = [&](SimCommand c) { c.line = (int)p.size() + 1; p.add(c); };
	auto rapid = [&](double x, double y, double z) { SimCommand c; c.type = SimCommandType::Rapid; c.target = glm::dvec3(x, y, z); add(c); };
	auto line = [&](double x, double y, double z) { SimCommand c; c.type = SimCommandType::Linear; c.target = glm::dvec3(x, y, z); c.feed = 800; add(c); };
	auto arc = [&](double x, double y, double cx, double cy, bool cw) { SimCommand c; c.type = SimCommandType::Arc; c.target = glm::dvec3(x, y, -2); c.center = glm::dvec3(cx, cy, -2); c.cw = cw; c.feed = 800; add(c); };

	rapid(-12, -4, 5);
	line(-12, -4, -2);
	// zigzag of the pocket, the tool is 3 mm in diameter
	for (int i = 0; i < 6; i++)
	{
		double y = -4 + i * 1.6;
		line(i % 2 == 0 ? 12 : -12, y, -2);
		line(i % 2 == 0 ? 12 : -12, y + 1.6, -2);
	}
	rapid(0, 0, 5);
	// a circle cut around, outside of the pocket
	rapid(30, 0, 5);
	line(30, 0, -1);
	arc(30, 0, 22, 0, false);
	rapid(30, 0, 5);
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);

	if (!glfwInit())
	{
		std::puts("skipped: glfwInit failed");
		return 0;
	}
	OpenGlRenderer renderer;
	renderer.initialize_GLFW();
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(W, H, "sim render test", nullptr, nullptr);
	if (window == nullptr)
	{
		std::puts("skipped: no OpenGL 3.3 core context on this machine");
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	if (gl3wInit() != 0)
	{
		std::puts("skipped: gl3w could not load OpenGL");
		glfwTerminate();
		return 0;
	}
	std::printf("OpenGL %s\n", glGetString(GL_VERSION));
	glViewport(0, 0, W, H);

	std::puts("programs");
	for (const char* name : { "sim", "simtile", "line3d" })
	{
		renderer.load_program(name, SHADER_DIR);
		renderer.use_program(name);
		GLint current = 0, linked = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &current);
		glGetProgramiv(current, GL_LINK_STATUS, &linked);
		CHECK(current != 0 && linked == GL_TRUE, "the program compiles and links");
	}
	CHECK(gl_errors() == 0, "no OpenGL error after the programs");

	std::puts("a pocket in a block");
	SimProgram prog;
	program(prog);
	SimSettings settings;
	prog.analyze(settings);

	SimTool tool;
	tool.shape = SimToolShape::Flat;
	tool.radius = 1.5;
	tool.length = 20;

	SimModelGrid grid;
	SimBlock block;
	block.low = glm::dvec2(-30, -20);
	block.high = glm::dvec2(40, 20);
	block.top = 0.0;
	block.bottom = -6.0;
	block.tool_radius = tool.radius;
	grid.setup(block);
	SimMaterial& material = grid.material();

	SimView view(&renderer);
	view.set_model(grid);
	CHECK(gl_errors() == 0, "no OpenGL error after the texture");

	// before the program: nothing is cut, the block is flat
	renderer.clear(view.background.r, view.background.g, view.background.b, 1.0f);
	view.draw(W, H);
	CHECK(gl_errors() == 0, "no OpenGL error after drawing");
	Picture before = read_picture();
	glm::ivec2 top_pixel = project(view, glm::vec3(-20, 12, 0));
	glm::ivec2 pocket_pixel = project(view, glm::vec3(0, 0, -2));
	glm::vec3 top_before = before.at(top_pixel.x, top_pixel.y);
	glm::vec3 pocket_before = before.at(pocket_pixel.x, pocket_pixel.y);
	CHECK(!near_color(top_before, glm::vec3(view.background), 0.08f), "the block is drawn (its top is not the background)");
	CHECK(near_color(top_before, pocket_before, 0.12f), "before the machining the whole top has the same look");

	// play the program: the player cuts the material and the view draws the paths
	SimPlayer player;
	player.on_move = [&](const SimMove& m) { material.cut(m.from, m.to, tool); view.add_move(m); };
	player.on_reset = [&]() { material.reset(); view.clear_paths(); };
	player.program(&prog);
	player.go_to_end();
	player.update(0, 5.0);
	CHECK(player.state() == SimPlayer::State::Finished, "the program was played");
	CHECK(view.path_vertices() > 100, "the paths were collected");

	// the reset of the player marked all the map, then the whole map is sent once; after that only a cut changes a zone
	view.upload();
	int x0, y0, x1, y1;
	CHECK(!material.take_dirty(x0, y0, x1, y1), "everything was sent");
	material.cut(glm::dvec3(35, 10, -1), glm::dvec3(36, 10, -1), tool);
	CHECK(material.take_dirty(x0, y0, x1, y1) && x1 - x0 < material.width() / 4 && y1 - y0 < material.height() / 4, "a small cut changes a small zone of the map");
	material.cut(glm::dvec3(35, 10, -1), glm::dvec3(36, 10, -1), tool);	// the same cut: nothing changes, nothing to send
	view.upload();
	view.set_tool(tool, player.position());

	renderer.clear(view.background.r, view.background.g, view.background.b, 1.0f);
	view.draw(W, H);
	CHECK(gl_errors() == 0, "no OpenGL error after the machining");
	Picture after = read_picture();

	glm::vec3 top_after = after.at(top_pixel.x, top_pixel.y);
	glm::vec3 pocket_after = after.at(pocket_pixel.x, pocket_pixel.y);
	std::printf("  top %g %g %g, pocket %g %g %g\n", top_after.r, top_after.g, top_after.b, pocket_after.r, pocket_after.g, pocket_after.b);
	CHECK(near_color(top_after, top_before, 0.08f), "the top which was not touched is the same");
	CHECK(!near_color(pocket_after, top_after, 0.05f), "the pocket does not look like the top");
	CHECK(pocket_after.r < top_after.r, "the pocket is darker than the top");

	// the paths and the tool: pixels of the colors of the lines exist
	auto count_color = [&](const Picture& pic, glm::vec4 c, float tolerance) {
		int n = 0;
		for (int y = 0; y < H; y += 1)
			for (int x = 0; x < W; x += 1)
				if (near_color(pic.at(x, y), glm::vec3(c), tolerance))
					n++;
		return n;
	};
	CHECK(count_color(after, view.feed_color, 0.05f) > 40, "the paths of the feed moves are orange");
	CHECK(count_color(after, view.rapid_color, 0.05f) > 10, "the rapid moves are drawn");
	CHECK(count_color(after, view.tool_color, 0.05f) > 20, "the tool is drawn");
	CHECK(count_color(before, view.feed_color, 0.05f) == 0, "there is no path before the program");

	// the sides of the block: a point of the front side is drawn with the color of the sides (lit)
	glm::ivec2 side_pixel = project(view, glm::vec3(0, -20, -3));
	glm::vec3 side = after.at(side_pixel.x, side_pixel.y);
	CHECK(!near_color(side, glm::vec3(view.background), 0.05f) && !near_color(side, glm::vec3(view.feed_color), 0.1f), "the front side of the block is drawn");

	// the paths can be hidden
	view.show_feed = false;
	view.show_rapid = false;
	view.show_tool = false;
	renderer.clear(view.background.r, view.background.g, view.background.b, 1.0f);
	view.draw(W, H);
	Picture bare = read_picture();
	CHECK(count_color(bare, view.feed_color, 0.05f) == 0 && count_color(bare, view.tool_color, 0.05f) == 0, "the paths and the tool can be hidden");
	view.show_feed = view.show_rapid = view.show_tool = true;

	// the views from the top: the pocket is where the program cut it
	view.camera().view(0);
	view.fit();
	view.show_axes = false;
	renderer.clear(view.background.r, view.background.g, view.background.b, 1.0f);
	view.draw(W, H);
	Picture top = read_picture();
	glm::ivec2 in_pocket = project(view, glm::vec3(0, 0, -2));
	glm::ivec2 out_pocket = project(view, glm::vec3(-20, 12, 0));
	CHECK(!near_color(top.at(in_pocket.x, in_pocket.y), top.at(out_pocket.x, out_pocket.y), 0.04f), "seen from above the pocket is not like the top");
	glm::ivec2 hole_edge_a = project(view, glm::vec3(0, 6.0f, 0)), hole_edge_b = project(view, glm::vec3(0, 3.0f, 0));
	CHECK(near_color(top.at(hole_edge_a.x, hole_edge_a.y), top.at(out_pocket.x, out_pocket.y), 0.06f), "the zone between the pocket and the circle is not cut");
	(void)hole_edge_b;

	// the picture, to look at
	std::filesystem::path folder = std::getenv("SIM_RENDER_OUT") ? std::filesystem::path(std::getenv("SIM_RENDER_OUT")) : std::filesystem::temp_directory_path();
	std::filesystem::create_directories(folder);
	std::vector<unsigned char> flat = top.rgba;
	for (size_t i = 3; i < flat.size(); i += 4) flat[i] = 255;
	stbi_write_png((folder / "sim_render_top.png").string().c_str(), W, H, 4, flat.data(), W * 4);
	flat = after.rgba;
	for (size_t i = 3; i < flat.size(); i += 4) flat[i] = 255;
	stbi_write_png((folder / "sim_render.png").string().c_str(), W, H, 4, flat.data(), W * 4);

	std::puts("the same program with the model of tiles");
	{
		SimModelTiles tiles;
		tiles.setup(block);
		SimView tile_view(&renderer);
		tile_view.set_model(tiles);
		CHECK(gl_errors() == 0, "no OpenGL error with the tiles");
		SimPlayer tile_player;
		tile_player.on_move = [&](const SimMove& m) { tiles.cut(m.from, m.to, tool); tile_view.add_move(m); };
		tile_player.on_reset = [&]() { tiles.reset(); tile_view.clear_paths(); };
		tile_player.program(&prog);
		tile_player.go_to_end();
		tile_player.update(0, 5.0);
		tile_view.upload();
		CHECK(gl_errors() == 0, "no OpenGL error after sending the tiles");
		tile_view.camera().view(0);
		tile_view.fit();
		tile_view.show_axes = false;
		tile_view.show_feed = tile_view.show_rapid = tile_view.show_tool = false;		// the paths are not at the same pixels (the framing is not the same)
		renderer.clear(tile_view.background.r, tile_view.background.g, tile_view.background.b, 1.0f);
		tile_view.draw(W, H);
		CHECK(gl_errors() == 0, "no OpenGL error after drawing the tiles");
		Picture tp = read_picture();
		std::printf("  %s\n", tiles.describe().c_str());

		// the same points of the scene look the same as with the grid
		auto same = [&](glm::vec3 world, const char* what) {
			glm::ivec2 a = project(view, world), b = project(tile_view, world);
			glm::vec3 ca = top.at(a.x, a.y), cb = tp.at(b.x, b.y);
			if (!near_color(ca, cb, 0.06f))
				std::printf("  %s: grid %g %g %g, tiles %g %g %g\n", what, ca.r, ca.g, ca.b, cb.r, cb.g, cb.b);
			CHECK(near_color(ca, cb, 0.06f), what);
		};
		same(glm::vec3(0, 0, -2), "tiles: the bottom of the pocket");
		same(glm::vec3(-20, 12, 0), "tiles: the top which was not touched");
		same(glm::vec3(0, 6.0f, 0), "tiles: between the pocket and the circle");
		same(glm::vec3(26, 0, -1), "tiles: the circle cut around");
		glm::ivec2 in_tiles = project(tile_view, glm::vec3(0, 0, -2)), out_tiles = project(tile_view, glm::vec3(-20, 12, 0));
		CHECK(!near_color(tp.at(in_tiles.x, in_tiles.y), tp.at(out_tiles.x, out_tiles.y), 0.04f), "tiles: seen from above the pocket is not like the top");
		CHECK(std::fabs(tiles.removed_volume() - grid.removed_volume()) / grid.removed_volume() < 0.03, "tiles: the volume is the one of the grid");

		// a second program played after a reset gives the same blocks (the atlas is made again)
		tile_player.stop();
		tile_view.upload();
		CHECK(tiles.removed_volume() == 0.0, "tiles: a reset gives back the block");
		tile_player.go_to_end();
		tile_player.update(0, 5.0);
		tile_view.upload();
		CHECK(gl_errors() == 0, "no OpenGL error after a reset and a new program");
		CHECK(std::fabs(tiles.removed_volume() - grid.removed_volume()) / grid.removed_volume() < 0.03, "tiles: the same volume after a reset");
	}

	std::puts("tiles: the atlas grows (a ball tool leaves no plateau, every tile has its heights)");
	{
		SimTool ball;
		ball.shape = SimToolShape::Ball;
		ball.radius = 0.5;
		ball.length = 20;
		SimBlock wide;
		wide.low = glm::dvec2(-150, -150);
		wide.high = glm::dvec2(150, 150);
		wide.top = 0.0;
		wide.bottom = -3.0;
		wide.tool_radius = ball.radius;
		wide.precision = 30.0;
		wide.max_cells = 4000000;
		wide.plateaus = false;
		SimModelGrid wide_grid;
		SimModelTiles wide_tiles;
		wide_grid.setup(wide);
		wide_tiles.setup(wide);
		SimView grid_view(&renderer), tiles_view(&renderer);
		grid_view.set_model(wide_grid);
		tiles_view.set_model(wide_tiles);
		// rasters on the whole block but a rim of 30 mm
		for (double y = -120; y <= 120; y += 0.7)
		{
			wide_grid.cut(glm::dvec3(-120, y, -1), glm::dvec3(120, y, -1), ball);
			wide_tiles.cut(glm::dvec3(-120, y, -1), glm::dvec3(120, y, -1), ball);
		}
		grid_view.upload();
		tiles_view.upload();
		CHECK(gl_errors() == 0, "no OpenGL error while the atlas grows");
		std::printf("  %s\n", wide_tiles.describe().c_str());
		CHECK(wide_tiles.describe().find("FULL") == std::string::npos, "the atlas has places for all the tiles");
		CHECK(std::fabs(wide_tiles.removed_volume() - wide_grid.removed_volume()) / wide_grid.removed_volume() < 0.03, "the tiles took away what the grid took away");
		for (SimView* v : { &grid_view, &tiles_view })
		{
			v->camera().view(0);
			v->fit();
			v->show_axes = false;
			v->show_box = false;
		}
		Picture images[2];
		SimView* views[2] = { &grid_view, &tiles_view };
		for (int k = 0; k < 2; k++)
		{
			renderer.clear(views[k]->background.r, views[k]->background.g, views[k]->background.b, 1.0f);
			views[k]->draw(W, H);
			CHECK(gl_errors() == 0, "no OpenGL error after drawing the big block");
			images[k] = read_picture();
		}
		// the same points: the floor between the rasters, the rim, and a point in the middle
		for (glm::vec3 world : { glm::vec3(0, 0, -1), glm::vec3(60, -40, -1), glm::vec3(-100, 90, -1), glm::vec3(140, 140, 0), glm::vec3(-135, 0, 0) })
		{
			glm::ivec2 a = project(grid_view, world), b = project(tiles_view, world);
			glm::vec3 ca = images[0].at(a.x, a.y), cb = images[1].at(b.x, b.y);
			if (!near_color(ca, cb, 0.08f))
				std::printf("  (%g, %g): grid %g %g %g, tiles %g %g %g\n", world.x, world.y, ca.r, ca.g, ca.b, cb.r, cb.g, cb.b);
			CHECK(near_color(ca, cb, 0.08f), "the big block looks the same with the tiles and with the grid");
		}
		glm::ivec2 floor_pixel = project(tiles_view, glm::vec3(0, 0, -1)), rim_pixel = project(tiles_view, glm::vec3(-135, 0, 0));
		CHECK(!near_color(images[1].at(floor_pixel.x, floor_pixel.y), images[1].at(rim_pixel.x, rim_pixel.y), 0.03f), "the machined floor is not like the rim");
	}

	std::puts("a block cut through by a torch");
	{
		SimTool torch;
		torch.shape = SimToolShape::Plasma;
		torch.radius = 0.6;
		SimModelGrid plate_grid;
		SimBlock plate_block;
		plate_block.low = glm::dvec2(-20, -20);
		plate_block.high = glm::dvec2(20, 20);
		plate_block.top = 0.0;
		plate_block.bottom = -3.0;
		plate_block.tool_radius = torch.radius;
		plate_block.precision = 0.2 / torch.radius * 100.0;		// a step of 0.2 mm
		plate_block.through = true;
		plate_grid.setup(plate_block);
		SimMaterial& plate = plate_grid.material();
		// a circle of radius 10 cut through
		SimPath circle(glm::dvec3(10, 0, 0), [] { SimCommand c; c.type = SimCommandType::Arc; c.target = glm::dvec3(10, 0, 0); c.center = glm::dvec3(0, 0, 0); c.feed = 1000; return c; }());
		glm::dvec3 last = circle.at(0);
		for (int i = 1; i <= 200; i++)
		{
			glm::dvec3 p = circle.at(i / 200.0);
			plate.cut(last, p, torch);
			last = p;
		}
		SimView plate_view(&renderer);
		plate_view.set_model(plate_grid);
		plate_view.camera().view(0);
		plate_view.fit();
		plate_view.show_axes = false;
		plate_view.show_box = false;
		renderer.clear(plate_view.background.r, plate_view.background.g, plate_view.background.b, 1.0f);
		plate_view.draw(W, H);
		CHECK(gl_errors() == 0, "no OpenGL error with a plate cut through");
		Picture p = read_picture();
		glm::ivec2 on_kerf = project(plate_view, glm::vec3(10, 0, 0));
		glm::ivec2 inside = project(plate_view, glm::vec3(0, 0, 0));
		CHECK(near_color(p.at(on_kerf.x, on_kerf.y), glm::vec3(plate_view.background), 0.06f), "the kerf of the torch shows the background");
		CHECK(!near_color(p.at(inside.x, inside.y), glm::vec3(plate_view.background), 0.06f), "the piece inside of the circle is there");
		flat = p.rgba;
		for (size_t i = 3; i < flat.size(); i += 4) flat[i] = 255;
		stbi_write_png((folder / "sim_render_plasma.png").string().c_str(), W, H, 4, flat.data(), W * 4);
	}

	std::printf("pictures: %s\n", folder.string().c_str());
	glfwDestroyWindow(window);
	glfwTerminate();
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
