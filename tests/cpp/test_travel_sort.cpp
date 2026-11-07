// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Order of the contours of a group: sort_by_travel() (src/postpro/travel_sort.h) shortens the rapid moves of the tool between them.
// No window, no application: random contours and clusters of contours (the letters of a text, the teeth of gears).
//
// Build: tests/cpp/build_travel_sort.bat   Run: tests/cpp/test_travel_sort.exe   (exit code 0 = all checks pass)
#include <travel_sort.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

// length of the rapid moves for the contours taken in this order
static float travel(const std::vector<glm::vec2>& starts, const std::vector<glm::vec2>& ends, const std::vector<int>& order, glm::vec2 cursor)
{
	float total = 0;
	for (int i : order)
	{
		total += glm::distance(cursor, starts[i]);
		cursor = ends[i];
	}
	return total;
}

static bool is_permutation(const std::vector<int>& order, int n)
{
	if ((int)order.size() != n)
		return false;
	std::vector<int> sorted = order;
	std::sort(sorted.begin(), sorted.end());
	for (int i = 0; i < n; i++)
		if (sorted[i] != i)
			return false;
	return true;
}

static void test_small()
{
	std::printf("small cases\n");
	glm::vec2 cursor(5, 5);
	std::vector<glm::vec2> none;
	CHECK(sort_by_travel(none, none, cursor).empty(), "no contour, no order");
	CHECK(cursor == glm::vec2(5, 5), "the cursor does not move without contour");

	std::vector<glm::vec2> one{ {1, 2} }, one_end{ {3, 4} };
	auto o = sort_by_travel(one, one_end, cursor);
	CHECK(o.size() == 1 && o[0] == 0, "one contour");
	CHECK(cursor == glm::vec2(3, 4), "the cursor is at the end of the last contour");

	// two contours: the near one first
	glm::vec2 c(0, 0);
	std::vector<glm::vec2> s{ {100, 0}, {1, 0} }, e{ {100, 0}, {1, 0} };
	auto two = sort_by_travel(s, e, c);
	CHECK(two.size() == 2 && two[0] == 1 && two[1] == 0, "the near contour is machined first");
	CHECK(c == glm::vec2(100, 0), "cursor after two contours");
}

static void test_line()
{
	std::printf("points on a line given in any order\n");
	// the best way from the origin is increasing x
	std::vector<glm::vec2> p;
	for (int i = 0; i < 50; i++)
		p.push_back(glm::vec2((float)((i * 17) % 50), 0));
	glm::vec2 cursor(-1, 0);
	auto order = sort_by_travel(p, p, cursor);
	CHECK(is_permutation(order, 50), "a permutation");
	CHECK(std::abs(travel(p, p, order, glm::vec2(-1, 0)) - 50.0f) < 1e-3f, "the way is the length of the line");
}

static void test_directions()
{
	std::printf("the direction of the contours is kept\n");
	// open contours: each one goes from (x, 0) to (x, 10), the end is far from the start of the next one
	std::vector<glm::vec2> s, e;
	for (int i = 0; i < 6; i++)
	{
		s.push_back(glm::vec2((float)i * 20, 0));
		e.push_back(glm::vec2((float)i * 20, 10));
	}
	glm::vec2 cursor(0, 0);
	auto order = sort_by_travel(s, e, cursor);
	CHECK(is_permutation(order, 6), "a permutation");
	// the cost goes from the end of a contour to the start of the next one
	float t = travel(s, e, order, glm::vec2(0, 0));
	std::vector<int> natural(6);
	std::iota(natural.begin(), natural.end(), 0);
	CHECK(t <= travel(s, e, natural, glm::vec2(0, 0)) + 1e-3f, "not longer than the natural order");
	CHECK(cursor == e[order.back()], "cursor at the end of the last contour");
}

static void test_random()
{
	std::printf("random contours\n");
	std::mt19937 gen(7);
	std::uniform_real_distribution<float> d(0, 1000);
	for (int n : { 3, 10, 100, 500 })
	{
		std::vector<glm::vec2> s, e;
		for (int i = 0; i < n; i++)
		{
			glm::vec2 a(d(gen), d(gen));
			s.push_back(a);
			e.push_back(i % 2 ? a : a + glm::vec2(20, 5)); // closed contours and open ones
		}
		std::vector<int> natural(n);
		std::iota(natural.begin(), natural.end(), 0);
		glm::vec2 cursor(0, 0);
		auto order = sort_by_travel(s, e, cursor);
		CHECK(is_permutation(order, n), "a permutation");
		float before = travel(s, e, natural, glm::vec2(0, 0));
		float after = travel(s, e, order, glm::vec2(0, 0));
		std::printf("  %d contours: %.0f -> %.0f\n", n, before, after);
		CHECK(after <= before, "never longer than the given order");
		if (n >= 100)
			CHECK(after < before / 4, "much shorter than a random order");

		glm::vec2 cursor2(0, 0);
		CHECK(sort_by_travel(s, e, cursor2) == order, "the same order for the same input");
	}
}

static void test_clusters()
{
	std::printf("clusters of contours (letters, teeth of gears)\n");
	std::mt19937 gen(3);
	std::uniform_real_distribution<float> d(0, 1);
	std::vector<glm::vec2> s;
	for (int k = 0; k < 40; k++)
	{
		glm::vec2 center(d(gen) * 1000, d(gen) * 800);
		for (int i = 0; i < 6; i++)
			s.push_back(center + glm::vec2(d(gen), d(gen)) * 30.0f);
	}
	std::shuffle(s.begin(), s.end(), gen);
	std::vector<int> natural(s.size());
	std::iota(natural.begin(), natural.end(), 0);
	glm::vec2 cursor(0, 0);
	auto order = sort_by_travel(s, s, cursor);
	float before = travel(s, s, natural, glm::vec2(0, 0));
	float after = travel(s, s, order, glm::vec2(0, 0));
	std::printf("  %zu contours: %.0f -> %.0f\n", s.size(), before, after);
	CHECK(is_permutation(order, (int)s.size()), "a permutation");
	CHECK(after < before / 8, "the contours of a cluster are machined together");
	// a cluster of 6 is at most ~60 mm wide: 40 clusters need about 40 hops of at most 1000 mm
	CHECK(after < 40 * 400.0f, "a reasonable way for 40 clusters");
}

static void test_big()
{
	std::printf("many contours\n");
	std::mt19937 gen(11);
	std::uniform_real_distribution<float> d(0, 2000);
	for (int n : { 2000, 5000 })
	{
		std::vector<glm::vec2> s;
		for (int i = 0; i < n; i++)
			s.push_back(glm::vec2(d(gen), d(gen)));
		glm::vec2 cursor(0, 0);
		auto t0 = std::chrono::steady_clock::now();
		auto order = sort_by_travel(s, s, cursor);
		double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
		std::printf("  %d contours: %.0f ms\n", n, ms);
		CHECK(is_permutation(order, n), "a permutation");
		CHECK(ms < 5000, "fast enough");
	}
}

int main()
{
	test_small();
	test_line();
	test_directions();
	test_random();
	test_clusters();
	test_big();
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
