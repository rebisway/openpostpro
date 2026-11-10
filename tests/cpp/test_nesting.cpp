// Test of the nesting engine on parts like those of a real cutting sheet (frames with a window, slotted plates, rings,
// brackets, triangles, washers). Checks that every part is placed, that no two parts share material (independent check on a grid),
// and writes nest_result.svg to look at the result.
// Build: tests\cpp\build_nesting.bat, run: tests\cpp\test_nesting.exe
#include <nesting.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace nesting;

static Ring circle_ring(float cx, float cy, float r, int n = 48)
{
	Ring ring;
	for (int i = 0; i < n; i++)
	{
		float a = 6.2831853f * i / n;
		ring.push_back({ cx + r * std::cos(a), cy + r * std::sin(a) });
	}
	return ring;
}

static Ring round_rect(float x, float y, float w, float h, float r)
{
	Ring ring;
	glm::vec2 c[4] = { {x + w - r, y + r}, {x + w - r, y + h - r}, {x + r, y + h - r}, {x + r, y + r} };
	for (int k = 0; k < 4; k++)
		for (int i = 0; i <= 8; i++)
		{
			float a = (k * 90 + i * 90.0f / 8) * 3.14159265f / 180 - 1.5707963f;
			ring.push_back({ c[k].x + r * std::cos(a), c[k].y + r * std::sin(a) });
		}
	return ring;
}

static Part frame(float w, float h, float r, float wx, float wy, float ww, float wh)
{
	Part p;
	p.outer = round_rect(0, 0, w, h, r);
	p.inner.push_back(round_rect(wx, wy, ww, wh, 6));
	return p;
}

static Part ring_part(float ro, float ri, float rc, float rci)
{
	Part p;
	p.outer = circle_ring(0, 0, ro);
	p.inner.push_back(circle_ring(0, 0, ri));
	return p;
}

static Part rings_part(float r1, float r2, float r3, float r4)
{
	Part p;
	p.outer = circle_ring(0, 0, r1);
	p.inner.push_back(circle_ring(0, 0, r2));
	p.inner.push_back(circle_ring(0, 0, r3));
	p.inner.push_back(circle_ring(0, 0, r4));
	return p;
}

static Part slotted(float w, float h, float r, int slots)
{
	Part p;
	p.outer = round_rect(0, 0, w, h, r);
	for (int i = 0; i < slots; i++)
		p.inner.push_back(round_rect(w * (i + 0.5f) / slots - 15, h / 2 - 7, 30, 14, 7));
	return p;
}

static Part polygon(std::vector<glm::vec2> pts)
{
	Part p;
	p.outer = pts;
	return p;
}

static int failures = 0;
static void check(bool ok, const char* text)
{
	std::printf("%s %s\n", ok ? "PASS" : "FAIL", text);
	if (!ok)
		failures++;
}

// Independent check: no cell of a 1 mm grid is inside the material of two parts, and every part is inside the sheet.
static void verify(const std::vector<Part>& parts, const std::vector<Placement>& result, const Options& o, const char* label)
{
	char text[128];
	bool outside = false;
	std::vector<Ring> rings;
	std::vector<int> owner(1100 * 600, -1);
	bool clash = false;
	auto inside = [](const Ring& r, glm::vec2 p)
	{
		bool in = false;
		for (size_t i = 0, j = r.size() - 1; i < r.size(); j = i++)
			if ((r[i].y > p.y) != (r[j].y > p.y) && p.x < (r[j].x - r[i].x) * (p.y - r[i].y) / (r[j].y - r[i].y) + r[i].x)
				in = !in;
		return in;
	};
	for (size_t k = 0; k < result.size(); k++)
	{
		if (!result[k].placed)
			continue;
		const Part& part = parts[result[k].part];
		Ring outer = transform(part.outer, result[k].angle, result[k].offset);
		std::vector<Ring> hs;
		for (const Ring& h : part.inner)
			hs.push_back(transform(h, result[k].angle, result[k].offset));
		glm::vec2 mn(1e9f), mx(-1e9f);
		for (glm::vec2 p : outer) { mn = glm::min(mn, p); mx = glm::max(mx, p); }
		for (int y = std::max(0, (int)mn.y); y < std::min(600, (int)mx.y + 1); y++)
			for (int x = std::max(0, (int)mn.x); x < std::min(1100, (int)mx.x + 1); x++)
			{
				// not the exact middle of the cell: two parts which touch along a line (no spacing) would both own a point which is on it
				glm::vec2 c(x + 0.4973f, y + 0.5021f);
				if (!inside(outer, c))
					continue;
				int around = 0;
				for (const Ring& h : hs)
					if (inside(h, c)) around++;
				if (around % 2 == 1)
					continue;
				if (owner[y * 1100 + x] != -1 && owner[y * 1100 + x] != (int)k)
				{
					if (!clash)
					{
						const Placement& other = result[owner[y * 1100 + x]];
						std::printf("clash at %d,%d: item %d (part %d angle %.0f offset %.2f,%.2f) with item %d (part %d angle %.0f offset %.2f,%.2f)\n", x, y, (int)k, result[k].part, result[k].angle, result[k].offset.x, result[k].offset.y, owner[y * 1100 + x], other.part, other.angle, other.offset.x, other.offset.y);
					}
					clash = true;
				}
				owner[y * 1100 + x] = (int)k;
			}
		if (!(mn.x >= o.margin - 0.5f && mn.y >= o.margin - 0.5f && mx.x <= o.width - o.margin + 0.5f && mx.y <= o.height - o.margin + 0.5f))
			outside = true;
	}
	std::snprintf(text, sizeof text, "%s: every part is inside the sheet", label);
	check(!outside, text);
	std::snprintf(text, sizeof text, "%s: no two parts share material", label);
	check(!clash, text);

}

int main(int argc, char** argv)
{
	// Outlines put on each other: the ring of a part is the same as the island of another one. Three copies of the small ring go into
	// the hole of the big one, none of them on the island (this was placed on it when only the second part looked at its edges).
	{
		std::vector<Part> two{ rings_part(70, 52, 38, 26), ring_part(38, 26, 0, 0) };
		two[1].quantity = 3;
		Options small;
		small.width = 400;
		small.height = 300;
		std::vector<Placement> r = nest(two, small);
		bool clear = true;
		for (size_t i = 0; i < r.size(); i++)
			for (size_t j = i + 1; j < r.size(); j++)
				if (r[i].placed && r[j].placed && overlap(two[r[i].part], r[i], two[r[j].part], r[j]))
					clear = false;
		check(clear, "rings on the island of another part");
	}

	std::vector<Part> parts;
	auto add = [&](Part p, int q) { p.quantity = q; parts.push_back(p); };
	add(frame(260, 400, 12, 30, 60, 200, 300), 2);
	add(ring_part(70, 52, 0, 0), 2);
	add(ring_part(38, 26, 0, 0), 4);
	add(rings_part(70, 52, 38, 26), 2);
	add(slotted(220, 70, 20, 3), 1);
	add(slotted(160, 120, 14, 1), 1);
	add(polygon({ {0, 0}, {60, 0}, {60, 18}, {18, 18}, {18, 90}, {0, 90} }), 3);
	add(polygon({ {0, 0}, {50, 0}, {0, 80} }), 5);
	add(ring_part(22, 12, 0, 0), 8);
	add(ring_part(14, 7, 0, 0), 8);
	add(slotted(140, 40, 10, 2), 2);
	Part disc; disc.outer = circle_ring(0, 0, 25); add(disc, 6);

	Options o;
	o.width = 1100;
	o.height = 600;
	o.margin = 5;
	o.step = 8;
	o.angles = { 0, 90, 180, 270 };

	// bench (test_nesting.exe bench <seconds>): many parts, no big frame which sets the length by itself
	if (argc > 1)
	{
		for (Part& p : parts)
			p.quantity = area(p) > 30000 ? 1 : p.quantity * 2;
		o.width = 2000;
		// the third argument: a step of rotation in degrees (90 by default); a time of 0 gives the greedy algorithm only
		if (argc > 3)
		{
			o.angles.clear();
			for (float a = 0; a < 360; a += (float)std::atof(argv[3]))
				o.angles.push_back(a);
		}
		float material = 0;
		for (const Part& p : parts) material += area(p) * p.quantity;
		for (int mode = 0; mode < (argc > 2 && std::atof(argv[2]) <= 0 ? 1 : 3); mode++)
		{
			Options b = o;
			b.algorithm = mode == 0 ? Algorithm::Greedy : mode == 1 ? Algorithm::Search : Algorithm::Overlap;
			b.time = argc > 2 ? (float)std::atof(argv[2]) : 20.0f;
			Progress progress;
			auto begin = std::chrono::steady_clock::now();
			std::vector<Placement> r = nest(parts, b, &progress);
			float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - begin).count();
			float len = length(parts, r);
			int missed = 0;
			for (const Placement& p : r) missed += p.placed ? 0 : 1;
			std::printf("%s: %d unplaced, length %.0f, density %.1f %%, %.1f s, %d tries\n", mode == 0 ? "greedy" : mode == 1 ? "search" : "overlap", missed, len, material / (len * (o.height - 2 * o.margin)) * 100, seconds, progress.tries.load());
			verify(parts, r, o, mode == 0 ? "bench greedy" : mode == 1 ? "bench search" : "bench overlap");
		}
		return failures ? 1 : 0;
	}

	std::vector<Placement> result = nest(parts, o);

	int count = 0, placed = 0;
	for (const Placement& p : result) { count++; placed += p.placed ? 1 : 0; }
	std::printf("%d parts, %d placed, utilisation %.1f %%, length used %.0f\n", count, placed, utilisation(parts, result, o) * 100, length(parts, result));
	check(placed == count, "every part is placed");

	verify(parts, result, o, "greedy");

	// The search: it never gives a longer sheet than the greedy result (it starts from it), has all the parts, and no overlap.
	{
		Options searching = o;
		searching.algorithm = Algorithm::Search;
		searching.time = 4;
		Progress progress;
		std::vector<Placement> found = nest(parts, searching, &progress);
		int missed = 0;
		for (const Placement& p : found) missed += p.placed ? 0 : 1;
		float before = length(parts, result), after = length(parts, found);
		std::printf("search: length %.0f (greedy %.0f), %d tries\n", after, before, progress.tries.load());
		check(missed == 0, "search: every part is placed");
		check(after <= before + 0.01f, "search: not longer than the greedy result");
		verify(parts, found, o, "search");
	}

	// The overlap minimization: it starts from the greedy result, so it is never longer; every part is placed and nothing overlaps.
	{
		Options minimizing = o;
		minimizing.algorithm = Algorithm::Overlap;
		minimizing.time = 6;
		Progress progress;
		std::vector<Placement> found = nest(parts, minimizing, &progress);
		int missed = 0;
		for (const Placement& p : found) missed += p.placed ? 0 : 1;
		float before = length(parts, result), after = length(parts, found);
		std::printf("overlap: length %.0f (greedy %.0f), %d tries\n", after, before, progress.tries.load());
		check(missed == 0, "overlap: every part is placed");
		check(after <= before + 0.01f, "overlap: not longer than the greedy result");
		verify(parts, found, o, "overlap");
	}

	// Stop: a long search ends soon after the stop is asked, and gives a valid result.
	for (Algorithm algorithm : { Algorithm::Search, Algorithm::Overlap })
	{
		Options running = o;
		running.algorithm = algorithm;
		running.time = 120;
		Progress progress;
		std::vector<Placement> stopped;
		auto begin = std::chrono::steady_clock::now();
		std::thread worker([&] { stopped = nest(parts, running, &progress); });
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		progress.stop = true;
		worker.join();
		float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - begin).count();
		const char* name = algorithm == Algorithm::Search ? "stop search" : "stop overlap";
		std::printf("%s: ended after %.1f s\n", name, seconds);
		check(seconds < 20, name);
		verify(parts, stopped, o, name);
	}

	// SVG of the result
	std::ofstream svg("nest_result.svg");
	svg << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1100 600' width='1100' height='600'><rect width='1100' height='600' fill='#333'/>\n";
	const char* colors[] = { "#6af", "#af6", "#c8f", "#fe8", "#8e8", "#e6e", "#8cf", "#fa3" };
	for (size_t k = 0; k < result.size(); k++)
	{
		if (!result[k].placed) continue;
		const Part& part = parts[result[k].part];
		svg << "<path fill='none' stroke='" << colors[result[k].part % 8] << "' stroke-width='1' d='";
		auto path = [&](const Ring& r)
		{
			Ring t = transform(r, result[k].angle, result[k].offset);
			for (size_t i = 0; i < t.size(); i++)
				svg << (i ? " L" : "M") << t[i].x << " " << 600 - t[i].y;
			svg << " Z ";
		};
		path(part.outer);
		for (const Ring& h : part.inner) path(h);
		svg << "'/>\n";
	}
	svg << "</svg>\n";

	std::printf("%s\n", failures ? "FAILED" : "ALL PASSED");
	return failures ? 1 : 0;
}
