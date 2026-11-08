// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Times the frames of the simulator for each model of the material, with a hidden OpenGL window: a big block (like gears.nc: 916 mm,
// a tool of 0.75 mm) is machined by rasters, frame after frame. A frame is what the application does: the player cuts for 8 ms,
// then the changes are sent to the graphic card (upload) and the scene is drawn. The three times are measured apart.
//
// Usage: test_sim_frames.exe [seconds for each model (default 12)] [block in mm (default 900)]
// Build: tests/cpp/build_sim_frames.bat
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "renderer_opengl.h"
#include <sim_view.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#ifndef SHADER_DIR
#error SHADER_DIR must be defined by the build script
#endif

static const int W = 900, H = 640;

static double now()
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Cut { glm::dvec3 a, b; };

// rasters of a pocket, 0.6 mm apart, with islands (the walls) which the tool goes around
static std::vector<Cut> program(double size)
{
	std::vector<Cut> cuts;
	const double low = -size * 0.45, high = size * 0.45;
	bool forward = true;
	for (double y = low; y <= high; y += 0.6)
	{
		double x0 = low, x1 = high;
		if (!forward)
			std::swap(x0, x1);
		cuts.push_back({ glm::dvec3(x0, y, -3), glm::dvec3(x1, y, -3) });
		forward = !forward;
	}
	return cuts;
}

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	const double seconds = argc > 1 ? std::atof(argv[1]) : 12.0;
	const double size = argc > 2 ? std::atof(argv[2]) : 900.0;

	if (!glfwInit())
	{
		std::puts("skipped: glfwInit failed");
		return 0;
	}
	OpenGlRenderer renderer;
	renderer.initialize_GLFW();
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(W, H, "sim frames", nullptr, nullptr);
	if (window == nullptr)
	{
		std::puts("skipped: no OpenGL 3.3 core context on this machine");
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	if (gl3wInit() != 0)
	{
		std::puts("skipped: gl3w could not load OpenGL");
		return 0;
	}
	std::printf("OpenGL %s, %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
	glViewport(0, 0, W, H);
	for (const char* name : { "sim", "simtile", "line3d" })
		renderer.load_program(name, SHADER_DIR);

	SimTool tool;
	tool.shape = SimToolShape::Flat;
	tool.radius = 0.375;
	tool.length = 20;

	SimBlock block;
	block.low = glm::dvec2(-size * 0.5, -size * 0.5);
	block.high = glm::dvec2(size * 0.5, size * 0.5);
	block.top = 0.0;
	block.bottom = -8.0;
	block.tool_radius = tool.radius;
	block.precision = 4.0;
	block.max_cells = 15000000;

	std::vector<Cut> cuts = program(size);
	std::printf("block %.0f mm, tool %.2f mm, %zu rasters, %.0f s for each model\n", size, tool.radius * 2, cuts.size(), seconds);
	std::printf("%-8s %7s %9s %9s %9s %9s %9s  %s\n", "model", "frames", "cut ms", "upload ms", "draw ms", "frame ms", "progress", "detail");

	for (int method = 0; method < SimModel::method_count(); method++)
	{
		auto model = SimModel::create(method);
		model->setup(block);
		SimView view(&renderer);
		view.set_model(*model);
		view.show_axes = false;
		model->upload();

		size_t next = 0;
		int frames = 0;
		double cut_total = 0, upload_total = 0, draw_total = 0;
		const double start = now();
		while (next < cuts.size() && now() - start < seconds * 1000.0)
		{
			double t0 = now();
			while (next < cuts.size() && now() - t0 < 8.0)
			{
				model->cut(cuts[next].a, cuts[next].b, tool);
				next++;
			}
			double t1 = now();
			view.upload();
			glFinish();
			double t2 = now();
			renderer.clear(view.background.r, view.background.g, view.background.b, 1.0f);
			view.draw(W, H);
			glFinish();
			double t3 = now();
			cut_total += t1 - t0;
			upload_total += t2 - t1;
			draw_total += t3 - t2;
			frames++;
		}
		const double n = frames > 0 ? frames : 1;
		std::printf("%-8s %7d %9.2f %9.2f %9.2f %9.2f %8.0f%%  %s\n", model->name(), frames, cut_total / n, upload_total / n, draw_total / n,
			(cut_total + upload_total + draw_total) / n, 100.0 * next / cuts.size(), model->describe().c_str());
	}
	glfwTerminate();
	return 0;
}
