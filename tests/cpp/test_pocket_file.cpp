// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Computes the pockets of a document without any window: test_pocket_file.exe [file.opp [curves.txt]]
// For each pocket of the file, the pocket is read, computed again and the toolpath is checked: two curves of the same pocket
// must not cross each other (the offsets of the letters must have been merged). Without argument, the file is data/pocket_text.opp:
// two texts (thousands of segments once offset by the radius 20 of the tool) in a rectangle with rounded corners.
// With a second argument, the curves of the toolpath are written in this file (a curve per block: "C level cw size", then its points).
//
// Build: tests/cpp/build_pocket_file.bat   (exit code 0 = no crossing)
#include <pocket.h>
#include <travel_sort.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

int main(int argc, char** argv)
{
	std::string path = std::string(__FILE__).substr(0, std::string(__FILE__).find_last_of("/\\") + 1) + "data/pocket_text.opp";
	if (argc > 1)
		path = argv[1];
	std::ifstream in(path);
	if (!in)
	{
		std::printf("file not found: %s\n", path.c_str());
		return 2;
	}
	std::string line;
	bool toolpath = false;
	static TestRenderer renderer;
	static Camera camera;
	renderer.camera(&camera);
	int pockets = 0, crossings = 0;
	while (std::getline(in, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line == "{TOOLPATH}") { toolpath = true; continue; }
		if (!toolpath || line.rfind("205;", 0) != 0)
			continue;
		Pocket pocket(&renderer);
		pocket.read(line);
		pocket.update();
		auto curves = pocket.coordinates();
		int local = 0;
		for (size_t i = 0; i < curves.size(); i++)
			for (size_t j = i + 1; j < curves.size(); j++)
				if (!curves[i].bounds().outside(curves[j].bounds()) && curves[i].curve_intersect(curves[j]))
				{
					local++;
					if (local <= 5)
						std::printf("  crossing: curve %zu (%d segments) and curve %zu (%d segments)\n", i, curves[i].size(), j, curves[j].size());
				}
		if (argc > 2)
		{
			std::ofstream out(argv[2]);
			for (Curve& c : curves)
			{
				out << "C " << c.level() << ' ' << c.cw() << ' ' << c.size() << "\n";
				for (const glm::vec2& v : c.coordinates())
					out << v.x << ' ' << v.y << "\n";
			}
		}
		// the rapid moves between the curves: in the order of the pocket, and sorted (the finishing pass stays at the end)
		{
			const size_t split = curves.size() - pocket.final_count();
			float natural = 0, sorted = 0;
			glm::vec2 end_natural(0.0f), end_sorted(0.0f);
			for (int part = 0; part < 2; part++)
			{
				std::vector<glm::vec2> starts, ends;
				for (size_t i = part == 0 ? 0 : split; i < (part == 0 ? split : curves.size()); i++)
				{
					starts.push_back(curves[i].front().point);
					ends.push_back(curves[i].back().point);
				}
				for (size_t i = 0; i < starts.size(); i++)
				{
					natural += glm::distance(end_natural, starts[i]);
					end_natural = ends[i];
				}
				glm::vec2 cursor = end_sorted;
				for (int i : sort_by_travel(starts, ends, cursor))
				{
					sorted += glm::distance(end_sorted, starts[i]);
					end_sorted = ends[i];
				}
			}
			std::printf("  rapid moves: %.0f mm in the order of the pocket, %.0f mm sorted (%zu curves, %zu for the finishing)\n", natural, sorted, curves.size(), pocket.final_count());
			if (curves.size() > 8 && sorted > natural)
			{
				std::printf("  the sorted order is longer\n");
				crossings++;
			}
		}
		std::printf("pocket %d: radius %.1f, %zu curves, %d crossings\n", pockets, pocket.radius(), curves.size(), local);
		crossings += local;
		pockets++;
	}
	std::printf("%d pockets, %d crossings\n", pockets, crossings);
	return crossings == 0 ? 0 : 1;
}
