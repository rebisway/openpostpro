// OFFSET OF A SPLINE FILE - reads the splines (type 107) of a drawing file (.opp) and offsets each one with the app's Offset toolpath.
// What is checked: an offset toolpath of a closed curve gives at least one curve (nothing generated is a bug). Necks narrower than the tool
// cut the toolpath in open pieces. bug_gears.opp: an inner offset of 0.5 gave nothing because one piece came too close to the original.
//
// Build: tests/cpp/build_offset_spline.bat   Run: tests/cpp/test_offset_spline.exe <file.opp> [radius]
#include <spline.h>
#include <curve.h>
#include <offset.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <chrono>

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	if (argc < 2)
	{
		std::puts("usage: test_offset_spline <file.opp> [radius]");
		return 2;
	}
	float radius = argc > 2 ? (float)std::atof(argv[2]) : 0.5f;
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::ifstream input(argv[1]);
	std::string text;
	bool shapes = false;
	int bad = 0, runs = 0;
	while (std::getline(input, text))
	{
		if (text.size() && text[0] == '{')
		{
			shapes = text == "{SHAPES}";
			continue;
		}
		if (!shapes || text.rfind("107;", 0) != 0)
			continue;
		Spline* s = new Spline(&renderer);
		s->read(text, 0.1f);
		Curve c = s->curve();
		if (std::getenv("NEAR_X"))
		{
			glm::vec2 q((float)std::atof(std::getenv("NEAR_X")), (float)std::atof(std::getenv("NEAR_Y")));
			for (size_t i = 0; i < c.size(); i++)
			{
				Segment& g = c[i];
				if (glm::distance(g.point, q) < 40)
					std::printf("  seg %zu %s p(%g,%g) c(%g,%g) r %g cw %d len %g\n", i, g.type == SegmentType::Line ? "line" : "arc ", g.point.x, g.point.y, g.center.x, g.center.y, g.radius, (int)g.cw, g.length);
			}
		}
		std::printf("spline: %zu segments, closed %d, length %g\n", c.size(), (int)c.closed(), c.length());
		{
			c.length();
			int under[5] = {0,0,0,0,0};
			float lim[5] = {0.01f, 0.05f, 0.1f, 0.5f, 1.0f};
			for (size_t i = 0; i < c.size(); i++)
				for (int k = 0; k < 5; k++)
					if (c[i].length >= 0 && c[i].length < lim[k]) under[k]++;
			std::printf("  input segments shorter than 0.01 / 0.05 / 0.1 / 0.5 / 1.0: %d %d %d %d %d\n", under[0], under[1], under[2], under[3], under[4]);
		}
		for (int cw = 0; cw < 2; cw++)
			for (int interior = 0; interior < 2; interior++)
			{
				{
					// the curve itself, without the toolpath
					Curve w = c;
					if (std::getenv("SIMPLIFY")) w.simplify((float)std::atof(std::getenv("SIMPLIFY")) * radius);
					w.cw(cw != 0);
					auto t0 = std::chrono::steady_clock::now();
					auto direct = w.offset(interior ? -radius : radius, 0, true);
					std::printf("  offset time %.3f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
					std::printf("  direct cw %d interior %d: %zu curves\n", cw, interior, direct.size());
					// the distance of every vertex of the result to the original (dense points of the original)
					std::vector<glm::vec2> dense = c.coordinates();
					for (Curve& d : direct)
					{
						if (!std::getenv("DENSE_CHECK"))
							continue;
						float worst = 1e9f;
						int where = -1;
						auto pts = d.coordinates();
						for (size_t i = 0; i < pts.size(); i++)
						{
							float nearest = 1e9f;
							for (size_t k = 0; k + 1 < dense.size(); k++)
							{
								glm::vec2 e = dense[k + 1] - dense[k];
								float l2 = glm::dot(e, e);
								float t = l2 < 1e-12f ? 0.0f : glm::clamp(glm::dot(pts[i] - dense[k], e) / l2, 0.0f, 1.0f);
								nearest = (glm::min)(nearest, glm::distance(pts[i], dense[k] + e * t));
							}
							if (nearest < worst) { worst = nearest; where = (int)i; }
						}
						{
							// the dense points of the result closer to the original than the radius: the places where the offset is wrong
							auto pts2 = d.coordinates();
							int run_start = -1; float run_min = 1e9f; glm::vec2 run_at;
							auto flush = [&](size_t end) {
								if (run_start >= 0)
									std::printf("    close run points %d..%zu: min %g at %g,%g\n", run_start, end, run_min, run_at.x, run_at.y);
								run_start = -1; run_min = 1e9f;
							};
							for (size_t i = 0; i < pts2.size(); i++)
							{
								float nearest = 1e9f;
								for (size_t k = 0; k + 1 < dense.size(); k++)
								{
									glm::vec2 e = dense[k + 1] - dense[k];
									float l2 = glm::dot(e, e);
									float t = l2 < 1e-12f ? 0.0f : glm::clamp(glm::dot(pts2[i] - dense[k], e) / l2, 0.0f, 1.0f);
									nearest = (glm::min)(nearest, glm::distance(pts2[i], dense[k] + e * t));
								}
								if (nearest < radius * 0.95f)
								{
									if (run_start < 0) run_start = (int)i;
									if (nearest < run_min) { run_min = nearest; run_at = pts2[i]; }
								}
								else
									flush(i);
							}
							flush(pts2.size());
						}
						if (std::getenv("NEAR_X") && interior)
						{
							glm::vec2 q((float)std::atof(std::getenv("NEAR_X")), (float)std::atof(std::getenv("NEAR_Y")));
							for (size_t i = 0; i < d.size(); i++)
								if (glm::distance(d[i].point, q) < 6)
									std::printf("    res %zu %s p(%.4f,%.4f) c(%.4f,%.4f) r %g cw %d\n", i, d[i].type == SegmentType::Line ? "line" : "arc ", d[i].point.x, d[i].point.y, d[i].center.x, d[i].center.y, d[i].radius, (int)d[i].cw);
						}
						std::printf("    piece %zu segments %zu points: closest vertex to the original %g (at %g,%g), radius %g\n", d.size(), pts.size(), worst, where >= 0 ? pts[where].x : 0.f, where >= 0 ? pts[where].y : 0.f, radius);
					}
				}
				Offset* o = new Offset(&renderer);
				o->radius(radius);
				o->cw(cw != 0);
				o->interior(interior != 0);
				o->add(c);
				o->compute(); o->update();
				auto got = o->coordinates();
				int opened = 0;
				float total = 0;
				for (Curve& r : got)
				{
					total += r.length();
					if (!r.closed())
						opened++;
				}
				runs++;
				bool fail = got.empty();	// a neck narrower than the tool cuts the toolpath in open pieces, which is fine: nothing at all is not
				if (fail)
					bad++;
				std::printf("  %s cw %d interior %d: %zu curves, %d open, total length %g\n", fail ? "FAIL" : "ok  ", cw, interior, got.size(), opened, total);
				delete o;
			}
	}
	std::printf("%d offsets, %d failed\n", runs, bad);
	return bad == 0 ? 0 : 1;
}
