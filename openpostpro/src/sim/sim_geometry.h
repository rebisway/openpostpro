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

// The path of a move of the simulator: a straight line or an arc (a circle in a plane, with a helix when the third axis changes).

#pragma once
#ifndef _SIM_GEOMETRY_H
#define _SIM_GEOMETRY_H

#include "sim_types.h"

class SimPath
{
private:
	glm::dvec3 _from;
	glm::dvec3 _to;
	bool _arc = false;
	glm::dvec3 _center;
	int _a = 0, _b = 1, _c = 2;		// the indices of the two axes of the plane and of the third one
	double _radius_from = 0, _radius_to = 0;
	double _angle = 0;				// angle of the start
	double _sweep = 0;				// signed angle swept (negative when clockwise)
	double _length = 0;

public:
	SimPath() {}
	SimPath(const glm::dvec3& from, const SimCommand& command);

	bool arc() const { return _arc; }
	double length() const { return _length; }
	// the point at a fraction (0 to 1) of the path
	glm::dvec3 at(double t) const;
	glm::dvec3 from() const { return _from; }
	glm::dvec3 to() const { return _to; }
	double sweep() const { return _sweep; }
	double radius() const { return (_radius_from + _radius_to) / 2.0; }

	// The number of straight pieces which follow the path at less than `tolerance` (1 for a line)
	int pieces(double tolerance) const;

	// The extreme points of the path (the corners of the box which holds it), used for the bounds of the program
	void bounds(glm::dvec3& low, glm::dvec3& high) const;
};

#endif
