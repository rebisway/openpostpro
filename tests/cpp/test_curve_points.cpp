// OFFSET OF A CURVE ALONE - reads the curve number N of a file written by test_pocket_file (its third argument: a block "S level cw size"
// followed by its segments, "L x y" or "A x y cx cy radius cw"), and offsets it by the given distance. Prints what comes out.
// Run: tests/cpp/test_curve_points.exe <segments.txt> <index> <distance> [cw 0|1]
#include <curve.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::puts("usage: test_curve_points <curves.txt> <index> <distance> [cw]");
		return 2;
	}
	int wanted = std::atoi(argv[2]);
	float distance = (float)std::atof(argv[3]);
	std::ifstream in(argv[1]);
	std::string text;
	int index = -1;
	int cw_file = 0;
	Curve c;
	while (std::getline(in, text))
	{
		if (text.size() && text[0] == 'S')
		{
			index++;
			std::istringstream s(text.substr(2));
			int level, size;
			s >> level >> cw_file >> size;
			continue;
		}
		if (index != wanted)
			continue;
		std::istringstream s(text.substr(2));
		if (text[0] == 'L')
		{
			float x, y;
			s >> x >> y;
			c.add(glm::vec2(x, y));
		}
		else if (text[0] == 'A')
		{
			float x, y, cx, cy, r;
			int cw;
			s >> x >> y >> cx >> cy >> r >> cw;
			c.add(glm::vec2(x, y), glm::vec2(cx, cy), r, cw != 0);
		}
	}
	c.close();
	c.cw(argc > 4 ? std::atoi(argv[4]) != 0 : cw_file != 0);
	std::printf("curve %d: %zu segments, cw %d, area %g, bounds %g x %g\n", wanted, c.size(), (int)c.cw(), c.area(), c.bounds().width(), c.bounds().height());
	if (std::getenv("SIMPLIFY"))
		std::printf("simplify removed %d vertices\n", c.simplify((float)std::atof(std::getenv("SIMPLIFY")) * (distance < 0 ? -distance : distance)));
	// CUT in the environment: as the pocket does, the result is cut where the tool can not go and the pieces are closed
	const bool cut = std::getenv("CUT") != nullptr;
	auto result = c.offset(distance, 0, cut, cut);
	std::printf("offset %g: %zu curves\n", distance, result.size());
	for (Curve& r : result)
		std::printf("  %zu segments, bounds %g x %g\n", r.size(), r.bounds().width(), r.bounds().height());
	if (std::getenv("DENSE_CHECK"))
	{
		// the result closer to the curve than the offset: where the offset is wrong
		std::vector<glm::vec2> dense = c.coordinates();
		for (Curve& r : result)
		{
			std::vector<glm::vec2> pts = r.coordinates();
			int run_start = -1; float run_min = 1e9f; glm::vec2 run_at;
			for (size_t i = 0; i <= pts.size(); i++)
			{
				float nearest = 1e9f;
				if (i < pts.size())
					for (size_t k = 0; k + 1 < dense.size(); k++)
					{
						glm::vec2 e = dense[k + 1] - dense[k];
						float l2 = glm::dot(e, e);
						float t = l2 < 1e-12f ? 0.0f : glm::clamp(glm::dot(pts[i] - dense[k], e) / l2, 0.0f, 1.0f);
						nearest = (glm::min)(nearest, glm::distance(pts[i], dense[k] + e * t));
					}
				if (i < pts.size() && nearest < glm::abs(distance) * 0.95f)
				{
					if (run_start < 0) run_start = (int)i;
					if (nearest < run_min) { run_min = nearest; run_at = pts[i]; }
				}
				else if (run_start >= 0)
				{
					std::printf("  close run points %d..%zu: min %g at %g,%g\n", run_start, i, run_min, run_at.x, run_at.y);
					run_start = -1; run_min = 1e9f;
				}
			}
		}
	}
	return result.empty() ? 1 : 0;
}
