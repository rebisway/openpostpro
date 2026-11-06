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

#include "extend.h"
#include <geometry.h>
#include <limits>

namespace
{
	// an intersection closer than this to the end of the entity is the end itself
	const float MIN_DISTANCE = 0.001f;
	const float MIN_ANGLE = 0.0001f;

	// distance from the point to the farthest corner of the boundaries: a prolongation of that length (plus a margin)
	// is long enough to reach anything. It is kept as short as possible, as the intersections are computed in float.
	float reach(glm::vec2 from, std::vector<Curve>& boundaries)
	{
		float result = 1.0f;
		for (Curve& c : boundaries)
		{
			if (c.size() == 0)
				continue;
			geometry::rectangle& b = c.bounds();
			glm::vec2 corners[4] = { b.top_left, b.bottom_right, glm::vec2(b.top_left.x, b.bottom_right.y), glm::vec2(b.bottom_right.x, b.top_left.y) };
			for (const glm::vec2& p : corners)
				result = (std::max)(result, glm::distance(from, p));
		}
		return result * 1.5f + 1.0f;
	}

	// calls `visit` with every intersection of the segment of the entity (from its point to `dst`) with the boundaries
	template <class Visit>
	void intersections(Segment& segment, glm::vec2 dst, std::vector<Curve>& boundaries, Visit visit)
	{
		for (Curve& c : boundaries)
		{
			if (c.size() < 2)
				continue;
			for (size_t i = 0; i + 1 < c.size(); i++)
			{
				glm::vec2 hits[2];
				int count = segment.intersect(dst, c[i], c[i + 1].point, hits);
				for (int h = 0; h < count; h++)
					visit(hits[h]);
			}
		}
	}
}

bool cad_extend::line(glm::vec2 fixed, glm::vec2 end, std::vector<Curve>& boundaries, glm::vec2& result)
{
	glm::vec2 delta = end - fixed;
	float length = glm::length(delta);
	if (length < geometry::ERR_FLOAT)
		return false;
	glm::vec2 direction = delta / length;

	Segment extension(end);
	glm::vec2 far_point = end + direction * reach(end, boundaries);

	float best = (std::numeric_limits<float>::max)();
	intersections(extension, far_point, boundaries, [&](glm::vec2 p)
	{
		float t = glm::dot(p - end, direction);
		if (t > MIN_DISTANCE && t < best)
		{
			best = t;
			result = p;
		}
	});

	return best != (std::numeric_limits<float>::max)();
}

bool cad_extend::arc(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, std::vector<Curve>& boundaries, glm::vec2& result)
{
	float radius = glm::distance(stop, center);
	if (radius < geometry::ERR_FLOAT || glm::distance(start, stop) < geometry::ERR_FLOAT)
		return false;

	// the prolongation goes on from stop, in the direction of the arc, up to the start: the whole rest of the circle
	Segment extension(stop, center, radius, cw, SegmentType::Arc);
	float stop_angle = geometry::oriented_angle(stop, center);

	float best = (std::numeric_limits<float>::max)();
	intersections(extension, start, boundaries, [&](glm::vec2 p)
	{
		float a = geometry::oriented_angle(p, center);
		float sweep = cw ? geometry::oriented_angle(stop_angle - a) : geometry::oriented_angle(a - stop_angle);
		if (sweep > MIN_ANGLE && sweep < best)
		{
			best = sweep;
			result = p;
		}
	});

	return best != (std::numeric_limits<float>::max)();
}
