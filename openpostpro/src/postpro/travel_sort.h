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

#pragma once
#include <glm/glm.hpp>
#include <limits>
#include <vector>

// Order of the curves which shortens the rapid moves: the curve whose first point is the nearest to the end of the previous one is
// taken, again and again. A curve keeps its direction (it is machined from its first point to its last one). cursor is the position of
// the tool, and is set at the end of the last curve.
// before[i], if it is given, holds the curves which must be done before the curve i (the holes of a contour are cut before it): the
// curve i is only taken when they all are done. The curves which wait for others are not an obstacle for the others.
inline std::vector<int> sort_by_travel(const std::vector<glm::vec2>& starts, const std::vector<glm::vec2>& ends, glm::vec2& cursor,
	const std::vector<std::vector<int>>& before = std::vector<std::vector<int>>())
{
	const int n = (int)starts.size();
	std::vector<int> order;
	order.reserve(n);
	std::vector<char> used(n, 0);
	for (int k = 0; k < n; k++)
	{
		int best = -1;
		float best_distance = std::numeric_limits<float>::max();
		for (int i = 0; i < n; i++)
		{
			if (used[i])
				continue;
			bool ready = true;
			if (i < (int)before.size())
				for (int b : before[i])
					if (!used[b])
					{
						ready = false;
						break;
					}
			if (!ready)
				continue;
			float d = glm::distance(cursor, starts[i]);
			if (d < best_distance)
			{
				best_distance = d;
				best = i;
			}
		}
		used[best] = 1;
		order.push_back(best);
		cursor = ends[best];
	}
	return order;
}
