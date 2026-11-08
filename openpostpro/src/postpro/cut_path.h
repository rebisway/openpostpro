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
#include <string>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <curve.h>

// Moves of one curve at one depth, written through a post-processor object. It is a template so that the sequence of the moves (the plunge
// especially) can be tested without the Python post-processor: `Post` has rapid(vec3), linear(vec3), circular(vec3, center, cw), feed(float),
// start_single_path() and stop_single_path() which return text.

struct CutPathParams
{
	float z = 0;				// depth of the pass
	float z_top = 0;			// height where the tool is when the pass starts (the surface, or the depth of the previous pass)
	float safe = 5;				// the tool is at this height at the first point of the curve
	float plunge_feed = 500;
	float feed = 1000;
	float tab_z = 0;			// height of the tabs
	// the first segment of the curve is an entry (offset start): the tool goes down along it instead of vertically
	bool oblique = false;
};

// The plunge. Vertical by default: straight down from the first point. Oblique: the tool goes down to the height of the surface above
// the first point, then the first segment of the curve (the entry) is done going down, so that it arrives at the depth of the pass at the
// end of the entry. Returns true when the first segment is done by the plunge (the path goes on with the second one).
template <class Post>
bool plunge(Post& post, std::string& output, const Curve& c, const CutPathParams& p)
{
	const float top = (std::min)(p.z_top, p.safe);
	const bool ramp = p.oblique && c.size() > 1 && top > p.z && glm::distance(c[0].point, c[1].point) > 0.0001f;

	output += post.feed(p.plunge_feed);
	if (!ramp)
	{
		output += post.linear(glm::vec3(c[0].point.x, c[0].point.y, p.z));
		return false;
	}

	// down to the surface (the tool is above the material: it does not cut), then the entry which goes down
	if (top < p.safe)
		output += post.rapid(glm::vec3(c[0].point.x, c[0].point.y, top));
	if (c[0].type == SegmentType::Line)
		output += post.linear(glm::vec3(c[1].point, p.z));
	else
		output += post.circular(glm::vec3(c[1].point, p.z), c[0].center, c[0].cw);
	return true;
}

// The curve at the depth of the pass, from the plunge to its end
template <class Post>
std::string cut_path(Post& post, const Curve& c, const CutPathParams& p)
{
	std::string output;

	const bool ramped = plunge(post, output, c, p);
	output += post.feed(p.feed);

	output += post.start_single_path();

	auto from = ramped ? c[1] : c.front();
	auto next = c.begin() + (ramped ? 2 : 1);
	float last_z = p.z;

	while (next != c.end())
	{
		float seg_z = from.tab ? (std::max)(p.z, p.tab_z) : p.z;
		if (seg_z != last_z)
		{
			output += post.linear(glm::vec3(from.point, seg_z));
			last_z = seg_z;
		}
		if (from.type == SegmentType::Line)
			output += post.linear(glm::vec3((*next).point, seg_z));
		else
			output += post.circular(glm::vec3((*next).point, seg_z), from.center, from.cw);
		from = (*next);
		next = std::next(next);
	}
	return output;
}
