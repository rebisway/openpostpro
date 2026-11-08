// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Compares the models of the material of the simulator (SimModel) without any window: every method makes the same cuts, then
// the heights, the volume, the memory and the times are compared with the first method (the grid, the reference).
//
// The cuts: a pocket (rasters at -2 around an island) and a diagonal groove at -4, in a block, with a small tool. Two blocks: one
// where every method can use its finest step, and a large one (the number of cells limits the step).
//
// Build: tests/cpp/build_sim_models.bat   (exit code 0 = every check passed)
#include <sim_model.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int checks = 0, failed = 0;
#define CHECK(condition, text) do { checks++; if (!(condition)) { failed++; std::printf("  FAILED: %s\n", text); } } while (0)

struct Cut { glm::dvec3 a, b; };

// The program: rasters of the pocket (the island of radius `island` stays), then a diagonal groove
static std::vector<Cut> program(double low, double high, double island)
{
	std::vector<Cut> cuts;
	const double center = (low + high) * 0.5;
	for (double y = low; y <= high; y += 0.5)
	{
		double dy = y - center;
		if (std::fabs(dy) < island + 0.75)
		{
			double dx = std::sqrt(std::max(0.0, (island + 0.75) * (island + 0.75) - dy * dy));
			cuts.push_back({ glm::dvec3(low, y, -2), glm::dvec3(center - dx, y, -2) });
			cuts.push_back({ glm::dvec3(center + dx, y, -2), glm::dvec3(high, y, -2) });
		}
		else
			cuts.push_back({ glm::dvec3(low, y, -2), glm::dvec3(high, y, -2) });
	}
	cuts.push_back({ glm::dvec3(low, low, -4), glm::dvec3(high, high, -4) });
	return cuts;
}

struct Result
{
	std::string name;
	std::string describe;
	double setup_ms = 0, cut_ms = 0, volume = 0;
	size_t cells = 0, memory = 0;
	std::vector<double> heights;
};

static double since(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

static Result run(int method, const SimBlock& block, const std::vector<Cut>& cuts, const SimTool& tool, const std::vector<glm::dvec2>& samples)
{
	Result r;
	auto model = SimModel::create(method);
	r.name = model->name();
	auto t = std::chrono::steady_clock::now();
	model->setup(block);
	r.setup_ms = since(t);
	t = std::chrono::steady_clock::now();
	for (const Cut& c : cuts)
		model->cut(c.a, c.b, tool);
	model->upload();		// without graphics: the tiles which are plateaus are simplified
	r.cut_ms = since(t);
	r.volume = model->removed_volume();
	r.cells = model->cells();
	r.memory = model->memory();
	r.describe = model->describe();
	for (const glm::dvec2& p : samples)
		r.heights.push_back(model->height_at(p.x, p.y));

	// what does not depend on the method
	CHECK(model->height_at(block.low.x + 0.5, block.low.y + 0.5) > -0.01 || block.low.x + 0.5 > 0, "the corner of the block is read");
	const glm::dvec3 low = model->low(), high = model->high();
	CHECK(low.z == block.bottom && high.z == block.top, "the box has the heights of the block");
	CHECK(low.x <= block.low.x + 1e-6 && high.x >= block.high.x - 1e-6, "the box covers the block");
	return r;
}

static void compare(const std::string& title, double low, double high, double margin, double tolerance, SimToolShape shape = SimToolShape::Flat)
{
	std::printf("%s\n", title.c_str());
	SimTool tool;
	tool.shape = shape;
	tool.radius = 0.75;
	tool.length = 20;

	SimBlock block;
	block.low = glm::dvec2(low - margin, low - margin);
	block.high = glm::dvec2(high + margin, high + margin);
	block.top = 0.0;
	block.bottom = -6.0;
	block.tool_radius = tool.radius;
	block.precision = 15.0;
	block.max_cells = 4000000;
	block.plateaus = shape == SimToolShape::Flat;

	const double island = (high - low) * 0.15;
	const double center = (low + high) * 0.5;
	std::vector<Cut> cuts = program(low, high, island);

	std::vector<glm::dvec2> samples;
	for (int i = 0; i <= 60; i++)
		for (int j = 0; j <= 60; j++)
			samples.push_back(glm::dvec2(block.low.x + (block.high.x - block.low.x) * i / 60.0, block.low.y + (block.high.y - block.low.y) * j / 60.0));

	std::vector<Result> results;
	for (int m = 0; m < SimModel::method_count(); m++)
		results.push_back(run(m, block, cuts, tool, samples));

	std::printf("  %-10s %12s %10s %10s %12s  %s\n", "method", "cells", "memory KB", "cuts ms", "volume mm3", "detail");
	for (const Result& r : results)
		std::printf("  %-10s %12zu %10zu %10.1f %12.1f  %s\n", r.name.c_str(), r.cells, r.memory / 1024, r.cut_ms, r.volume, r.describe.c_str());

	for (const Result& r : results)
		CHECK(r.describe.find("FULL") == std::string::npos, "the model has room for all its heights");

	// the reference: what the program takes away, about (area of the pocket - island) * 2 mm, and the groove
	const double area = (high - low) * (high - low) - 3.14159265 * (island + 0.75) * (island + 0.75);
	const Result& ref = results[0];
	CHECK(ref.volume > area * 2.0 * 0.9 && ref.volume < area * 2.0 * 1.4, "the volume of the reference is the one of the pocket (and the groove)");

	// the heights of the sample: the island stays at the top, the pocket is at -2, the groove at -4
	std::unique_ptr<SimModel> probe = SimModel::create(0);
	probe->setup(block);
	for (const Cut& c : cuts)
		probe->cut(c.a, c.b, tool);
	CHECK(std::fabs(probe->height_at(center + island * 0.5, center - island * 0.4) - 0.0) < 1e-6, "the island is not touched");
	CHECK(std::fabs(probe->height_at(low + 3.3, high - 3.7) - (-2.0)) < tolerance, "the bottom of the pocket is at -2");
	CHECK(std::fabs(probe->height_at(low + (high - low) * 0.3, low + (high - low) * 0.3) - (-4.0)) < tolerance, "the groove is at -4");

	// the other methods against the reference
	for (size_t k = 1; k < results.size(); k++)
	{
		const Result& r = results[k];
		int different = 0;
		double worst = 0;
		for (size_t i = 0; i < r.heights.size(); i++)
		{
			double d = std::fabs(r.heights[i] - ref.heights[i]);
			worst = std::max(worst, d);
			if (d > 0.05)
				different++;
		}
		std::printf("  %s against %s: %d / %zu points differ by more than 0.05 mm, the worst is %.3f mm, volume %+.2f %%\n", r.name.c_str(), ref.name.c_str(),
			different, r.heights.size(), worst, (r.volume - ref.volume) / ref.volume * 100.0);
		CHECK(different * 100 < (int)r.heights.size() * 3, "the heights are the ones of the reference (less than 3 % of the points differ)");
		CHECK(std::fabs(r.volume - ref.volume) / ref.volume < 0.03, "the volume is the one of the reference (3 %)");
	}
}

int main()
{
	std::printf("%d method(s):", SimModel::method_count());
	for (int m = 0; m < SimModel::method_count(); m++)
		std::printf(" %s", SimModel::method_name(m));
	std::printf("\n");

	compare("a block of 100 x 100 mm, tool of 1.5 mm (every method can be fine)", 10, 90, 10, 0.2);
	compare("a large block of 600 x 600 mm (the number of cells limits the grid)", 10, 590, 10, 0.6);
	compare("a block of 100 x 100 mm with a ball tool (no plateau: every tile has heights)", 10, 90, 10, 0.2, SimToolShape::Ball);

	std::printf("%d checks, %d failed\n", checks, failed);
	return failed == 0 ? 0 : 1;
}
