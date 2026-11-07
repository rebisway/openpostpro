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

// The material of the simulator: a height for each column of a grid (a block seen from above). The tool lowers the heights along
// its path. The heights are given to the graphic card as a texture: the zone which changed is known (take_dirty), so only that zone
// is sent again.
//
// Same method as the simulator of GGCad (a grid of columns lowered under the tool), but each column is one number and the shape of
// the tool is a profile: the height of its surface above its tip at a distance of its axis.

#pragma once
#ifndef _SIM_MATERIAL_H
#define _SIM_MATERIAL_H

#include "sim_types.h"

class SimMaterial
{
private:
	double _origin_x = 0, _origin_y = 0;	// position of the column (0, 0)
	double _step = 1.0;
	int _width = 0, _height = 0;			// number of columns
	double _top = 0, _bottom = -1;			// heights of the block
	std::vector<float> _heights;

	bool _dirty = false;
	int _x0 = 0, _y0 = 0, _x1 = -1, _y1 = -1;	// the zone which changed, inclusive

	void cut_piece(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool);
	void touch(int x, int y);

public:
	// The block: its corners in the plane, the heights of its top and of its bottom, the step between two columns
	void setup(const glm::dvec2& low, const glm::dvec2& high, double top, double bottom, double step);
	// Back to the block as it was before the machining
	void reset();

	// The tool goes from a to b (positions of its tip)
	void cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool);

	// The height of the surface of a tool above its tip, at a distance of its axis (a large number outside of the tool)
	static double profile(const SimTool& tool, double distance);

	// The step of the grid: a part (precision, in percent) of the radius of the tool, made larger if there would be too many columns
	static double choose_step(double span_x, double span_y, double tool_radius, double precision, int max_cells);

	int width() const { return _width; }
	int height() const { return _height; }
	double step() const { return _step; }
	double origin_x() const { return _origin_x; }
	double origin_y() const { return _origin_y; }
	double top() const { return _top; }
	double bottom() const { return _bottom; }
	const std::vector<float>& data() const { return _heights; }
	float at(int x, int y) const { return _heights[(size_t)y * _width + x]; }
	// The height of the block under a point of the plane (the nearest column)
	double height_at(double x, double y) const;
	// The volume taken away, in cubic millimeters
	double removed_volume() const;

	// The zone which changed since the last call (columns, inclusive). False if nothing changed.
	bool take_dirty(int& x0, int& y0, int& x1, int& y1);
	void mark_all_dirty();
};

#endif
