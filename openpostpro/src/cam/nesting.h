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

// Automatic nesting: places parts on a rectangular sheet without overlap.
// The engine only knows polygons (arcs are sampled by the caller). A part is an outer ring and the rings inside it: holes, then
// islands inside the holes and so on. A small part can be placed inside the hole of a big one. The spacing between parts is
// obtained by the caller, who gives the rings already inflated by half of it (Curve::offset).

#pragma once
#ifndef _NESTING_H
#define _NESTING_H

#include <glm/glm.hpp>
#include <vector>

namespace nesting
{
	// closed ring: the last point is joined to the first one
	using Ring = std::vector<glm::vec2>;

	struct Part
	{
		Ring outer;
		// the rings inside the outer one, at any depth: the material is where a point is inside an odd number of rings (the outer
		// one counts as the first)
		std::vector<Ring> inner;
		int quantity = 1;
	};

	struct Options
	{
		float width = 1000;
		float height = 500;
		// free border all around the sheet
		float margin = 5;
		// distance between two candidate positions of the first scan (the position is then refined down to precision)
		float step = 8;
		float precision = 0.1f;
		// rotations tried for each part, in degrees
		std::vector<float> angles = { 0, 90, 180, 270 };
	};

	// where a part is: a point p of the part goes to R(angle) * p + offset
	struct Placement
	{
		int part = -1;
		bool placed = false;
		float angle = 0;
		glm::vec2 offset = glm::vec2();
	};

	// the placed ring of a part
	Ring transform(const Ring& ring, float angle, glm::vec2 offset);

	// true if the two parts, each placed with its own transformation, share some material
	bool overlap(const Part& a, const Placement& pa, const Part& b, const Placement& pb);

	// area of the material of a part (the outer ring without its holes, plus its islands...)
	float area(const Part& part);

	// places each part (as many times as its quantity). One Placement per instance, in the order of placement.
	std::vector<Placement> nest(const std::vector<Part>& parts, const Options& options);

	// length of the sheet used: the right edge of the farthest part
	float length(const std::vector<Part>& parts, const std::vector<Placement>& placements);

	// used area / area of the sheet
	float utilisation(const std::vector<Part>& parts, const std::vector<Placement>& placements, const Options& options);
}

#endif
