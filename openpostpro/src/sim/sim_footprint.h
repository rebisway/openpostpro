// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// The columns which are under the tool. A tool which goes from a to b covers a capsule (the segment and the disc of the tool around
// it): on each row of columns, the covered ones are the columns of one interval of x. The models only look at those columns, not at
// the whole box of the move.

#pragma once
#ifndef _SIM_FOOTPRINT_H
#define _SIM_FOOTPRINT_H

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

// The interval of x of the capsule (a, b, radius r) on the row y. False if the row does not meet it.
inline bool footprint_row(const glm::dvec2& a, const glm::dvec2& b, double r, double y, double& x_low, double& x_high)
{
	double lo = std::numeric_limits<double>::max(), hi = -std::numeric_limits<double>::max();

	// the discs at the two ends
	for (const glm::dvec2& c : { a, b })
	{
		double dy = y - c.y;
		double q = r * r - dy * dy;
		if (q < 0.0)
			continue;
		double s = std::sqrt(q);
		lo = std::min(lo, c.x - s);
		hi = std::max(hi, c.x + s);
	}

	// the rectangle between them: 0 <= s <= length along the move, |w| <= r across it (s and w are linear in x on the row)
	glm::dvec2 d = b - a;
	double length2 = glm::dot(d, d);
	if (length2 > 1e-18)
	{
		double length = std::sqrt(length2);
		glm::dvec2 u = d / length;
		glm::dvec2 n(-u.y, u.x);
		double s0 = (y - a.y) * u.y - a.x * u.x, s1 = u.x;
		double w0 = (y - a.y) * n.y - a.x * n.x, w1 = n.x;
		double x_a = -std::numeric_limits<double>::max(), x_b = std::numeric_limits<double>::max();
		bool inside = true;
		auto clip = [&](double c0, double c1, double low, double high)
		{
			if (std::fabs(c1) < 1e-12)
			{
				if (c0 < low || c0 > high)
					inside = false;
				return;
			}
			double p = (low - c0) / c1, q = (high - c0) / c1;
			if (p > q)
				std::swap(p, q);
			x_a = std::max(x_a, p);
			x_b = std::min(x_b, q);
		};
		clip(s0, s1, 0.0, length);
		clip(w0, w1, -r, r);
		if (inside && x_a <= x_b)
		{
			lo = std::min(lo, x_a);
			hi = std::max(hi, x_b);
		}
	}

	if (lo > hi)
		return false;
	// the union of the three intervals is the interval of the capsule (it is convex)
	x_low = lo - 1e-9;
	x_high = hi + 1e-9;
	return true;
}

#endif
