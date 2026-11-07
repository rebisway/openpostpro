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

#include "sim_material.h"
#include <algorithm>
#include <cmath>
#include <limits>

static const double PI = 3.14159265358979323846;

void SimMaterial::setup(const glm::dvec2& low, const glm::dvec2& high, double top, double bottom, double step)
{
	_step = std::max(step, 1e-4);
	_origin_x = low.x;
	_origin_y = low.y;
	_top = top;
	_bottom = std::min(bottom, top);
	_width = std::max(2, (int)std::ceil((high.x - low.x) / _step) + 1);
	_height = std::max(2, (int)std::ceil((high.y - low.y) / _step) + 1);
	reset();
}

void SimMaterial::reset()
{
	_heights.assign((size_t)_width * _height, (float)_top);
	mark_all_dirty();
}

void SimMaterial::touch(int x, int y)
{
	if (!_dirty)
	{
		_x0 = _x1 = x;
		_y0 = _y1 = y;
		_dirty = true;
		return;
	}
	_x0 = std::min(_x0, x);
	_x1 = std::max(_x1, x);
	_y0 = std::min(_y0, y);
	_y1 = std::max(_y1, y);
}

void SimMaterial::mark_all_dirty()
{
	_dirty = _width > 0 && _height > 0;
	_x0 = 0;
	_y0 = 0;
	_x1 = _width - 1;
	_y1 = _height - 1;
}

bool SimMaterial::take_dirty(int& x0, int& y0, int& x1, int& y1)
{
	if (!_dirty)
		return false;
	x0 = _x0;
	y0 = _y0;
	x1 = _x1;
	y1 = _y1;
	_dirty = false;
	return true;
}

double SimMaterial::profile(const SimTool& tool, double d)
{
	const double outside = 1e30;
	double r = tool.radius;
	if (d > r)
		return outside;

	switch (tool.shape)
	{
	case SimToolShape::Flat:
	case SimToolShape::Plasma:
		return 0.0;
	case SimToolShape::Ball:
		return r - std::sqrt(std::max(r * r - d * d, 0.0));
	case SimToolShape::Torus:
	{
		double rc = std::clamp(tool.small_radius, 0.0, r);
		double inner = r - rc;
		if (d <= inner)
			return 0.0;
		double e = d - inner;
		return rc - std::sqrt(std::max(rc * rc - e * e, 0.0));
	}
	case SimToolShape::Cone:
	{
		double tip = std::clamp(tool.small_radius, 0.0, r);
		double half = std::clamp(tool.angle, 2.0, 178.0) * PI / 360.0;
		if (d <= tip)
			return 0.0;
		return (d - tip) / std::tan(half);
	}
	}
	return 0.0;
}

double SimMaterial::choose_step(double span_x, double span_y, double tool_radius, double precision, int max_cells)
{
	double step = std::max(tool_radius * precision / 100.0, 0.01);
	double cells = (span_x / step + 1.0) * (span_y / step + 1.0);
	if (max_cells > 0 && cells > max_cells)
		step *= std::sqrt(cells / max_cells);
	return step;
}

double SimMaterial::height_at(double x, double y) const
{
	if (_width == 0)
		return _top;
	int ix = (int)std::lround((x - _origin_x) / _step);
	int iy = (int)std::lround((y - _origin_y) / _step);
	ix = std::clamp(ix, 0, _width - 1);
	iy = std::clamp(iy, 0, _height - 1);
	return at(ix, iy);
}

double SimMaterial::removed_volume() const
{
	double sum = 0;
	for (float h : _heights)
		sum += _top - h;
	return sum * _step * _step;
}

void SimMaterial::cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool)
{
	if (_heights.empty())
		return;

	// long moves are cut by pieces, so that the zone looked at for each piece stays small
	double length = glm::distance(glm::dvec2(a), glm::dvec2(b));
	double piece = std::max(tool.radius, 6.0 * _step);
	int count = std::max(1, (int)std::ceil(length / piece));
	for (int i = 0; i < count; i++)
	{
		glm::dvec3 p0 = a + (b - a) * ((double)i / count);
		glm::dvec3 p1 = a + (b - a) * ((double)(i + 1) / count);
		cut_piece(p0, p1, tool);
	}
}

void SimMaterial::cut_piece(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool)
{
	double r = tool.radius;
	if (r <= 0)
		return;

	double low_x = std::min(a.x, b.x) - r, high_x = std::max(a.x, b.x) + r;
	double low_y = std::min(a.y, b.y) - r, high_y = std::max(a.y, b.y) + r;
	int x0 = std::max(0, (int)std::floor((low_x - _origin_x) / _step));
	int x1 = std::min(_width - 1, (int)std::ceil((high_x - _origin_x) / _step));
	int y0 = std::max(0, (int)std::floor((low_y - _origin_y) / _step));
	int y1 = std::min(_height - 1, (int)std::ceil((high_y - _origin_y) / _step));
	if (x0 > x1 || y0 > y1)
		return;

	glm::dvec2 d(b.x - a.x, b.y - a.y);
	double l2 = glm::dot(d, d);
	bool through = tool.shape == SimToolShape::Plasma;
	// the profile of a flat tool does not depend on the distance: no need of the square root
	bool flat = tool.shape == SimToolShape::Flat || tool.shape == SimToolShape::Plasma;
	// a move at the same height has one candidate, a plunge or a ramp has its two ends too
	bool level = std::fabs(b.z - a.z) < 1e-12;
	double r2 = r * r;
	float bottom = (float)_bottom;

	for (int iy = y0; iy <= y1; iy++)
	{
		double py = _origin_y + iy * _step;
		for (int ix = x0; ix <= x1; ix++)
		{
			double px = _origin_x + ix * _step;

			// the nearest point of the move, and its two ends (a plunge only has the lowest end)
			double t = l2 < 1e-18 ? 0.0 : std::clamp(((px - a.x) * d.x + (py - a.y) * d.y) / l2, 0.0, 1.0);
			double best = std::numeric_limits<double>::max();
			bool touched = false;
			int candidates = level ? 1 : 3;
			double ts[3] = { t, 0.0, 1.0 };
			for (int k = 0; k < candidates; k++)
			{
				double tt = ts[k];
				double ex = px - (a.x + d.x * tt), ey = py - (a.y + d.y * tt);
				double dist2 = ex * ex + ey * ey;
				if (dist2 > r2)
					continue;
				touched = true;
				double z = a.z + (b.z - a.z) * tt;
				best = std::min(best, flat ? z : z + profile(tool, std::sqrt(dist2)));
			}
			if (!touched)
				continue;

			float h = through ? bottom : (float)std::max(best, _bottom);
			size_t index = (size_t)iy * _width + ix;
			if (h < _heights[index])
			{
				_heights[index] = h;
				touch(ix, iy);
			}
		}
	}
}
