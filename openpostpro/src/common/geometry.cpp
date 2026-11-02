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

#include <geometry.h>

namespace geometry
{
/// <summary>
/// Return angle in radiant between -pi and pi with horizontal
/// </summary>
/// <param name="p">Point to test</param>
/// <returns>Angle in radiant</returns>
float angle(glm::vec2 p)
{
	if (p.x == 0)
	{
		if (p.y > 0)
			return glm::half_pi<float>();
		else
			return glm::pi<float>() + glm::half_pi<float>();
	}
	else
		return glm::atan(p.y, p.x);
}

/// <summary>
/// Return angle in radiant between -pi and pi with horizontal and origin
/// </summary>
/// <param name="p">Point to test</param>
/// <param name="center">offset center</param>
/// <returns>Angle in radiant</returns>
float angle(glm::vec2 p, glm::vec2 origin)
{
	return angle(p - origin);
}

float oriented_angle(glm::vec2 p1, glm::vec2 p2, glm::vec2 origin)
{
	auto cross = (p1.x-origin.x)*(p2.y-origin.y) - (p2.x - origin.x) * (p1.y - origin.y);
	auto cw = cross > 0;
	return oriented_angle(p1, p2, origin, cw);
}

/// <summary>
/// Return angle in radian between 0 and 2*PI
/// </summary>
/// <param name="radian">angle</param>
/// <returns></returns>
float oriented_angle(float radian)
{
	while (radian > glm::two_pi<float>())
		radian -= glm::two_pi<float>();
	while (radian < 0)
		radian += glm::two_pi<float>();
	return radian;
}

/// <summary>
/// Return angle in radian between 0 and 2*PI
/// </summary>
/// <param name="p">Coordinates to test. Origin is (0,0).</param>
/// <returns></returns>
float oriented_angle(glm::vec2 p)
{
	if (p.x == 0)
	{
		if (p.y > 0)
			return glm::half_pi<float>();
		else
			return glm::pi<float>() + glm::half_pi<float>();
	}

	return oriented_angle(glm::atan(p.y, p.x));
}

/// <summary>
/// Return angle in radian between 0 and 2*PI
/// </summary>
/// <param name="p">Coordinates to test</param>
/// <param name="origin"></param>
/// <returns></returns>
float oriented_angle(glm::vec2 p, glm::vec2 origin)
{
	return oriented_angle(p - origin);
}


float oriented_angle(float a_start, float a_stop, bool cw)
{
	float angle = 0;
	if (cw)
	{
		if (a_start > a_stop)
			angle = a_start - a_stop;
		else
			angle = a_start + glm::two_pi<float>() - a_stop;
	}
	else
	{
		if (a_stop > a_start)
			angle = a_stop - a_start;
		else
			angle = glm::two_pi<float>() - a_start + a_stop;
	}

	return angle;
}

float oriented_angle(glm::vec2 p1, glm::vec2 p2, bool cw)
{
	return oriented_angle(p1, p2, geometry::vec2_empty, cw);
}

float oriented_angle(glm::vec2 p1, glm::vec2 p2, glm::vec2 origin, bool cw)
{
	auto a_start = geometry::oriented_angle(geometry::oriented_angle(p1, origin));
	auto a_stop = geometry::oriented_angle(geometry::oriented_angle(p2, origin));

	return oriented_angle(a_start, a_stop, cw);
}

/// <summary>
/// Compute the square distance between 2 points
/// </summary>
/// <param name="p1">first ref_point</param>
/// <param name="p2">second ref_point</param>
/// <returns>the square distance</returns>
float distance(glm::vec2 p1, glm::vec2 p2)
{
	float dx = p1.x - p2.x;
	float dy = p1.y - p2.y;
	return glm::sqrt(dx * dx + dy * dy);
}

/// <summary>
/// Compute the square distance between 2 points
/// </summary>
/// <param name="p1">first ref_point</param>
/// <param name="p2">second ref_point</param>
/// <returns>the square distance</returns>
float distance2(glm::vec2 p1, glm::vec2 p2)
{
	float dx = p1.x - p2.x;
	float dy = p1.y - p2.y;
	return dx * dx + dy * dy;
}

/// <summary>
/// Return square distance between ref_point to line going through p1 and p2
/// </summary>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <param name="ref_point"></param>
/// <returns></returns>
float foretriangle2(glm::vec2 p1, glm::vec2 p2, glm::vec2 point)
{
	// vertical
	if (glm::abs(p1.x - p2.x) < geometry::ERR_FLOAT3)
		return (point.x - p1.x) * (point.x - p1.x);

	// horizonal
	if (glm::abs(p1.y - p2.y) < geometry::ERR_FLOAT3)
		return (point.y - p1.y) * (point.y - p1.y);

	auto h = projection(point, p1, p2);

	return distance2(h, point);
}

/// <summary>
/// Return distance between ref_point to line going through p1 and p2
/// </summary>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <param name="ref_point"></param>
/// <returns></returns>
float foretriangle(glm::vec2& p1, glm::vec2& p2, glm::vec2& point)
{
	// vertical
	if (glm::abs(p1.x - p2.x) < geometry::ERR_FLOAT3)
		return glm::abs(point.x - p1.x);

	// horizonal
	if (glm::abs(p1.y - p2.y) < geometry::ERR_FLOAT3)
		return glm::abs(point.y - p1.y);

	return glm::sqrt(foretriangle2(p1, p2, point));
}

/// <summary>
/// Test if ref_point inside rectangle
/// </summary>
/// <param name="top_left"></param>
/// <param name="bottom_right"></param>
/// <param name="ref_point"></param>
/// <returns></returns>
bool rectangle_contains(glm::vec2& top_left, glm::vec2& bottom_right, glm::vec2& point)
{
	return (point.x >= top_left.x && point.x <= bottom_right.x && point.y >= bottom_right.y && point.y <= top_left.y);
}

bool rectangle_contains(rectangle& r, glm::vec2& point)
{
	return (point.x >= r.top_left.x && point.x <= r.bottom_right.x && point.y >= r.bottom_right.y && point.y <= r.top_left.y);
}

bool rectangle_contains(float top, float left, float bottom, float right, glm::vec2& point)
{
	return (point.x >= left && point.x <= right && point.y >= bottom && point.y <= top);
}

/// <summary>
/// Test if rectangle 2 intersect or inside rectangle 1
/// </summary>
/// <param name="top_left1"></param>
/// <param name="bottom_right1"></param>
/// <param name="top_left2"></param>
/// <param name="bottom_right2"></param>
/// <returns></returns>
bool rectangle_contains(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2)
{
	// check if 2 is outside 1
	return rectangle_contains(top_left1, bottom_right1, top_left2) && rectangle_contains(top_left1, bottom_right1, bottom_right2);
}

bool rectangle_contains(rectangle& r1, rectangle& r2)
{
	return rectangle_contains(r1.top_left, r1.bottom_right, r2.top_left) && rectangle_contains(r1.top_left, r1.bottom_right, r2.bottom_right);
}

bool rectangle_intersect(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2)
{
	// check if 2 is outside 1
	return (top_left1.x < bottom_right2.x && bottom_right1.x > top_left2.x && top_left1.y > bottom_right2.y && bottom_right1.y < top_left2.y);
}

bool rectangle_intersect(rectangle& r1, rectangle& r2)
{
	return (r1.top_left.x < r2.bottom_right.x && r1.bottom_right.x > r2.top_left.x && r1.top_left.y > r2.bottom_right.y && r1.bottom_right.y < r2.top_left.y);
}

bool rectangle_outside(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2)
{
	if (top_left1.y < bottom_right2.y ||	// rect2 over rect1
		bottom_right1.y > top_left2.y ||	// rect2 under rect 1
		top_left1.x > bottom_right2.x ||	// rect2 on left of rect1
		bottom_right1.x < top_left2.x)		// rect2 on right of rect1
		return true;

	return false;
}

bool rectangle_outside(rectangle& r1, rectangle& r2)
{
	if (r1.top_left.y < r2.bottom_right.y ||	// rect2 over rect1
		r1.bottom_right.y > r2.top_left.y ||	// rect2 under rect 1
		r1.top_left.x > r2.bottom_right.x ||	// rect2 on left of rect1
		r1.bottom_right.x < r2.top_left.x)		// rect2 on right of rect1
		return true;

	return false;
}

/// <summary>
/// Interpolate circle vertices
/// </summary>
/// <param name="radius"></param>
/// <param name="segment_length"></param>
/// <returns></returns>
std::vector<glm::vec2> circle(float radius, float segment_length)
{
	float angle_inc = segment_length / radius;

	std::vector<glm::vec2> one;

	for (float angle = 0; angle < glm::pi<float>() * 2; angle += angle_inc)
	{
		one.push_back(glm::vec2(radius * glm::cos(angle), radius * glm::sin(angle)));
	}

	return one;
}

std::vector<glm::vec2> circle(float radius, glm::vec2 center, float segment_length)
{
	float angle_inc = segment_length / radius;

	std::vector<glm::vec2> one;

	for (float angle = 0; angle < glm::pi<float>() * 2; angle += angle_inc)
	{
		one.push_back(glm::vec2(radius * glm::cos(angle) + center.x, radius * glm::sin(angle) + center.y));
	}

	return one;
}

/// <summary>
/// Interpolate arc vertices
/// </summary>
/// <param name="source"></param>
/// <param name="center"></param>
/// <param name="destination"></param>
/// <param name="cw"></param>
/// <param name="segment_length"></param>
/// <returns></returns>
std::vector<glm::vec2> arc(glm::vec2 source, glm::vec2 center, glm::vec2 destination, bool cw, float segment_length) {
	std::vector<glm::vec2> l;

	// compute arc ref_point coordinates
	auto a_p = [](float r, float a, glm::vec2 c) {
		return glm::vec2(c.x + r * glm::cos(a), c.y + r * glm::sin(a));
		};

	float r = glm::distance(source, center);
	glm::vec2 src = source - center;
	glm::vec2 dst = destination - center;
	float inc = segment_length / r;
	float a1 = oriented_angle(src);
	float a2 = oriented_angle(dst);

	l.push_back(source);

	if (cw)
	{
		if (a1 < a2)
			a1 += glm::two_pi<float>();
		for (float a = a1 - inc; a > a2; a -= inc)
			l.push_back(a_p(r, a, center));
	}
	else
	{
		if (a1 >= a2)
			a2 += glm::two_pi<float>();
		for (float a = a1 + inc; a < a2; a += inc)
			l.push_back(a_p(r, a, center));
	}

	l.push_back(destination);

	return l;
}

std::vector<glm::vec2> ellipse(float minor, float major, float start, float stop, float segment_length)
{
	std::vector<glm::vec2> vertices;

	if (minor == 0 || major == 0)
		return vertices;

	float a = major / 2.0f;
	float b = minor / 2.0f;

	float inc = segment_length / 10.0f, ang = 0;

	float e = stop;
	while (start > e) // we ensure stop angle > start angle, add mod(2*pi)
		e += glm::two_pi<float>();
	float s = start;

	while (s > glm::two_pi<float>())
		s -= glm::two_pi<float>();
	while (e > glm::two_pi<float>())
		e -= glm::two_pi<float>();


	for (ang = s; ang <= e; ang += inc)
	{
		glm::vec2 p((a * glm::cos(ang)), (b * glm::sin(ang)));
		vertices.push_back(p);
	}
	glm::vec2 p((a * glm::cos(stop)), (b * sin(stop)));
	vertices.push_back(p);

	if (start == 0 && stop == glm::two_pi<float>())
		vertices.push_back(vertices.front());

	return vertices;
}

std::vector<glm::vec2> ellipse(glm::vec2 center, float minor, float major, float start, float stop, float angle, float segment_length)
{
	std::vector<glm::vec2> vertices = ellipse(minor, major, start, stop, segment_length);

	glm::mat4x4 mat(1.0f);

	if (angle != 0)
		mat = glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));

	mat = glm::translate(glm::vec3(center.x, center.y, 1.0f)) * mat;

	for (int i = 0; i < vertices.size(); i++)
		vertices[i] = mat * glm::vec4(vertices[i].x, vertices[i].y, 1, 1);

	return vertices;
}

std::vector<glm::vec2> bezier_from_line(glm::vec2 from, glm::vec2 to)
{
	return std::vector<glm::vec2>({ from, to, from, to });
}

std::vector<glm::vec2> bezier_from_QBSpline(std::vector<glm::vec2> spline)
{
	std::vector<glm::vec2> vertices;

	// Convert the Quadratic points to integer
	glm::vec2 p0(spline[0]);        // Quadratic on curve start ref_point
	glm::vec2 p1(spline[1]);         // Quadratic control ref_point
	glm::vec2 p2(spline[2]);         // Quadratic on curve end ref_point

	// conversion of a quadratic to a cubic

	// Cubic p0 is the on curve start ref_point
	vertices.push_back(p0);

	// Cubic p1 in terms of Quadratic p0 and p1
	vertices.push_back(glm::vec2(p0.x + 2 * (p1.x - p0.x) / 3.0f, p0.y + 2 * (p1.y - p0.y) / 3.0f));

	// Cubic p2 in terms of Qudartic p1 and p2
	vertices.push_back(glm::vec2(p1.x + 1 * (p2.x - p1.x) / 3.0f, p1.y + 1 * (p2.y - p1.y) / 3.0f));

	// Cubic p3 is the on curve end ref_point
	vertices.push_back(p2);

	return vertices;
}

rectangle arc_bounds(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw)
{
	float radius = geometry::distance(start, center);

	return arc_bounds(start, center, stop, radius, cw);
}

rectangle arc_bounds(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw)
{
	rectangle result;

	float start_a = geometry::oriented_angle(start, center);
	float stop_a = geometry::oriented_angle(stop, center);
	float three_forth_pi = glm::half_pi<float>() + glm::pi<float>();

	result.top_left.x = glm::min(start.x, stop.x);
	result.top_left.y = glm::max(start.y, stop.y);
	result.bottom_right.x = glm::max(start.x, stop.x);
	result.bottom_right.y = glm::min(start.y, stop.y);

	if (cw)
	{
		float pivot = start_a;
		start_a = stop_a;
		stop_a = pivot;
		cw = !cw;
	}

	if (start_a < stop_a)
	{
		if (start_a < glm::half_pi<float>())
		{
			if (stop_a >= three_forth_pi)
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
				result.bottom_right.y = center.y - radius;
			}
			else if (stop_a >= glm::pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
			}
			else if (stop_a >= glm::half_pi<float>())
			{
				result.top_left.y = center.y + radius;
			}
		}
		else if (start_a < glm::pi<float>())
		{
			if (stop_a >= three_forth_pi)
			{
				result.top_left.x = center.x - radius;
				result.bottom_right.y = center.y - radius;
			}
			else if (stop_a >= glm::pi<float>())
			{
				result.top_left.x = center.x - radius;
			}
		}
		else if (start_a < three_forth_pi)
		{
			if (stop_a >= three_forth_pi)
			{
				result.bottom_right.y = center.y - radius;
			}
		}
	}
	else
	{
		if (start_a >= three_forth_pi)
		{
			if (stop_a >= three_forth_pi)
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
				result.bottom_right.y = center.y - radius;
				result.bottom_right.x = center.x + radius;
			}
			else if (stop_a >= glm::pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
				result.bottom_right.x = center.x + radius;
			}
			else if (stop_a >= glm::half_pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.bottom_right.x = center.x + radius;
			}
			else if (stop_a >= 0)
			{
				result.bottom_right.x = center.x + radius;
			}
		}
		else if (start_a >= glm::pi<float>())
		{
			if (stop_a >= glm::pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
				result.bottom_right.y = center.y - radius;
				result.bottom_right.x = center.x + radius;
			}
			else if (stop_a >= glm::half_pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.bottom_right.x = center.x + radius;
				result.bottom_right.y = center.y - radius;
			}
			else if (stop_a >= 0)
			{
				result.bottom_right.x = center.x + radius;
				result.bottom_right.y = center.y - radius;
			}
		}
		else if (start_a >= glm::half_pi<float>())
		{
			if (stop_a >= glm::half_pi<float>())
			{
				result.top_left.y = center.y + radius;
				result.top_left.x = center.x - radius;
				result.bottom_right.y = center.y - radius;
				result.bottom_right.x = center.x + radius;
			}
			else if (stop_a >= 0)
			{
				result.top_left.x = center.x - radius;
				result.bottom_right.x = center.x + radius;
				result.bottom_right.y = center.y - radius;
			}
		}
		else
		{
			result.top_left.y = center.y + radius;
			result.top_left.x = center.x - radius;
			result.bottom_right.y = center.y - radius;
			result.bottom_right.x = center.x + radius;
		}
	}

	return result;

	//rectangle result;

	//float start_a = geometry::oriented_angle(start, center);
	//float stop_a = geometry::oriented_angle(stop, center);
	//float three_forth_pi = glm::half_pi<float>() + glm::pi<float>();

	//result.top_left.x = glm::min(start.x, stop.x);
	//result.top_left.y = glm::max(start.y, stop.y);
	//result.bottom_right.x = glm::max(start.x, stop.x);
	//result.bottom_right.y = glm::min(start.y, stop.y);

	//if (cw)
	//{
	//	float pivot = start_a;
	//	start_a = stop_a;
	//	stop_a = pivot;
	//	cw = !cw;
	//}

	//if (start_a < stop_a)
	//{
	//	if (start_a < glm::half_pi<float>())
	//	{
	//		if (stop_a >= three_forth_pi)
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.y = center.y - radius;
	//		}
	//		else if (stop_a >= glm::pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//		}
	//		else if (stop_a >= glm::half_pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//		}
	//	}
	//	else if (start_a < glm::pi<float>())
	//	{
	//		if (stop_a >= three_forth_pi)
	//		{
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.y = center.y - radius;
	//		}
	//		else if (stop_a >= glm::pi<float>())
	//		{
	//			result.top_left.x = center.x - radius;
	//		}
	//	}
	//	else if (start_a < three_forth_pi)
	//	{
	//		if (stop_a >= three_forth_pi)
	//		{
	//			result.bottom_right.y = center.y - radius;
	//		}
	//	}
	//}
	//else
	//{
	//	if (start_a >= three_forth_pi)
	//	{
	//		if (stop_a >= three_forth_pi)
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.y = center.y - radius;
	//			result.bottom_right.x = center.x + radius;
	//		}
	//		else if (stop_a >= glm::pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.x = center.x + radius;
	//		}
	//		else if (stop_a >= glm::half_pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.bottom_right.x = center.x + radius;
	//		}
	//		else if (stop_a >= 0)
	//		{
	//			result.bottom_right.x = center.x + radius;
	//		}
	//	}
	//	else if (start_a >= glm::pi<float>())
	//	{
	//		if (stop_a >= glm::pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.y = center.y - radius;
	//			result.bottom_right.x = center.x + radius;
	//		}
	//		else if (stop_a >= glm::half_pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.bottom_right.x = center.x + radius;
	//			result.bottom_right.y = center.y - radius;
	//		}
	//		else if (stop_a >= 0)
	//		{
	//			result.bottom_right.x = center.x + radius;
	//			result.bottom_right.y = center.y - radius;
	//		}
	//	}
	//	else if (start_a >= glm::half_pi<float>())
	//	{
	//		if (stop_a >= glm::half_pi<float>())
	//		{
	//			result.top_left.y = center.y + radius;
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.y = center.y - radius;
	//			result.bottom_right.x = center.x + radius;
	//		}
	//		else if (stop_a >= 0)
	//		{
	//			result.top_left.x = center.x - radius;
	//			result.bottom_right.x = center.x + radius;
	//			result.bottom_right.y = center.y - radius;
	//		}
	//	}
	//	else
	//	{
	//		result.top_left.y = center.y + radius;
	//		result.top_left.x = center.x - radius;
	//		result.bottom_right.y = center.y - radius;
	//		result.bottom_right.x = center.x + radius;
	//	}
	//}

	//return result;
}

glm::vec2 circle_center(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
	glm::vec2 result = glm::vec2();

	float x1 = p1.x, x2 = p2.x, x3 = p3.x, y1 = p1.y, y2 = p2.y, y3 = p3.y;

	float a1 = (y1 - y3) / (x1 - x3);
	float a2 = (y2 - y3) / (x2 - x3);

	if (a1 == 0 || a2 == 0) {
		bool permuted = false;
		if (y1 - y3 == 0 || x1 - x3 == 0) { // p1-p3 vertical or horizontal, permutes
			permuted = true;
			x2 = p3.x;
			y2 = p3.y;
			x3 = p2.x;
			y3 = p2.y;
		}
		if (y2 - y3 == 0 || x2 - x3 == 0) {
			if (!permuted) {
				x1 = p3.x;
				y1 = p3.y;
				x3 = p1.x;
				y3 = p1.y;
			}
			else { // triangle rectangle
				if (y1 - y3 == 0 && x2 - x3 == 0) { // p1-p3 horizontal p2-p3 vertical
					result.x = (x1 + x3) / 2;
					result.y = (y2 + y3) / 2;
				}
				else { // p2-p3 horizontal p1-p3 vertical
					result.x = (x2 + x3) / 2;
					result.y = (y1 + y3) / 2;
				}
				return result;
			}
		}
		a1 = (y1 - y3) / (x1 - x3);
		a2 = (y2 - y3) / (x2 - x3);
	}

	// P1-P3
	float xe = (x1 + x3) / 2;
	float ye = (y1 + y3) / 2;

	// P2-P3
	float xf = (x2 + x3) / 2;
	float yf = (y2 + y3) / 2;

	float b1 = ye + (xe / a1);
	float b2 = yf + (xf / a2);

	result.x = (b2 - b1) / (1 / a2 - 1 / a1);
	result.y = -result.x / a1 + b1;
	return result;
}

bool arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 point, float err)
{
	float r1 = distance(center, start);
	float r2 = distance(center, point);
	if (glm::abs(r1 - r2) > err)
		return false;

	return colinear_arc_point(start, center, stop, cw, point);
}

bool arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 point, float err)
{
	float r2 = distance(center, point);
	if (glm::abs(radius - r2) > err)
		return false;

	return colinear_arc_point(start, center, stop, cw, point);
}

bool colinear_arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 point)
{
	float a1 = oriented_angle(start, center);
	float a2 = oriented_angle(stop, center);
	float a3 = oriented_angle(point, center);

	return colinear_arc_point(a1, center, a2, cw, point);
}

bool colinear_arc_point(float start, glm::vec2 center, float stop, bool cw, glm::vec2 point)
{
	float ctr = oriented_angle(point, center);

	if (ctr == start || ctr == stop)
		return true;

	if (cw)
	{
		if (start > stop)
		{
			if (ctr > stop - geometry::ERR_FLOAT5 && ctr < start + geometry::ERR_FLOAT5)
				return true;
		}
		else
		{
			if (ctr >= 0 - geometry::ERR_FLOAT5 && ctr < start + geometry::ERR_FLOAT5 || ctr > stop - geometry::ERR_FLOAT5)
				return true;
		}
	}
	else
	{
		if (start < stop)
		{
			if (ctr > start - geometry::ERR_FLOAT5 && ctr < stop + geometry::ERR_FLOAT5)
				return true;
		}
		else
		{
			if (ctr > start - geometry::ERR_FLOAT5 || ctr >= 0 - geometry::ERR_FLOAT5 && ctr < stop + geometry::ERR_FLOAT5)
				return true;
		}
	}

	return false;
}

glm::vec2 arc_middle(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw)
{
	return arc_middle(start, center, stop, geometry::distance(start, center), cw);
}

glm::vec2 arc_middle(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw)
{
	float a1 = oriented_angle(start, center);
	float a2 = oriented_angle(stop, center);
	float a3 = 0;

	if (radius == 0)
		radius = geometry::distance(start, center);

	if (cw)
	{
		if (a1 > a2)
			a3 = a1 - ((a1 - a2) / 2);
		else
			a3 = oriented_angle(a1 - ((a1 - (a2 - glm::two_pi<float>())) / 2));
	}
	else
	{
		if (a1 < a2)
			a3 = a2 - ((a2 - a1) / 2);
		else
			a3 = oriented_angle(a2 - ((a2 - (a1 - glm::two_pi<float>())) / 2));
	}

	return position(a3, radius, center);
}

float arc_thickness(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw)
{
	return arc_thickness(start, center, stop, geometry::distance(center, start), cw);
}

float arc_thickness(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw)
{
	auto middle = arc_middle(start, center, stop, radius, cw);
	auto axe = geometry::middle(start, stop);

	return geometry::distance(middle, axe);
}

float arc_length(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw)
{
	return arc_length(start, center, stop, distance(start, center), cw);
}

float arc_length(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw)
{
	float a1 = oriented_angle(start, center);
	float a2 = oriented_angle(stop, center);

	if (!cw)
	{
		if (a1 < a2)
			return (a2 - a1) * radius;
		else
			return (glm::two_pi<float>() - a1 + a2) * radius;
	}
	else
	{
		if (a1 > a2)
			return (a1 - a2) * radius;
		else
			return (glm::two_pi<float>() - a2 + a1) * radius;
	}
}

int arc_position(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 test)
{
	auto a1 = geometry::oriented_angle(start,center);
	auto a2 = geometry::oriented_angle(stop, center);
	auto a3 = geometry::oriented_angle(test, center);

	if (cw) 
	{
		if (a1 > a2)
		{
			if (a3 > a1)
				return -1;
			else if (a3 > a2)
				return 0;
			else
				return 1;
		}
		else
		{
			if (a3 > a1 && a3 < a2)
				return -1;
			else if (a3 < a1 || a3 > a2)
				return 0;
			else
				return 1;
		}
	}
	else
	{
		if (a1 < a2)
		{
			if (a3 < a1 || a3 > a2)
				return -1;
			else if (a3 > a1 && a3 < a2)
				return 0;
			else
				return 1;
		}
		else
		{
			if (a3 > a2 && a3 < a1)
				return -1;
			else if (a1 < a3 || a2 > a3)
				return 0;
			else
				return 1;
		}
	}
}

bool arc_position_left(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 test)
{
	auto a1 = geometry::oriented_angle(start, center);
	auto a2 = geometry::oriented_angle(stop, center);
	auto a3 = geometry::oriented_angle(test, center);

	if (cw)
	{
		if (a1 > a2)
		{
			if (a3 > a1)
				return true;
		}
		else
		{
			if (a3 > a1 && a3 < a2)
				return true;
		}
	}
	else
	{
		if (a1 < a2)
		{
			if (a3 < a1 || a3 > a2)
				return true;
		}
		else
		{
			if (a3 > a2 && a3 < a1)
				return true;
		}
	}
	return false;
}

/// <summary>
/// Return coordinates on a circle from angle and radius
/// </summary>
/// <param name="angle">angle in radian</param>
/// <param name="radius">radius</param>
/// <returns>coordinates</returns>
glm::vec2 position(float angle, float radius)
{
	// remove modulo [2*PI]
	while (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
	while (angle < 0) angle += glm::two_pi<float>();

	if (angle == glm::half_pi<float>())
		return glm::vec2(0, radius);
	if (angle == glm::pi<float>() + glm::half_pi<float>())
		return glm::vec2(0, -radius);
	if (angle == 0)
		return glm::vec2(radius, 0);
	if (angle == glm::pi<float>())
		return glm::vec2(-radius, 0);

	return glm::vec2(radius * glm::cos(angle), radius * glm::sin(angle));
}

/// <summary>
/// Return coordinates on a circle from angle and radius
/// </summary>
/// <param name="angle">angle in radian</param>
/// <param name="radius">radius</param>
/// <param name="origin">position of the center of circle</param>
/// <returns></returns>
glm::vec2 position(float angle, float radius, glm::vec2 origin)
{
	// remove modulo [2*PI]
	while (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
	while (angle < 0) angle += glm::two_pi<float>();

	if (angle == glm::half_pi<float>())
		return glm::vec2(origin.x, origin.y + radius);
	if (angle == glm::pi<float>() + glm::half_pi<float>())
		return glm::vec2(origin.x, origin.y - radius);
	if (angle == 0)
		return glm::vec2(origin.x + radius, origin.y);
	if (angle == glm::pi<float>())
		return glm::vec2(origin.x - radius, origin.y);

	return glm::vec2(radius * glm::cos(angle) + origin.x, radius * glm::sin(angle) + origin.y);
}

/// <summary>
/// Return coordinates on a circle from angle and radius
/// </summary>
/// <param name="source">coordinates of original source ref_point</param>
/// <param name="radius">radius to apply</param>
/// <param name="origin">position of the center of circle</param>
/// <returns></returns>
glm::vec2 position(glm::vec2 source, float radius, glm::vec2 origin)
{
	auto a = angle(source, origin);

	if (a == 0)
		return glm::vec2(origin.x + radius, origin.y);
	else if (a == glm::half_pi<float>())
		return glm::vec2(origin.x, origin.y + radius);
	else if (a == glm::pi<float>())
		return glm::vec2(origin.x - radius, origin.y);
	else if (a == glm::half_pi<float>() + glm::pi<float>())
		return glm::vec2(origin.x, origin.y - radius);

	return glm::vec2(radius * glm::cos(a) + origin.x, radius * glm::sin(a) + origin.y);
}

/// <summary>
/// convert a float/double to string with a decimal precision number
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="a_value">value to convert</param>
/// <param name="n">number of decimal</param>
/// <returns>a string representation</returns>
template <typename T>
std::string to_string(const T value, const int n)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << value;
	return std::move(out).str();
}

template <typename T>
std::string to_string(const T value)
{
	std::ostringstream oss;
	oss << std::noshowpoint << value;
	return oss.str();
}

/// <summary>
/// Return vec2 to std::string such as (x;y)
/// </summary>
/// <param name="v">vector to print</param>
/// <returns>A std::string representation</returns>
std::string to_string(glm::vec2 v, const int n, char separator)
{
	if (n > 0)
		return "(" + geometry::to_string(v.x, n) + separator + geometry::to_string(v.y, n) + ')';
	else
		return "(" + geometry::to_string(v.r) + separator + geometry::to_string(v.g) + ')';
}


/// <summary>
/// Return vec3 to std::string such as (x;y;z)
/// </summary>
/// <param name="v">vector to print</param>
/// <returns>A std::string representation</returns>
std::string to_string(glm::vec3 v, const int n, char separator)
{
	if (n > 0)
		return "(" + geometry::to_string(v.x, n) + separator + geometry::to_string(v.y, n) + separator + geometry::to_string(v.z, n) + ')';
	else
		return "(" + geometry::to_string(v.r) + separator + geometry::to_string(v.g) + separator + geometry::to_string(v.b) + ')';
}


/// <summary>
/// Return vec3 to std::string such as (r;g;b;a)
/// </summary>
/// <param name="v">vector to print</param>
/// <returns>A std::string representation</returns>
std::string to_string(glm::vec4 v, const int n, char separator)
{
	if (n > 0)
		return "(" + geometry::to_string(v.r, n) + separator + geometry::to_string(v.g, n) + separator + geometry::to_string(v.b, n) + separator + geometry::to_string(v.a, n) + ')';
	else
		return "(" + geometry::to_string(v.r) + separator + geometry::to_string(v.g) + separator + geometry::to_string(v.b) + separator + geometry::to_string(v.a) + ')';
}

glm::vec4 from_string(std::string value, char separator)
{
	glm::vec4 result = geometry::vec4_empty;
	short p = 0;
	if (value.size() > 2)
	{
		if (value[0] == '(')
			value = value.substr(1, value.size() - 2);
		std::string val;
		for (int i = 0; i < value.size(); i++)
		{
			if (value[i] == separator)
			{
				if (p == 0)
					result.r = std::stof(val);
				else if (p == 1)
					result.g = std::stof(val);
				else if (p == 2)
					result.b = std::stof(val);
				val = "";
				p++;
			}
			else if (value[i] != 'f' && value[i] != ' ')
				val += value[i];
		}
		result.a = std::stof(val);
	}
	return result;
}

glm::vec4 from_string4(std::string value, char separator)
{
	return from_string(value, separator);
}

glm::vec3 from_string3(std::string value, char separator)
{
	glm::vec3 result = geometry::vec3_empty;
	short p = 0;
	if (value.size() > 2)
	{
		if (value[0] == '(')
			value = value.substr(1, value.size() - 2);
		std::string val;
		for (int i = 0; i < value.size(); i++)
		{
			if (value[i] == separator)
			{
				if (p == 0)
					result.x = std::stof(val);
				else if (p == 1)
					result.y = std::stof(val);
				val = "";
				p++;
			}
			else if (value[i] != 'f' && value[i] != ' ')
				val += value[i];
		}
		result.z = std::stof(val);
	}
	return result;
}

glm::vec2 from_string2(std::string value, char separator)
{
	glm::vec2 result = geometry::vec2_empty;
	short p = 0;
	if (value.size() > 2)
	{
		if (value[0] == '(')
			value = value.substr(1, value.size() - 2);
		std::string val;
		for (int i = 0; i < value.size(); i++)
		{
			if (value[i] == separator)
			{
				if (p == 0)
					result.x = std::stof(val);
				val = "";
				p++;
			}
			else if (value[i] != 'f' && value[i] != ' ')
				val += value[i];
		}
		result.y = std::stof(val);
	}
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           curve helper methods                                               //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<glm::vec2> cubic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float scale)
{
	std::vector<glm::vec2> coords;

	auto d1 = glm::distance(p0, p1);
	auto d2 = glm::distance(p1, p2);
	auto d3 = glm::distance(p2, p3);

	int pts = (int)((glm::distance(p0, p1) + glm::distance(p1, p2) + glm::distance(p2, p3)) * scale);
	if (pts < 30)
		pts = 30;
	float t = 0.0f;
	float dt = 1.0f / pts;
	float cx = 3.0f * (p1.x - p0.x);
	float cy = 3.0f * (p1.y - p0.y);
	float bx = 3.0f * (p2.x - p1.x) - cx;
	float by = 3.0f * (p2.y - p1.y) - cy;
	float ax = p3.x - p0.x - cx - bx;
	float ay = p3.y - p0.y - cy - by;

	coords.reserve(pts);
	for (int i = 0; i < pts; ++i)
	{
		float tsqr = t * t;
		float tcube = tsqr * t;
		glm::vec2 p(
			(ax * tcube) + (bx * tsqr) + (cx * t) + p0.x,
			(ay * tcube) + (by * tsqr) + (cy * t) + p0.y
		);
		if (!(coords.size() > 0 && coords[coords.size() - 1] == p))
			coords.push_back(p);
		t += dt;
	}
	return coords;
}

std::vector<glm::vec2> cubic(std::vector<glm::vec2> points, float scale)
{
	return cubic(points[0], points[1], points[2], points[3], scale);
}

void cubic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, std::vector<glm::vec2>& coords, float scale)
{
	int pts = (int)((glm::distance(p0, p1) + glm::distance(p1, p2) + glm::distance(p2, p3)) * scale);
	if (pts < 30) pts = 30;
	float t = 0.0f;
	float dt = 1.0f / pts;
	float cx = 3.0f * (p1.x - p0.x);
	float cy = 3.0f * (p1.y - p0.y);
	float bx = 3.0f * (p2.x - p1.x) - cx;
	float by = 3.0f * (p2.y - p1.y) - cy;
	float ax = p3.x - p0.x - cx - bx;
	float ay = p3.y - p0.y - cy - by;

	coords.reserve(pts);
	for (int i = 0; i < pts; ++i)
	{
		float tsqr = t * t;
		float tcube = tsqr * t;
		glm::vec2 p(
			(ax * tcube) + (bx * tsqr) + (cx * t) + p0.x,
			(ay * tcube) + (by * tsqr) + (cy * t) + p0.y
		);
		if (!(coords.size() > 0 && coords[coords.size() - 1] == p))
			coords.push_back(p);
		t += dt;
	}
}

std::vector<glm::vec2> quadratic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, float scale)
{
	// https://fontforge.org/docs/techref/bezier.html#converting-truetype-to-postscript
	glm::vec2 cp1 = p0 + (2.0f / 3.0f) * (p1 - p0);
	glm::vec2 cp2 = p2 + (2.0f / 3.0f) * (p1 - p2);

	return cubic(p0, cp1, cp2, p2, scale);
}

std::vector<glm::vec2> quadratic(std::vector<glm::vec2> points, float scale)
{
	return quadratic(points[0], points[1], points[2], scale);
}

float eval_nc(std::vector<float> cubic, float u) {
	return (((cubic[3] * u) + cubic[2]) * u + cubic[1]) * u + cubic[0];
}

float b(int i, float t) {
	switch (i) {
	case -2:
		return (((-t + 3) * t - 3) * t + 1) / 6;
	case -1:
		return (((3 * t - 6) * t) * t + 4) / 6;
	case 0:
		return (((-3 * t + 3) * t + 3) * t + 1) / 6;
	case 1:
		return (t * t * t) / 6;
	}
	return 0.0f; //we only get here if an invalid i is specified
}

//evaluate a ref_point on the B spline
glm::vec2 p(int i, float t, std::vector<glm::vec2>& vectors) {
	float px = 0;
	float py = 0;
	for (int j = -2; j <= 1; j++) {
		px += (b(j, t) * vectors[i + j].x);
		py += (b(j, t) * vectors[i + j].y);
	}
	return glm::vec2(px, py);
}

glm::vec2 eq(float t, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4) {
	float t2 = t * t;
	float t3 = t2 * t;

	float b1 = 0.5f * (-t3 + 2 * t2 - t);
	float b2 = 0.5f * (3 * t3 - 5 * t2 + 2);
	float b3 = 0.5f * (-3 * t3 + 4 * t2 + t);
	float b4 = 0.5f * (t3 - t2);

	glm::vec2 v = glm::vec2(p1.x, p1.y) * b1 +
		glm::vec2(p2.x, p2.y) * b2 +
		glm::vec2(p3.x, p3.y) * b3 +
		glm::vec2(p4.x, p4.y) * b4;

	return glm::vec2(v.x, v.y);
}

float delta_t = 0;
glm::vec2 catmull(float t, std::vector<glm::vec2>& points)
{
	// On recherche l'interval du spline
	int p = (int)(t / delta_t);
	// On calcule l'indice du ref_point de contrôle local
	int p0 = p - 1;
	{ if (p0 < 0) p0 = 0; else if (p0 >= (int)points.size() - 1) p0 = (int)points.size() - 1; }
	int p1 = p;
	{ if (p1 < 0) p1 = 0; else if (p1 >= (int)points.size() - 1) p1 = (int)points.size() - 1; }
	int p2 = p + 1;
	{ if (p2 < 0) p2 = 0; else if (p2 >= (int)points.size() - 1) p2 = (int)points.size() - 1; }
	int p3 = p + 2;
	{ if (p0 < 0) p3 = 0; else if (p3 >= (int)points.size() - 1) p3 = (int)points.size() - 1; }
	// temps relatif
	float lt = (t - delta_t * (float)p) / delta_t;
	// Interpolation
	return eq(lt, points[p0], points[p1], points[p2], points[p3]);
}


std::vector<std::vector<float>> natural_cubic(int n, std::vector<float> x) {
	std::vector<float> gamma(n + 1), delta(n + 1), D(n + 1);
	int i;

	/* nous résolvons l'équation
	[2 1       ] [D[0]]   [3(x[1] - x[0])  ]
	|1 4 1     | |D[1]|   |3(x[2] - x[0])  |
	|  1 4 1   | | .  | = |      .         |
	|    ..... | | .  |   |      .         |
	|     1 4 1| | .  |   |3(x[n] - x[n-2])|
	[       1 2] [D[n]]   [3(x[n] - x[n-1])]

	by using row operations to convert the matrix to upper triangular
	and then back sustitution.  The D[i] are the derivatives at the knots.
	*/

	gamma[0] = 1.0f / 2.0f;
	for (i = 1; i < n; i++)
		gamma[i] = 1 / (4 - gamma[i - 1]);
	gamma[n] = 1 / (2 - gamma[n - 1]);

	delta[0] = 3 * (x[1] - x[0]) * gamma[0];
	for (i = 1; i < n; i++)
		delta[i] = (3 * (x[i + 1] - x[i - 1]) - delta[i - 1]) * gamma[i];
	delta[n] = (3 * (x[n] - x[n - 1]) - delta[n - 1]) * gamma[n];

	D[n] = delta[n];
	for (i = n - 1; i >= 0; i--)
		D[i] = delta[i] - gamma[i] * D[i + 1];

	/* now compute the coefficients of the cubics */
	std::vector<std::vector<float>> C;
	for (i = 0; i < n; i++) {
		C.push_back(std::vector<float>({ x[i], D[i], 3 * (x[i + 1] - x[i]) - 2 * D[i] - D[i + 1], 2 * (x[i] - x[i + 1]) + D[i] + D[i + 1] }));
	}
	return C;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// 
/// </summary>
/// <param name="points"></param>
/// <returns></returns>
std::vector<glm::vec2>bezier(std::vector<glm::vec2> points, float scale)
{
	std::vector<glm::vec2> coords;
	for (int i = 0; i < points.size() - 3; i += 3) {
		if (points[i] == points[i + 2] && points[i + 1] == points[i + 3]) {
			if (coords.size() == 0)
				coords.push_back(points[i]);
			coords.push_back(points[i + 3]);
		}
		else {
			cubic(points[i], points[i + 1], points[i + 2], points[i + 3], coords, scale);
		}
	}
	if (coords.size() > 0 && points.size() > 0 && coords[coords.size() - 1] != points[points.size() - 1])
		coords.push_back(points[points.size() - 1]);
	return coords;
}

std::vector<glm::vec2> bspline(std::vector<glm::vec2>& points)
{
	std::vector<glm::vec2> coords, vectors;
	vectors.push_back(points[0]);
	vectors.insert(vectors.end(), points.begin(), points.end());
	vectors.push_back(points[points.size() - 1]);
	coords.push_back(points[0]);
	for (int i = 2; i < vectors.size() - 1; i++) {
		for (int j = 1; j <= 12; j++) {
			coords.push_back(p(i, j / 12.0f, vectors));
		}
	}
	coords.push_back(points[points.size() - 1]);
	return coords;
}

std::vector<glm::vec2> catmull(std::vector<glm::vec2>& points) {
	std::vector<glm::vec2> coords;
	if (points.size() < 2) return coords;

	delta_t = 1.0f / (float)points.size();

	float start = 0.0f; 	// coordonnées x min
	float end = 0.0f; 	// coordonnées x max
	glm::vec2 last = points.back();
	start = end = points.front().x;

	for (glm::vec2 pi : points)
	{
		start = glm::min(start, pi.x);
		end = glm::max(end, pi.x);
	}

	//compute CatmullRom operator for each X coordinate
	for (float x = 0.0f; x <= end; x += 0.1f) {
		float t = x / end;
		glm::vec2 v = catmull(t, points);
		coords.push_back(v);
		if (coords.size() > 1 && v == last)
			break;
	}
	return coords;
}

std::vector<glm::vec2> lagrange(std::vector<glm::vec2>& points) {
	std::vector<glm::vec2> coords;
	if (points.size() < 2) return coords;

	float start = 0; 	// coordonnées x min
	float end = 0; 	// coordonnées x max
	glm::vec2 first = points.front();
	glm::vec2 last = points.back();
	start = end = first.x;

	for (glm::vec2 pi : points)
	{
		start = glm::min(start, pi.x);
		end = glm::max(end, pi.x);
	}

	float inc = 1;
	if (last.x < first.x) {
		float t = start; start = end; end = t;
		inc = -1;
	}

	// tried first with std::vector, very slow :-(, change back to dynamic array
	int coords_count = 0;
	for (float x = start; inc < 0 ? x >= end : x <= end; x += inc)
		coords_count++;
	float* Xs = new float[coords_count];
	float** OpList = new float* [coords_count];


	//compute lagrange operator for each X coordinate
	int pos = 0;
	for (float x = start; inc < 0 ? x >= end : x <= end; x += inc)
	{
		//list of float to hold the Lagrange operators
		float* L = new float[points.size()];
		//Init the list with 1's
		std::fill_n(L, points.size(), 1.0f);
		for (int i = 0; i < points.size(); i++)
		{
			auto pi = points[i];
			for (int k = 0; k < points.size(); k++)
			{
				glm::vec2 pk = points[k];
				if (i != k) {
					if (pi.x != pk.x)
						L[i] *= (x - pk.x) / (pi.x - pk.x);
				}
			}
		}
		OpList[pos] = L;
		Xs[pos++] = x;
	}

	//Computing the Polynomial P(x) which is y in our curve
	float* Ys = new float[coords_count];
	int j = 0;
	for (int j = 0; j < coords_count; j++)
	{
		float y = 0;
		for (int i = 0; i < points.size(); i++)
		{
			y += OpList[j][i] * points[i].y;
		}
		Ys[j] = y;
	}
	coords.reserve(points.size());
	for (int i = 0; i < coords_count; i++) {
		coords.push_back(glm::vec2(Xs[i], Ys[i]));
	}

	delete[] Xs;
	delete[] Ys;

	for (int j = 0; j < coords_count; j++)
		delete OpList[j];
	delete[] OpList;

	return coords;
}

std::vector<glm::vec2> naturalCubic(std::vector<glm::vec2>& points) {
	std::vector<glm::vec2> coords;
	std::vector<float> xx(points.size()), yy(points.size());

	for (int i = 0; i < points.size(); i++) {
		xx[i] = points[i].x;
		yy[i] = points[i].y;
	}

	if (points.size() >= 2) {
		std::vector<std::vector<float>> X = natural_cubic((int)points.size() - 1, xx);
		std::vector<std::vector<float>> Y = natural_cubic((int)points.size() - 1, yy);
		coords.push_back(glm::vec2(eval_nc(X[0], 0), eval_nc(Y[0], 0)));
		for (int i = 0; i < X.size(); i++) {
			for (int j = 1; j <= 12; j++) {
				float u = j / 12.0f;
				coords.push_back(glm::vec2(eval_nc(X[i], u), eval_nc(Y[i], u)));
			}
		}
	}
	return coords;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       end curve helper methods                                               //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// reduce number of segment in polyline
/// </summary>
/// <param name="vertices">polyline</param>
/// <param name="max">max length</param>
/// <returns></returns>
std::vector<glm::vec2> reduce(std::vector<glm::vec2>& vertices, float max)
{
	std::vector<glm::vec2> result;
	glm::vec2 previous = vertices.front();
	float m2 = max * max;

	result.push_back(previous);
	for (glm::vec2 next : vertices)
	{
		if (geometry::distance2(previous, next) >= m2)
		{
			previous = next;
			result.push_back(previous);
		}
	}

	if (result.size() == 0)
	{
		return vertices;
	}
	if (result.size() == 1)
		result.push_back(vertices.back());

	return result;
}

/// <summary>
/// reduce number of segment in polylines
/// </summary>
/// <param name="vertices">polylines</param>
/// <param name="max">max length</param>
/// <returns></returns>
std::vector<std::vector<glm::vec2>> reduce(std::vector<std::vector<glm::vec2>>& vertices, float max)
{
	std::vector<std::vector<glm::vec2>> result;

	for (std::vector<glm::vec2> v : vertices)
	{
		result.push_back(reduce(v, max));
	}

	return result;
}

/// <summary>
/// Add a constant to a vector
/// </summary>
/// <param name="p">vector to offset</param>
/// <param name="value">constant to add</param>
void offset(glm::vec2& p, float value)
{
	p.x += value;
	p.y += value;
}

/// <summary>
/// Add a constant to a vector
/// </summary>
/// <param name="p">vector to offset</param>
/// <param name="value">constant to add</param>
void offset(glm::vec3& p, float value)
{
	p.x += value;
	p.y += value;
	p.z += value;
}

/// <summary>
/// return the coordinates of orthogonal projection of a ref_point on a segment
/// </summary>
/// <param name="p">ref_point to project</param>
/// <param name="p1">first segment ref_point</param>
/// <param name="p2">second segment ref_point</param>
/// <returns>coordinates of projection</returns>
glm::vec2 projection(glm::vec2 p, glm::vec2 p1, glm::vec2 p2)
{
	float dh = p1.x - p2.x;
	if (glm::abs(dh) <= geometry::ERR_FLOAT3)
		return glm::vec2(p1.x, p.y);

	float dv = p1.y - p2.y;
	if (glm::abs(dv) <= geometry::ERR_FLOAT3)
		return glm::vec2(p.x, p1.y);

	// y = a * x + b
	auto a = dv / dh;
	auto b = p1.y - (a * p1.x);

	// orthogonal
	auto o_a = -1 / a;
	auto o_b = p.y - (o_a * p.x);

	// intersection
	auto x = (o_b - b) / (a - o_a);
	auto y = a * x + b;
	return glm::vec2(x, y);
}

/// <summary>
/// return the symmetry ref_point with an axe of symmetry
/// </summary>
/// <param name="p">source</param>
/// <param name="p1">first axe ref_point</param>
/// <param name="p2">second axe ref_point</param>
/// <returns>symmetry ref_point</returns>
glm::vec2 symmetry(glm::vec2 p, glm::vec2 p1, glm::vec2 p2)
{
	auto h = projection(p, p1, p2);

	return glm::vec2(2 * h.x - p.x, 2 * h.y - p.y);
}

/// <summary>
/// return the symmetry ref_point with a center of symmetry
/// </summary>
/// <param name="p">source</param>
/// <param name="center">center of symmetry</param>
/// <returns>symmetry ref_point</returns>
glm::vec2 symmetry(glm::vec2 p, glm::vec2 center)
{
	if (center.x == p.x)
		return glm::vec2(p.x, center.y - (p.y - center.y));
	if (center.y == p.y)
		return glm::vec2(center.x - (p.x - center.x), p.y);
	auto a = oriented_angle(p, center) + glm::pi<float>();
	auto r = glm::distance(center, p);
	return geometry::position(a, r, center);
}

glm::vec2 middle(glm::vec2 p1, glm::vec2 p2)
{
	return (p1 + p2) / 2.0f;
}

/// <summary>
/// check if ref_point is a segment. the 3 points are supposed to be colinear
/// </summary>
bool colinear_segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test)
{
	auto x = test.x >= glm::min(p1.x, p2.x) && test.x <= glm::max(p1.x, p2.x);
	auto y = test.y >= glm::min(p1.y, p2.y) && test.y <= glm::max(p1.y, p2.y);
	return  x && y;
}

bool colinear_segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test, float err)
{
	auto x = test.x >= (glm::min(p1.x, p2.x) - err) && test.x <= (glm::max(p1.x, p2.x) + err);
	auto y = test.y >= (glm::min(p1.y, p2.y) - err) && test.y <= (glm::max(p1.y, p2.y) + err);
	return  x && y;
}

bool colinear_segments(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4)
{
	// vertical
	if (p1.x == p2.x && p1.x == p3.x && p1.x == p4.x)
		return true;

	// horizontal
	if (p1.y == p2.y && p1.y == p3.y && p1.y == p4.y)
		return true;

	auto u = p2 - p1;
	auto v = p4 - p3;

	float d = u.x * v.y - u.y * v.x;

	return glm::abs(d) < ERR_FLOAT;
}

/// <summary>
/// check if ref_point is a segment.
/// </summary>
bool segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test)
{
	auto a = p2 - p1;
	auto b = test - p1;
	auto cross = a.x * b.y - b.x * a.y;

	if ( cross < ERR_FLOAT)
		return  colinear_segment_point(p1, p2, test);
	return false;
}

int segment_position(glm::vec2 start, glm::vec2 stop, glm::vec2 test)
{
	// vertical
	if (start.x == stop.x)
	{
		if (start.y > stop.y)
		{
			if (test.y > start.y)
				return -1;
			else if (test.y > stop.y)
				return 0;
			else
				return 1;
		}
		else
		{
			if (test.y < start.y)
				return -1;
			else if (test.y < stop.y)
				return 0;
			else
				return 1;
		}
	}
	else // otherwise test on x value
	{
		if (start.x > stop.x)
		{
			if (test.x > start.x)
				return -1;
			else if (test.x > stop.x)
				return 0;
			else
				return 1;
		}
		else
		{
			if (test.x < start.x)
				return -1;
			else if (test.x < stop.x)
				return 0;
			else
				return 1;
		}
	}
}

bool segment_position_left(glm::vec2 start, glm::vec2 stop, glm::vec2 test)
{
	// vertical
	if (start.x == stop.x)
	{
		if (start.y > stop.y)
		{
			if (test.y > start.y)
				return true;
		}
		else if (test.y < start.y)
			return true;
	}
	else // otherwise test on x value
	{
		if (start.x > stop.x)
		{
			if (test.x > start.x)
				return true;
		}
		else if (test.x < start.x)
			return true;
	}
	return false;
}

bool line_line_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2& result)
{
	// line p1,p2 is horizontal and p3,p4 is vertical
	if (p1.y == p2.y && p3.x == p4.x)
	{
		result.x = p3.x;
		result.y = p1.y;
		return true;
	}
	else // line p1,p2 is vertical and p3,p4 is horizontal
		if (p1.x == p2.x && p3.y == p4.y) {
			result.x = p1.x;
			result.y = p3.y;
			return true;
		}
		else // lines are parallel
			if (p1.x == p2.x && p1.x == p3.x && p1.x == p4.x || p1.y == p2.y && p1.y == p3.y && p1.y == p4.y)
			{
				return false;
			}
			else
			{
				// det solution
				//double detL1 = p1.x * p2.y - p2.x * p1.y;
				//double detL2 = p3.x * p4.y - p4.x * p3.y;
				//double x1mx2 = p1.x - p2.x;
				//double x3mx4 = p3.x - p4.x;
				//double y1my2 = p1.y - p2.y;
				//double y3my4 = p3.y - p4.y;

				//double xnom = detL1 * x3mx4 - detL2 * x1mx2;
				//double ynom = detL1 * y3my4 - detL2 * y1my2;
				//double denom = x1mx2 * y3my4 - x3mx4 * y1my2;

				//if (denom != 0)
				//{
				//	result.x = (float)(xnom / denom);
				//	result.y = (float)(ynom / denom);
				//	
				//	if (glm::abs(p1.x - p2.x) < geometry::ERR_FLOAT5)
				//		result.x = p1.x;
				//	else if (glm::abs(p3.x - p4.x) < geometry::ERR_FLOAT5)
				//		result.x = p3.x;
				//	if (glm::abs(p1.y - p2.y) < geometry::ERR_FLOAT5)
				//		result.y = p1.y;
				//	else if (glm::abs(p3.y - p4.y) < geometry::ERR_FLOAT5)
				//		result.y = p3.y;

				//	return true;
				//}



				//// parametric solution
				glm::vec2 i = p2 - p1;
				glm::vec2 j = p4 - p3;
				double m = 0, k = 0;
				double div = (i.x * j.y - i.y * j.x);

				if (div != 0)
				{
					m = (i.x * p1.y
						- i.x * p3.y
						- i.y * p1.x
						+ i.y * p3.x
						) / div;
				// k = (j.x * p1.y
				//		- j.x * p3.y
				//		- j.y * p1.x
				//		+ j.y * p3.x
				//		) / div;
				
					result = p3 + (j * (float)m);
					result.x = p3.x + (float)(j.x * m);
					result.y = p3.y + (float)(j.y * m);

					// check for verticality or horizontality to increase accuracy
				if (glm::abs(p1.x - p2.x) < geometry::ERR_FLOAT5)
					result.x = p1.x;
				else if (glm::abs(p3.x - p4.x) < geometry::ERR_FLOAT5)
					result.x = p3.x;
				if (glm::abs(p1.y - p2.y) < geometry::ERR_FLOAT5)
					result.y = p1.y;
				else if (glm::abs(p3.y - p4.y) < geometry::ERR_FLOAT5)
					result.y = p3.y;

					return true;
				}

				//// Line equation solution
				//double a1 = 0, b1 = 0, a2 = 0, b2 = 0, dh1, dh2; // affine coeff y = a * x + b

				//dh1 = (p1.x - p2.x);
				//dh2 = (p3.x - p4.x);

				//if (dh1 != 0)
				//{
				//	a1 = (p1.y - p2.y) / dh1;
				//	b1 = p1.y - a1 * p1.x;
				//}
				//else
				//	b1 = p1.x;

				//if (dh2 != 0)
				//{
				//	a2 = (p3.y - p4.y) / dh2;
				//	b2 = p3.y - a2 * p3.x;
				//}
				//else
				//	b2 = p3.x;

				//// if not parallel
				//if (glm::abs(a1 - a2) > 0)
				//{
				//	if (dh1 == 0) {
				//		result.x = (float)b1;
				//		result.y = (float)(a2 * b1 + b2);
				//	}
				//	else if (dh2 == 0) {
				//		result.x = (float)b2;
				//		result.y = (float)(a1 * b2 + b1);
				//	}
				//	else {
				//		result.x = (float)((b2 - b1) / (a1 - a2));

				//		if (a1 == 0) // if segment 1 is horizontal, then y intersection is p1.y or p2.y, so we avoid float rounding
				//			result.y = p1.y;
				//		else if (a2 == 0)  // if segment 2 is horizontal, then y intersection is p3.y oy p4.y
				//			result.y = p3.y;
				//		else
				//			result.y = (float)(a1 * result.x + b1);
				//	}
				//	return true;
				//}
			}
	return false;
}

bool segment_segment_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2& result)
{
	bool intersect = line_line_intersect(p1, p2, p3, p4, result);

	if (result == p1 || result == p2)
	{
		// we are in the case of 2 jointed segments
		if (result == p3 || result == p4)
			return false;
	}

	return intersect && 
		colinear_segment_point(p1, p2, result, geometry::ERR_FLOAT3) && colinear_segment_point(p3, p4, result, geometry::ERR_FLOAT3);
}

bool segment_polyline_intersect(glm::vec2 p1, glm::vec2 p2, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result)
{
	if (coordinates.size() < 2)
		return false;

	//float x_min, x_max, y_min, y_max;
	//x_min = x_max = coordinates[0].x;
	//y_min = y_max = coordinates[0].y;

	//for (glm::vec2 p : coordinates)
	//{
	//	x_min = glm::min(x_min, p.x);
	//	x_max = glm::max(x_max, p.x);
	//	y_min = glm::min(y_min, p.y);
	//	y_max = glm::min(y_max, p.y);
	//}

	//if (geometry::rectangle_intersect(p1, p2))
	glm::vec2 p = glm::vec2();
	for (size_t i = 0; i < coordinates.size() - 1; i++)
	{
		if (segment_segment_intersect(p1, p2, coordinates[i], coordinates[i + 1], p))
			result.push_back(p);
	}

	return result.size() > 0;
}

int line_circle_intersect2(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2], float err)
{
	// translate center to origin
	p1 -= center;
	p2 -= center;
	
	//if (p1.x == p2.x) // line is vertical vertical
	//{
	//	result[0].x = result[1].x = p1.x;
	//	result[0].y = glm::sqrt(glm::abs(radius * radius - result[0].x * result[0].x)); //radius * glm::sin(glm::acos(result[0].x));
	//	result[1].y = -result[0].y;
	//	return 2;
	//}
	//else if (p1.y == p2.y) // line is horizontal
	//{
	//	result[0].y = result[1].y = p1.y;
	//	result[0].x = glm::sqrt(glm::abs(radius * radius - result[0].y * result[0].y)); // radius* glm::cos(glm::asin(result[0].y));
	//	result[1].x = -result[0].x;
	//	return 2;
	//}
	//else
	{
		// compute line coef : y = m * x + d, we already know the line is not vertical
		// or a * x + b * y + c = 0
		double a = (p1.y - p2.y) / (p1.x - p2.x);
		double b = -1;
		double c = p1.y - a * p1.x;

		double x0 = -a * c / (a * a + b * b);
		double y0 = -b * c / (a * a + b * b);

		double r2 = radius * radius;
		double a2 = a * a;
		double b2 = b * b;
		double c2 = c * c;

		if (glm::abs(c2 - r2 * (a2 + b2)) < err)
		{
			result[0].x = center.x + (float)(x0);
			result[0].y = center.y + (float)(y0);
			return 1;
		}
		else if (c2 < r2 * (a2 + b2) + err)
		{
			double d = r2 - c2 / (a2 + b2);
			double mult = sqrt(d / (a2 + b2));
			result[0].x = center.x + (float)(x0 + b * mult);
			result[1].x = center.x + (float)(x0 - b * mult);
			result[0].y = center.y + (float)(y0 - a * mult);
			result[1].y = center.y + (float)(y0 + a * mult);
			return 2;
		}
	}
	return 0;
}

int line_circle_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2], float err)
{
	float distance = foretriangle(p1, p2, center);
	
	if (distance == radius) // tangente
	{
		result[0] = projection(center, p1, p2);
		return 1;
	}
	
	if (distance <= radius)
	{
		// translate center to origin
		p1 -= center;
		p2 -= center;

		// vertical
		if (p1.x == p2.x)
		{
			result[0].x = result[1].x = p1.x;
			result[0].y = glm::sqrt(glm::abs(radius * radius - result[0].x * result[0].x)); //radius * glm::sin(glm::acos(result[0].x));
			result[1].y = -result[0].y;
		}
		else // horizontal
			if (p1.y == p2.y)
		{
				result[0].y = result[1].y = p1.y;
				result[0].x = glm::sqrt(glm::abs(radius * radius - result[0].y * result[0].y)); // radius* glm::cos(glm::asin(result[0].y));
				result[1].x = -result[0].x;
		}
		else
		{
			//// compute line coef : y = a * x + b, we already know the line is not vertical
			//	double a = (p1.y - p2.y) / (p1.x - p2.x);
			//	double b = p1.y - a * p1.x;

			//	double A = 1 + a * a;
			//	double B = 2 * (a * (b /*- center.y*/) /*- center.x*/);
			//	double C = /*center.x * center.x +*/ (b /*- center.y*/) * (b /*- center.y*/) - radius * radius;
			//	double delta = B * B - 4 * A * C;

			//if (delta < 0) {
			//	// this should not happen distance of center projection <= radius, that means line is tangeant
			//	//if (glm::abs(delta) < ERR_FLOAT)
			//	//	delta = 0;
			//	//else
			//	//	return 0;
			//	result[0] = projection(center, p1, p2);
			//	return 1;
			//}
			//if (delta == 0) {
			//	double x = -B / (2 * A);
			//	result[0] = glm::vec2(center.x + x, center.y + (a * x + b));
			//	return 1;
			//}
			//auto sqrt_delta = glm::sqrt(delta);
			//float x1 = (float)(((-B - sqrt_delta) / (2 * A)));
			//float x2 = (float)(((-B + sqrt_delta) / (2 * A)));
			//result[0] = glm::vec2(x1, a * x1 + b);
			//result[1] = glm::vec2(x2, a * x2 + b);
			double a = (p1.y - p2.y) / (p1.x - p2.x);
			double b = -1;
			double c = p1.y - a * p1.x;

			double x0 = -a * c / (a * a + b * b);
			double y0 = -b * c / (a * a + b * b);

			double r2 = radius * radius;
			double a2 = a * a;
			double b2 = b * b;
			double c2 = c * c;

			double d = r2 - c2 / (a2 + b2);
			double mult = sqrt(d / (a2 + b2));
			result[0].x = (float)(x0 + b * mult);
			result[1].x = (float)(x0 - b * mult);
			result[0].y = (float)(y0 - a * mult);
			result[1].y = (float)(y0 + a * mult);
			}

		result[0] += center;
		result[1] += center;

		return 2;
	}

	return 0;
}

int segment_circle_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2])
{
	glm::vec2 points[2];

	if (line_circle_intersect(p1, p2, center, radius, points))
	{
		int count = 0;

		if (colinear_segment_point(p1, p2, points[0]))
		{
			count++;
			result[0] = points[0];
		}

		if (colinear_segment_point(p1, p2, points[1]))
		{
			result[count] = points[1];
			count++;
		}

		return count;
	}
	return 0;
}

int circle_circle_intersect(glm::vec2 c1, float r1, glm::vec2 c2, float r2, glm::vec2 result[2])
{
	auto dist = glm::distance(c1, c2);
	auto radius = r1 + r2;

	if (dist == radius) // circles are tangent
	{
		result[0] = geometry::position(c2, r1, c1);
		return 1;
	}

	if (dist == 0 || dist > radius || dist < glm::abs(r1 - r2))
		return 0;

	// solving equation
	//float d = glm::sqrt((c2.x - c1.x) * (c2.x - c1.x) + (c2.y - c1.y) * (c2.y - c1.y));
	//double factor = glm::sqrt(((r1 + r2) * (r1 + r2) - d * d) * (d * d - (r2 - r1) * (r2 - r1)));
	//double x = (c2.x + c1.x) / 2 + ((c2.x - c1.x) * (r1 * r1 - r2 * r2)) / (2 * d * d);
	//double y = (c2.y + c1.y) / 2 + ((c2.y - c1.y) * (r1 * r1 - r2 * r2)) / (2 * d * d);
	//double x1 = x + ((c2.y - c1.y) / (2 * d * d)) * factor;
	//double x2 = x - ((c2.y - c1.y) / (2 * d * d)) * factor;
	//double y1 = y - ((c2.x - c1.x) / (2 * d * d)) * factor;
	//double y2 = y + ((c2.x - c1.x) / (2 * d * d)) * factor;
	//result[0] = glm::vec2(x1, y1);
	//result[1] = glm::vec2(x2, y2);

	double r1_2 = r1 * r1;
	double r2_2 = r2 * r2;
	double d_2 = dist * dist;
	double a = (r1_2 - r2_2 + d_2) / (2 * dist);
	double h = glm::sqrt(r1_2 - (a * a));
	double h_d = h / dist;
	glm::vec2 p3 = c1 + (float)((a / dist)) * (c2 - c1);
	result[0] = glm::vec2(p3.x + h_d * (c2.y - c1.y), p3.y - h_d * (c2.x - c1.x));
	result[1] = glm::vec2(p3.x - h_d * (c2.y - c1.y), p3.y + h_d * (c2.x - c1.x));

	if (h == 0)
		return 1;

	return 2;
}

int circle_polyline_intersect(glm::vec2 c1, float r1, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result)
{
	glm::vec2 points[2] { vec2_empty, vec2_empty };
	for (int i = 0; i < coordinates.size() - 1; i++)
	{
		auto count = segment_circle_intersect(coordinates[i], coordinates[i + 1], c1, r1, points);
		for(int j=0; j<count; j++)
		{
			result.push_back(points[j]);
		}
	}

	return (int)result.size();
}

int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, float start, glm::vec2 center, float stop, float radius, bool cw, glm::vec2 result[2], float err)
{
	glm::vec2 points[2];

	if (line_circle_intersect(p1, p2, center, radius, points, err))
	{
		int count = 0;

		for (glm::vec2 p : points)
		{
			if (colinear_arc_point(start, center, stop, cw, p))
			{
				result[count] = p;
				count++;
			}
		}

		return count;
	}
	return 0;
}

int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 result[2], float err)
{
	float start_a = geometry::oriented_angle(start, center);
	float stop_a = geometry::oriented_angle(stop, center);

	return line_arc_intersect(p1, p2, start_a, center, stop_a, radius, cw, result, err);
}

int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 result[2], float err)
{
	float radius = geometry::distance(start, center);
	return line_arc_intersect(p1, p2, start, center, stop, radius, cw, result, err);
}

int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, float start, glm::vec2 center, float stop, float radius, bool cw, glm::vec2 result[2])
{
	glm::vec2 points[2];

	if (line_circle_intersect(p1, p2, center, radius, points))
	{
		int count = 0;

		for (glm::vec2 p : points)
		{
			for (glm::vec2 p : points)
			{
				if (colinear_segment_point(p1, p2, p) && colinear_arc_point(start, center, stop, cw, p))
				{
					result[count] = p;
					count++;
				}
			}
		}

		return count;
	}
	return 0;
}

int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 result[2])
{
	float radius = geometry::distance(start, center);

	return segment_arc_intersect(p1, p2, start, center, stop, radius, cw, result);
}

int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 result[2])
{
	glm::vec2 points[2];
	
	if (radius == 0)
		radius = geometry::distance(start, center);

	if (line_circle_intersect(p1, p2, center, radius, points))
	{
		int count = 0;
		
		float start_a = geometry::oriented_angle(start, center);
		float stop_a  = geometry::oriented_angle(stop, center);

		for (glm::vec2 p : points)
		{
			if (colinear_segment_point(p1, p2, p) && colinear_arc_point(start_a, center, stop_a, cw, p))
			{
				result[count] = p;
				count++;
			}
		}

		return count;
	}
	return 0;
}

int arc_circle_intersect(glm::vec2 c1, float r1, float start, float stop, glm::vec2 c2, float r2, glm::vec2 result[2])
{
	glm::vec2 points[2];
	if (circle_circle_intersect(c1, r1, c2, r2, points))
	{
		int count = 0;

		for (glm::vec2 p : points)
		{
			auto angle = oriented_angle(p, c1);

			if (glm::abs(angle - start) > ERR_FLOAT && glm::abs(angle - stop) > ERR_FLOAT)
			{
				if (start < stop)
				{
					if (angle > start && angle < stop)
					{
						result[count] = p;
						count++;
					}
				}
				else
				{
					if (angle >= 0 && angle < stop || angle > start)
					{
						result[count] = p;
						count++;
					}
				}
			}
		}

		return count;
	}

	return 0;
}

int arc_circle_intersect(float arc_start, glm::vec2 arc_center, float arc_stop, float arc_radius, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2])
{
	glm::vec2 points[2];
	int count = circle_circle_intersect(arc_center, arc_radius, circle_center, circle_radius, points);
	if (count > 0)
	{
		int count_arc1 = 0;

		for (int i = 0; i < count; i++)
		{
			if (colinear_arc_point(arc_start, arc_center, arc_stop, arc_cw, points[i]))
			{
				result[count_arc1] = points[i];
				count_arc1++;
			}
		}

		return count_arc1;
	}

	return 0;
}

int arc_circle_intersect(glm::vec2 arc_start, glm::vec2 arc_center, glm::vec2 arc_stop, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2])
{
	float radius = geometry::distance(arc_start, arc_center);
	return arc_circle_intersect(arc_start, arc_center, arc_stop, radius, arc_cw, circle_center, circle_radius, result);
}

int arc_circle_intersect(glm::vec2 arc_start, glm::vec2 arc_center, glm::vec2 arc_stop, float arc_radius, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2])
{
	float start_a = geometry::oriented_angle(arc_start, arc_center);
	float stop_a  = geometry::oriented_angle(arc_stop, arc_center);
	return arc_circle_intersect(start_a, arc_center, stop_a, arc_radius, arc_cw, circle_center, circle_radius, result);
}

int arc_arc_intersect(float start1, glm::vec2 c1, float stop1, float r1, bool cw1, float start2, glm::vec2 c2, float stop2, float r2, bool cw2, glm::vec2 result[2])
{
	glm::vec2 points[2];
	int count = circle_circle_intersect(c1, r1, c2, r2, points);
	if (count > 0)
	{
		glm::vec2 points_arc1[2];
		int count_arc1 = 0, count_arc2 = 0;

		for (int i = 0; i < count; i++)
		{
			if (colinear_arc_point(start1, c1, stop1, cw1, points[i]))
			{
				points_arc1[count_arc1] = points[i];
				count_arc1++;
			}
		}

		if (count_arc1 > 0)
		{
			for (int i = 0; i < count_arc1; i++)
			{
				if (colinear_arc_point(start2, c2, stop2, cw2, points_arc1[i]))
				{
					result[count_arc2] = points_arc1[i];
					count_arc2++;
				}
			}
		}

		return count_arc2;
	}

	return 0;
}

int arc_arc_intersect(glm::vec2 start1, glm::vec2 c1, glm::vec2 stop1, bool cw1, glm::vec2 start2, glm::vec2 c2, glm::vec2 stop2, bool cw2, glm::vec2 result[2])
{
	float r1 = geometry::distance(c1, start1);
	float r2 = geometry::distance(c2, start2);

	return arc_arc_intersect(start1, c1, stop1, r1, cw1, start2, c2, stop2, r2, cw2, result);
}

int arc_arc_intersect(glm::vec2 start1, glm::vec2 c1, glm::vec2 stop1, float r1, bool cw1, glm::vec2 start2, glm::vec2 c2, glm::vec2 stop2, float r2, bool cw2, glm::vec2 result[2])
{
	float start1_a = geometry::oriented_angle(start1, c1);
	float stop1_a  = geometry::oriented_angle(stop1, c1);
	float start2_a = geometry::oriented_angle(start2, c2);
	float stop2_a  = geometry::oriented_angle(stop2, c2);
	return arc_arc_intersect(start1_a, c1, stop1_a, r1, cw1, start2_a, c2, stop2_a, r2, cw2, result);
}

int arc_polyline_intersect(float start, glm::vec2 c, float stop, float r, bool cw, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result)
{
	glm::vec2 points[2]{ vec2_empty, vec2_empty };
	for (int i = 0; i < coordinates.size() - 1; i++)
	{
		auto count = segment_arc_intersect(coordinates[i], coordinates[i + 1], start, c, stop, r, cw, points);
		for (int j = 0; j < count; j++)
		{
			result.push_back(points[j]);
		}
	}

	return (int)result.size();
}

int polyline_polyline_intersect(std::vector<glm::vec2> coord1, std::vector<glm::vec2> coord2, std::vector<glm::vec2>& result)
{
	glm::vec2 p = glm::vec2();
	for (size_t i = 0; i < coord1.size() - 1; i++)
	{
		for (size_t j = 0; j < coord2.size() - 1; j++)
		{

			if (segment_segment_intersect(coord1[i], coord1[i + 1], coord2[j], coord2[j + 1], p))
				result.push_back(p);
		}
	}

	return result.size() > 0;
}

bool cw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4)
{
	std::vector<glm::vec2> coords { p1, p2, p3, p4 };
	return cw(coords);
}

bool cw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
	std::vector<glm::vec2> coords{ p1, p2, p3 };
	return cw(coords);
}

bool cw(std::vector<glm::vec2>& points)
{
	bool pop = false;

	if (points.front() != points.back())
	{
		points.push_back(points.front());
		pop = true;
	}

	float area = 0;
	glm::vec2 f = points.front();
	auto pit = points.begin() + 1;
	while (pit != points.end())
	{
		area += (f.x * (*pit).y) - ((*pit).x * f.y);
		f = (*pit);
		pit++;
	}

	if (pop)
		points.pop_back();

	return area < 0;
}

glm::vec3 v3(glm::vec2 v, float z)
{
	return glm::vec3(v.x, v.y, z);
}

//float distance_between_segments(glm::vec2 s1, glm::vec2 d1, glm::vec2 s2, glm::vec2 d2)
//{
//	// both vertical
//	if (s1.x == d1.x && s2.x == d2.x)
//		return glm::abs(s1.x - s2.x);
//
//	// both horizontal
//	if (s1.y == d1.y && s2.y == d2.y)
//		return glm::abs(s1.y - s2.y);
//
//	float d = 0;
//
//	// test the projection of s1 on (s2,d2)
//	auto p = geometry::projection(s1, s2, d2);
//	if (geometry::colinear_segment_point(s2, d2, p))
//		d = geometry::distance(p, s1);
//
//	// test the projection of d1 on (s2,d2)
//	p = geometry::projection(d1, s2, d2);
//	if (geometry::colinear_segment_point(s2, d2, p))
//		d = glm::min(d, geometry::distance(p, d1));
//
//	// test the projection of s2 on (s1,d1)
//	p = geometry::projection(s2, s1, d1);
//	if (geometry::colinear_segment_point(s1, d1, p))
//		d = glm::min(d, geometry::distance(p, s2));
//
//	// test the projection of d2 on (s1,d1)
//	p = geometry::projection(d2, s1, d1);
//	if (geometry::colinear_segment_point(s1, d1, p))
//		d = glm::min(d, geometry::distance(p, d2));
//
//	// if no projection are on a segment, that's means min dist are
//	// between end points
//	if (d == 0)
//	{
//		d = glm::min(d, geometry::distance(s1, s2));
//		d = glm::min(d, geometry::distance(s1, d2));
//		d = glm::min(d, geometry::distance(d1, s2));
//		d = glm::min(d, geometry::distance(d1, d2));
//	}
//
//	return d;
//}
//
//float distance_between_segment_arc(glm::vec2 s1, glm::vec2 d1, glm::vec2 s2, glm::vec2 c2, glm::vec2 d2, bool cw2)
//{
//	// get the projection of arc center on (s1,d1)
//	auto p = geometry::projection(c2, s1, d1);
//	if (!colinear_segment_point(s1, d1, p))
//
//
//
//	glm::vec2 pp[2];
//	int count = geometry::segment_circle_intersect(c2, p, c2, geometry::distance(c2, s2), pp);
//
//	for (int i = 0; i < count; i++)
//	{
//		if (geometry::arc_point(s2, c2, d2, cw2, pp[i]) && segment_point(s1, d1, pp[i]))
//			return distance(pp[i])
//	}
//
//	return 0.0f;
//}
//
//float distance_between_arc_arc(glm::vec2 s1, glm::vec2 c1, glm::vec2 d1, bool cw1, glm::vec2 s2, glm::vec2 c2, glm::vec2 d2, bool cw2)
//{
//	return 0.0f;
//}
//
//float distance_between_segment_circle(glm::vec2 s1, glm::vec2 d1, glm::vec2 c2, float r2)
//{
//	return 0.0f;
//}
//
//float distance_between_arc_circle(glm::vec2 s1, glm::vec2 c1, glm::vec2 d1, bool cw1, glm::vec2 c2, float r2)
//{
//	return 0.0f;
//}
//
//float distance_between_circle_circle(glm::vec2 c1, float r1, glm::vec2 c2, float r2)
//{
//	return geometry::distance(c1, c2) - (r1 + r2);
//}

}