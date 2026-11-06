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

// Cut: finds which shapes are cut by a click, and where. The cutting itself is done by cad::split().

#pragma once
#ifndef _CUT_SHAPE_H
#define _CUT_SHAPE_H

#include <layer.h>
#include <curve.h>
#include <vector>

namespace cad_cut
{
	struct Cut
	{
		Shape* shape = nullptr;
		glm::vec2 point;		// where the shape is cut, always on the shape
	};

	// The point of the curve which is nearest to `point`. Returns false if this point is an end of an open curve
	// (nothing to cut there).
	bool nearest(Curve& curve, glm::vec2 point, glm::vec2& result);

	// All the intersections of two curves.
	std::vector<glm::vec2> intersections(Curve& a, Curve& b);

	// A polyline (list of points) cut at `cut`, a point of it: the two pieces, or one piece for a closed polyline (it is
	// opened at that point). Empty if `cut` is an end of an open polyline or is not on it.
	std::vector<std::vector<glm::vec2>> split_points(const std::vector<glm::vec2>& points, glm::vec2 cut);

	// A line, arc, circle, polyline or ellipse cut at `cut`, as new shapes of the same layer: two pieces, or one for a circle (an arc
	// all the way around) and for a closed polyline (opened at the cut). Empty if it can not be cut.
	// The original shape is left as it is.
	std::vector<Shape*> split_shape(Shape* shape, glm::vec2 cut);

	// The shapes cut by a click: every visible line, arc, circle, polyline or ellipse under the mouse position `mouse` or
	// under `snapped` (the position of the click once the magnet is applied, the same as mouse without magnet).
	// When several shapes are under the click, they are cut at the intersection of the first two which is nearest to the
	// click, as the user aims at a crossing. Otherwise the shape is cut at the point of it which is nearest to `snapped`.
	// Shapes for which that point is an end are not returned.
	std::vector<Cut> find(std::vector<Layer*>& layers, glm::vec2 mouse, glm::vec2 snapped);
}

#endif
