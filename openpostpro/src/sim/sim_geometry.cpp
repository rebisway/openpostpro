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

#include "sim_geometry.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>


SimPath::SimPath(const glm::dvec3& from, const SimCommand& command)
{
	_from = from;
	_to = command.target;

	if (command.type != SimCommandType::Arc)
	{
		_length = glm::distance(_from, _to);
		return;
	}

	_arc = true;
	_center = command.center;

	// the two axes of the plane and the third one
	switch (command.plane)
	{
	case SimPlane::XY: _a = 0; _b = 1; _c = 2; break;
	case SimPlane::XZ: _a = 2; _b = 0; _c = 1; break;
	case SimPlane::YZ: _a = 1; _b = 2; _c = 0; break;
	}

	double fa = _from[_a] - _center[_a], fb = _from[_b] - _center[_b];
	double ta = _to[_a] - _center[_a], tb = _to[_b] - _center[_b];
	_radius_from = std::hypot(fa, fb);
	_radius_to = std::hypot(ta, tb);
	_angle = std::atan2(fb, fa);

	double end = std::atan2(tb, ta);
	double sweep = end - _angle;
	if (command.cw)
	{
		while (sweep >= 0)
			sweep -= glm::two_pi<double>();
		// the start and the end are the same point: a full circle
		if (std::fabs(sweep) < 1e-9)
			sweep = -glm::two_pi<double>();
		if (sweep < -glm::two_pi<double>())
			sweep += glm::two_pi<double>();
	}
	else
	{
		while (sweep <= 0)
			sweep += glm::two_pi<double>();
		if (std::fabs(sweep) < 1e-9)
			sweep = glm::two_pi<double>();
		if (sweep > glm::two_pi<double>())
			sweep -= glm::two_pi<double>();
	}
	// a start and an end which are the same point give a full circle, not a null arc
	if (glm::distance(_from, _to) < 1e-9 && std::fabs(sweep) < 1e-6)
		sweep = command.cw ? -glm::two_pi<double>() : glm::two_pi<double>();
	_sweep = sweep;

	// the length is the one of the arc, with the helix if the third axis changes
	double flat = std::fabs(_sweep) * (_radius_from + _radius_to) / 2.0;
	double rise = _to[_c] - _from[_c];
	_length = std::hypot(flat, rise);
}

glm::dvec3 SimPath::at(double t) const
{
	if (!_arc)
		return _from + (_to - _from) * t;
	if (t <= 0)
		return _from;
	if (t >= 1)
		return _to;

	double angle = _angle + _sweep * t;
	double radius = _radius_from + (_radius_to - _radius_from) * t;
	glm::dvec3 p;
	p[_a] = _center[_a] + radius * std::cos(angle);
	p[_b] = _center[_b] + radius * std::sin(angle);
	p[_c] = _from[_c] + (_to[_c] - _from[_c]) * t;
	return p;
}

int SimPath::pieces(double tolerance) const
{
	if (!_arc)
		return 1;
	double r = std::max(_radius_from, _radius_to);
	if (r < 1e-9)
		return 1;
	double step = tolerance >= r ? glm::pi<double>() : 2.0 * std::acos(1.0 - tolerance / r);
	step = std::max(step, 0.01);
	return std::max(1, (int)std::ceil(std::fabs(_sweep) / step));
}

void SimPath::bounds(glm::dvec3& low, glm::dvec3& high) const
{
	low = glm::min(_from, _to);
	high = glm::max(_from, _to);
	if (!_arc)
		return;

	// the points of the circle on the axes which are inside of the arc
	for (int k = 0; k < 4; k++)
	{
		double axis_angle = k * glm::half_pi<double>();
		double d = axis_angle - _angle;
		double s = _sweep;
		bool inside;
		if (s >= 0)
		{
			while (d < 0) d += glm::two_pi<double>();
			while (d >= glm::two_pi<double>()) d -= glm::two_pi<double>();
			inside = d <= s;
		}
		else
		{
			while (d > 0) d -= glm::two_pi<double>();
			while (d <= -glm::two_pi<double>()) d += glm::two_pi<double>();
			inside = d >= s;
		}
		if (inside)
		{
			double t = d / s;
			glm::dvec3 p = at(t);
			low = glm::min(low, p);
			high = glm::max(high, p);
		}
	}
}
