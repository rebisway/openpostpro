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

//////////////////////////////////////////////////////////////////////
//
// Angles are mainly expressed or need to be expressed [2PI]. oriented_angle() function can move any radian angle to [2PI]. 
// Be carefull as default c++/glm trigo functions use radian angles between (-PI;PI).
//


#pragma once
#ifndef _GEOMETRY_H
#define _GEOMETRY_H
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>
#include <ios>
#include <sstream>
#include <glm/gtx/transform.hpp>
#include <iomanip>
#include <vector>

namespace geometry
{
	inline static const glm::vec2 vec2_empty = glm::vec2(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	inline static const glm::vec3 vec3_empty = glm::vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	inline static const glm::vec4 vec4_empty = glm::vec4(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	inline static const glm::vec2 vec2_max = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	inline static const glm::vec3 vec3_max = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	inline static const glm::vec4 vec4_max = glm::vec4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());


	inline static const float ERR_FLOAT6 = 0.000001f;
	inline static const float ERR_FLOAT5 = 0.00001f;
	inline static const float ERR_FLOAT4 = 0.0001f;
	inline static const float ERR_FLOAT3 = 0.001f;
	inline static const float ERR_FLOAT2 = 0.01f;
	inline static const float ERR_FLOAT1 = 0.1f;
	inline static const float ERR_FLOAT  = ERR_FLOAT6;
	inline static const float ERR_CROSS  = 0.0005f;

	struct rectangle
	{
		glm::vec2 top_left;
		glm::vec2 bottom_right;

		float top() { return top_left.y; }
		float left() { return top_left.x; }
		float width() { return bottom_right.x - top_left.x; }
		float height() { return top_left.y - bottom_right.y; }
		float bottom() { return bottom_right.y; }
		float right() { return bottom_right.x; }
		float x() { return top_left.x; }
		float y() { return bottom_right.y; }

		rectangle() 
		{
			top_left = geometry::vec2_empty;
			bottom_right = geometry::vec2_empty;
		}

		rectangle(const rectangle& r)
		{
			top_left = r.top_left;
			bottom_right = r.bottom_right;
		}

		rectangle(glm::vec2& top_left, glm::vec2& bottom_right)
		{
			this->top_left = top_left;
			this->bottom_right = bottom_right;
		}

		rectangle(float left, float top, float right, float bottom)
		{
			top_left.x = left;
			top_left.y = top;
			bottom_right.x = right;
			bottom_right.y = bottom;
		}

		rectangle& operator=(const rectangle& r)
		{
			top_left = r.top_left;
			bottom_right = r.bottom_right;
			return *this;
		}

		bool empty()
		{
			return top_left == geometry::vec2_empty;
		}

		bool contains(glm::vec2& point)
		{
			return (point.x >= top_left.x && point.x <= bottom_right.x && point.y >= bottom_right.y && point.y <= top_left.y);
		}

		bool contains(rectangle& r)
		{
			return contains(r.top_left) && contains(r.bottom_right);
		}

		bool intersect(rectangle& r)
		{
			return (top_left.x < r.bottom_right.x && bottom_right.x > r.top_left.x && top_left.y > r.bottom_right.y && bottom_right.y < r.top_left.y);
		}

		bool outside(rectangle& r)
		{
			if (top_left.y < r.bottom_right.y ||	// rect2 over rect1
				bottom_right.y > r.top_left.y ||	// rect2 under rect1
				top_left.x > r.bottom_right.x ||	// rect2 on left of rect1
				bottom_right.x < r.top_left.x)		// rect2 on right of rect1
				return true;

			return false;
		}

		rectangle offset(float o)
		{
			return rectangle(top_left.x - o, top_left.y + o, bottom_right.x + o, bottom_right.y - o);
		}

		void max()
		{
			top_left.x = (std::numeric_limits<float>::max)();
			top_left.y = -(std::numeric_limits<float>::max)();
			bottom_right.x = -(std::numeric_limits<float>::max)();
			bottom_right.y = (std::numeric_limits<float>::max)();
		}
	};

	/// <summary>
	/// Return angle in radiant between -pi and pi with horizontal
	/// </summary>
	/// <param name="p">Point to test</param>
	/// <returns>Angle in radiant</returns>
	float angle(glm::vec2 p);
	
	/// <summary>
	/// Return angle in radiant between -pi and pi with horizontal and origin
	/// </summary>
	/// <param name="p">Point to test</param>
	/// <param name="center">offset center</param>
	/// <returns>Angle in radiant</returns>
	float angle(glm::vec2 p, glm::vec2 origin);

	/// <summary>
	/// Return angle in radiant between 0 and 2*pi, [2PI]
	/// Origin is coordinates (0;0)
	/// </summary>
	/// <param name="p">Point to test</param>
	/// <returns>Angle in radiant</returns>
	float oriented_angle(glm::vec2 p);

	/// <summary>
	/// Return angle in radiant between 0 and 2*pi, [2PI]
	/// of segment [origin, p]
	/// </summary>
	/// <param name="p"></param>
	/// <param name="origin"></param>
	/// <returns></returns>
	float oriented_angle(glm::vec2 p, glm::vec2 origin);

	/// <summary>
	/// Return angle between 3 points between 0 and 2*pi, [2PI]
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="origin"></param>
	/// <param name="p2"></param>
	/// <returns></returns>
	float oriented_angle(glm::vec2 p1, glm::vec2 p2, glm::vec2 origin);

	/// <summary>
	/// Convert radian angle between -PI and PI to an angle between 0 and 2*PI, [2PI]
	/// </summary>
	/// <param name="radian"></param>
	/// <returns></returns>
	float oriented_angle(float radian);

	/// <summary>
	/// Return oriented arc angle between 0 and 2*PI, [2PI]
	/// </summary>
	/// <param name="radian"></param>
	/// <returns></returns>
	float oriented_angle(float a_start, float a_stop, bool cw);

	/// <summary>
	/// Return oriented arc angle between 0 and 2*PI, [2PI]
	/// </summary>
	/// <param name="radian"></param>
	/// <returns></returns>
	float oriented_angle(glm::vec2 p1, glm::vec2 p2, bool cw);

	/// <summary>
	/// Return oriented arc angle between 0 and 2*PI
	/// </summary>
	/// <param name="radian"></param>
	/// <returns></returns>
	float oriented_angle(glm::vec2 p1, glm::vec2 p2, glm::vec2 origin, bool cw);

	/// <summary>
	/// Return the distance between 2 points
	/// </summary>
	/// <param name="p1">first ref_point</param>
	/// <param name="p2">second ref_point</param>
	/// <returns>the square distance</returns>
	float distance(glm::vec2 p1, glm::vec2 p2);

	/// <summary>
	/// Return the square distance between 2 points
	/// </summary>
	/// <param name="p1">first ref_point</param>
	/// <param name="p2">second ref_point</param>
	/// <returns>the square distance</returns>
	float distance2(glm::vec2 p1, glm::vec2 p2);

	/// <summary>
	/// Return square orthogonal distance between a ref_point and a line going through p1 and p2
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	float foretriangle2(glm::vec2 p1, glm::vec2 p2, glm::vec2 point);

	/// <summary>
	/// Return orthogonal distance between a ref_point and a line going through p1 and p2
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	float foretriangle(glm::vec2& p1, glm::vec2& p2, glm::vec2& point);

	/// <summary>
	/// Test if ref_point inside rectangle
	/// </summary>
	/// <param name="top_left"></param>
	/// <param name="bottom_right"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	bool rectangle_contains(glm::vec2& top_left, glm::vec2& bottom_right, glm::vec2& point);
	
	/// <summary>
	/// Test if ref_point inside rectangle
	/// </summary>
	/// <param name="r"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	bool rectangle_contains(rectangle& r, glm::vec2& point);

	/// <summary>
	/// Test if ref_point inside rectangle
	/// </summary>
	/// <param name="top_left"></param>
	/// <param name="bottom_right"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	bool rectangle_contains(float top, float left, float bottom, float right, glm::vec2& point);

	/// <summary>
	/// Test if rectangle 2 intersect or inside rectangle 1
	/// </summary>
	/// <param name="top_left1"></param>
	/// <param name="bottom_right1"></param>
	/// <param name="top_left2"></param>
	/// <param name="bottom_right2"></param>
	/// <returns></returns>
	bool rectangle_contains(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2);
	
	/// <summary>
	/// Test if rectangle 2 intersect or inside rectangle 1
	/// </summary>
	/// <param name="r1"></param>
	/// <param name="r2"></param>
	/// <returns></returns>
	bool rectangle_contains(rectangle& r1, rectangle& r2);

	/// <summary>
	/// Return true if the two rectangles intersects
	/// </summary>
	/// <param name="top_left1"></param>
	/// <param name="bottom_right1"></param>
	/// <param name="top_left2"></param>
	/// <param name="bottom_right2"></param>
	/// <returns></returns>
	bool rectangle_intersect(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2);

	/// <summary>
	/// Return true if the two rectangles intersects
	/// </summary>
	/// <param name="r1"></param>
	/// <param name="r2"></param>
	/// <returns></returns>
	bool rectangle_intersect(rectangle& r1, rectangle& r2);

	/// <summary>
	/// Return true if rectangles are strickly outside, no contains and no intersect
	/// </summary>
	/// <param name="top_left1"></param>
	/// <param name="bottom_right1"></param>
	/// <param name="top_left2"></param>
	/// <param name="bottom_right2"></param>
	/// <returns></returns>
	bool rectangle_outside(glm::vec2& top_left1, glm::vec2& bottom_right1, glm::vec2& top_left2, glm::vec2& bottom_right2);

	/// <summary>
	/// Return true if rectangles are strickly outside, no contains and no intersect
	/// </summary>
	/// <param name="r1"></param>
	/// <param name="r2"></param>
	/// <returns></returns>
	bool rectangle_outside(rectangle& r1, rectangle& r2);

	/// <summary>
	/// Interpolate circle vertices
	/// </summary>
	/// <param name="radius"></param>
	/// <param name="segment_length"></param>
	/// <returns></returns>
	std::vector<glm::vec2> circle(float radius, float segment_length = 1);

	/// <summary>
	/// Interpolate circle vertices
	/// </summary>
	/// <param name="radius"></param>
	/// <param name="center"></param>
	/// <param name="segment_length"></param>
	/// <returns></returns>
	std::vector<glm::vec2> circle(float radius, glm::vec2 center, float segment_length = 1);

	/// <summary>
	/// Interpolate arc vertices
	/// </summary>
	/// <param name="source"></param>
	/// <param name="center"></param>
	/// <param name="destination"></param>
	/// <param name="cw"></param>
	/// <param name="segment_length"></param>
	/// <returns></returns>
	std::vector<glm::vec2> arc(glm::vec2 source, glm::vec2 center, glm::vec2 destination, bool cw, float segment_length = 1);

	/// <summary>
	/// Interpolate an ellipse
	/// </summary>
	/// <param name="minor"></param>
	/// <param name="major"></param>
	/// <param name="start"></param>
	/// <param name="stop"></param>
	/// <param name="segment_length"></param>
	/// <returns></returns>
	std::vector<glm::vec2> ellipse(float minor, float major, float start = 0, float stop = glm::two_pi<float>(), float segment_length = 1);

	/// <summary>
	/// Interpolate an ellipse
	/// </summary>
	/// <param name="center"></param>
	/// <param name="minor"></param>
	/// <param name="major"></param>
	/// <param name="start"></param>
	/// <param name="stop"></param>
	/// <param name="angle"></param>
	/// <param name="segment_length"></param>
	/// <returns></returns>
	std::vector<glm::vec2> ellipse(glm::vec2 center, float minor, float major, float start = 0, float stop = glm::two_pi<float>(), float angle = 0.0f, float segment_length = 1);

	/// <summary>
	/// Convert a segment to a cubic bezier control points array
	/// </summary>
	/// <param name="from"></param>
	/// <param name="to"></param>
	/// <returns></returns>
	std::vector<glm::vec2> bezier_from_line(glm::vec2 from, glm::vec2 to);

	/// <summary>
	/// Convert a Quadratic bezier control points array to cubic bezier control points array
	/// </summary>
	/// <param name="spline"></param>
	/// <returns></returns>
	std::vector<glm::vec2> bezier_from_QBSpline(std::vector<glm::vec2> spline);

	/// <summary>
	/// Return the bounds of an arc.
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	rectangle arc_bounds(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw);
	
	/// <summary>
	/// Return the bounds of an arc.
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	rectangle arc_bounds(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw);

	/// <summary>
	/// Return the center of a circle going through 3 points
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <returns></returns>
	glm::vec2 circle_center(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);

	/// <summary>
	/// Return true if ref_point is part of arc
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <param name="ref_point"></param>
	/// <returns></returns>
	bool arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 point, float err = ERR_FLOAT);

	/// <summary>
	/// Return true if ref_point is part of arc
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <param name="ref_point"></param>
	/// <param name="err"></param>
	/// <returns></returns>
	bool arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 point, float err = ERR_FLOAT);

	/// <summary>
	/// Return true if ref_point is part of arc, radius is not tested, ref_point is supposed to be on circle
	/// </summary>
	/// <param name="start">start coordinates</param>
	/// <param name="center">center coordinates</param>
	/// <param name="stop">stop coordinates</param>
	/// <param name="cw">if true, clockwise</param>
	/// <param name="ref_point">coordinates to test</param>
	/// <returns></returns>
	bool colinear_arc_point(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 point);

	/// <summary>
	/// Return true if ref_point is part of arc, radius is not tested, ref_point is supposed to be on circle
	/// </summary>
	/// <param name="start">start angle in radian [2PI]</param>
	/// <param name="center">center coordinates</param>
	/// <param name="stop">stop angle in radian [2PI]</param>
	/// <param name="cw">if true, clockwise</param>
	/// <param name="ref_point">coordinates to test</param>
	/// <returns></returns>
	bool colinear_arc_point(float start, glm::vec2 center, float stop, bool cw, glm::vec2 point);

	/// <summary>
	/// Return the coordinates of the middle ref_point of arc. Axe [center;middle] is symmetry axe of arc.
	/// </summary>
	/// <param name="source"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	glm::vec2 arc_middle(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw);
	
	/// <summary>
	/// Return the coordinates of the middle ref_point of arc. Axe [center;middle] is symmetry axe of arc.
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	glm::vec2 arc_middle(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw);

	/// <summary>
	/// Return the thickness of an arc, the distance between the arc middle ref_point and the middle ref_point between start and stop
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	float arc_thickness(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw);
	
	/// <summary>
	/// Return the thickness of an arc, the distance between the arc middle ref_point and the middle ref_point between start and stop
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	float arc_thickness(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw);

	/// <summary>
	/// Return the length of an arc
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	float arc_length(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw);

	/// <summary>
	/// Return the length of an arc
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <returns></returns>
	float arc_length(glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw);

	/// <summary>
	/// If test ref_point is before start, return -1, if test is between start ans stop return 0, if after stop return 1
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	int arc_position(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 test);

	/// <summary>
	/// If test ref_point is before start return true
	/// </summary>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	bool arc_position_left(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 test);

	/// <summary>
	/// Return coordinates on a circle from angle and radius
	/// </summary>
	/// <param name="angle">angle in radian</param>
	/// <param name="radius">radius</param>
	/// <returns>coordinates</returns>
	glm::vec2 position(float angle, float radius);

	/// <summary>
	/// Return coordinates on a circle from angle and radius
	/// </summary>
	/// <param name="angle">angle in radian</param>
	/// <param name="radius">radius</param>
	/// <param name="origin">position of the center of circle</param>
	/// <returns></returns>
	glm::vec2 position(float angle, float radius, glm::vec2 origin);

	/// <summary>
	/// Return coordinates on a circle from angle and radius
	/// </summary>
	/// <param name="source">coordinates of original source ref_point</param>
	/// <param name="radius">radius to apply</param>
	/// <param name="origin">position of the center of circle</param>
	/// <returns></returns>
	glm::vec2 position(glm::vec2 source, float radius, glm::vec2 origin);

	/// <summary>
	/// convert a float/double to string with a decimal precision number
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="a_value">value to convert</param>
	/// <param name="n">number of decimal</param>
	/// <returns>a string representation</returns>
	template <typename T> std::string to_string(const T value, const int n);

	template <typename T> std::string to_string(const T value);

	/// <summary>
	/// Return vec2 to std::string such as (x;y)
	/// </summary>
	/// <param name="v">vector to print</param>
	/// <returns>A std::string representation</returns>
	std::string to_string(glm::vec2 v, const int n = 6, char separator = ';');


	/// <summary>
	/// Return vec3 to std::string such as (x;y;z)
	/// </summary>
	/// <param name="v">vector to print</param>
	/// <returns>A std::string representation</returns>
	std::string to_string(glm::vec3 v, const int n = 6, char separator = ';');


	/// <summary>
	/// Return vec3 to std::string such as (r;g;b;a)
	/// </summary>
	/// <param name="v">vector to print</param>
	/// <returns>A std::string representation</returns>
	std::string to_string(glm::vec4 v, const int n = 6, char separator = ';');

	glm::vec4 from_string(std::string value, char separator = ';');
	glm::vec4 from_string4(std::string value, char separator = ';');
	glm::vec3 from_string3(std::string value, char separator = ';');
	glm::vec2 from_string2(std::string value, char separator = ';');


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                           curve helper methods                                               //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::vector<glm::vec2> cubic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float scale = 1.0f);

	std::vector<glm::vec2> cubic(std::vector<glm::vec2> points, float scale = 1.0f);

	void cubic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, std::vector<glm::vec2>& coords, float scale = 1.0f);

	std::vector<glm::vec2> quadratic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, float scale = 1.0f);

	std::vector<glm::vec2> quadratic(std::vector<glm::vec2> points, float scale = 1.0f);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// 
	/// </summary>
	/// <param name="points"></param>
	/// <returns></returns>
	std::vector<glm::vec2>bezier(std::vector<glm::vec2> points, float scale = 1.0f);

	std::vector<glm::vec2> bspline(std::vector<glm::vec2>& points);

	std::vector<glm::vec2> catmull(std::vector<glm::vec2>& points);

	std::vector<glm::vec2> lagrange(std::vector<glm::vec2>& points);
		
	std::vector<glm::vec2> naturalCubic(std::vector<glm::vec2>& points);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                       end curve helper methods                                               //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// reduce number of segment in polyline
	/// </summary>
	/// <param name="vertices">polyline</param>
	/// <param name="max">max length</param>
	/// <returns></returns>
	std::vector<glm::vec2> reduce(std::vector<glm::vec2>& vertices, float max = 1.0f);

	/// <summary>
	/// reduce number of segment in polylines
	/// </summary>
	/// <param name="vertices">polylines</param>
	/// <param name="max">max length</param>
	/// <returns></returns>
	std::vector<std::vector<glm::vec2>> reduce(std::vector<std::vector<glm::vec2>>& vertices, float max = 1.0f);

	/// <summary>
	/// Add a constant to a vector
	/// </summary>
	/// <param name="p">vector to offset</param>
	/// <param name="value">constant to add</param>
	void offset(glm::vec2& p, float value);

	/// <summary>
	/// Add a constant to a vector
	/// </summary>
	/// <param name="p">vector to offset</param>
	/// <param name="value">constant to add</param>
	void offset(glm::vec3& p, float value);

	/// <summary>
	/// return the coordinates of orthogonal projection of a ref_point on a segment
	/// </summary>
	/// <param name="p">ref_point to project</param>
	/// <param name="p1">first segment ref_point</param>
	/// <param name="p2">second segment ref_point</param>
	/// <returns>coordinates of projection</returns>
	glm::vec2 projection(glm::vec2 p, glm::vec2 p1, glm::vec2 p2);

	/// <summary>
	/// return the symmetry ref_point with an axe of symmetry
	/// </summary>
	/// <param name="p">source</param>
	/// <param name="p1">first axe ref_point</param>
	/// <param name="p2">second axe ref_point</param>
	/// <returns>symmetry ref_point</returns>
	glm::vec2 symmetry(glm::vec2 p, glm::vec2 p1, glm::vec2 p2);

	/// <summary>
	/// return the symmetry ref_point with a center of symmetry
	/// </summary>
	/// <param name="p">source</param>
	/// <param name="center">center of symmetry</param>
	/// <returns>symmetry ref_point</returns>
	glm::vec2 symmetry(glm::vec2 p, glm::vec2 center);

	/// <summary>
	/// Return the coordinates of the middle of segment [p1;p2]
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <returns></returns>
	glm::vec2 middle(glm::vec2 p1, glm::vec2 p2);

	/// <summary>
	/// Return true if test ref_point is between p1 and p2.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	bool segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test);

	/// <summary>
	/// Return true if test ref_point is between p1 and p2. test ref_point is supposed to be colinear.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	bool colinear_segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test);
	bool colinear_segment_point(glm::vec2 p1, glm::vec2 p2, glm::vec2 test, float err);
	/// <summary>
	/// Return true if segments are colinear.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <param name="p4"></param>
	/// <returns></returns>
	bool colinear_segments(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4);


	/// <summary>
	/// If test ref_point is before start, return -1, if test is between start ans stop return 0, if test is after stop return 1
	/// test ref_point is supposed to be colinear
	/// </summary>
	/// <param name="start"></param>
	/// <param name="stop"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	int segment_position(glm::vec2 start, glm::vec2 stop, glm::vec2 test);

	/// <summary>
	/// If test ref_point is before start return true
	/// test ref_point is supposed colinear
	/// </summary>
	/// <param name="start"></param>
	/// <param name="stop"></param>
	/// <param name="test"></param>
	/// <returns></returns>
	bool segment_position_left(glm::vec2 start, glm::vec2 stop, glm::vec2 test);

	/// <summary>
	/// Return true if lines itersect. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <param name="p4"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	bool line_line_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2& result);

	/// <summary>
	/// Return true if segments itersect. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <param name="p4"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	bool segment_segment_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2& result);

	/// <summary>
	/// Return true if a segment intersect with a polyline. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="coordinates"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	bool segment_polyline_intersect(glm::vec2 p1, glm::vec2 p2, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a line and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="center"></param>
	/// <param name="radius"></param>
	/// <param name="result"></param>
	/// <param name="err"></param>
	/// <returns></returns>
	int line_circle_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2], float err = ERR_FLOAT);
	int line_circle_intersect2(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2], float err = ERR_FLOAT);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a segment and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="center"></param>
	/// <param name="radius"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int segment_circle_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 center, float radius, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a circle and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="c1"></param>
	/// <param name="r1"></param>
	/// <param name="c2"></param>
	/// <param name="r2"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int circle_circle_intersect(glm::vec2 c1, float r1, glm::vec2 c2, float r2, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections between a circle and a polyline. Coordinates are stored in result.
	/// </summary>
	/// <param name="c1"></param>
	/// <param name="r1"></param>
	/// <param name="coordinates"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int circle_polyline_intersect(glm::vec2 c1, float r1, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a line and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1">first line ref_point coordinates</param>
	/// <param name="p2">second line ref_point coordinates</param>
	/// <param name="start">start angle [2PI]. see oriented_angle().</param>
	/// <param name="center">center coordinates</param>
	/// <param name="stop">stop angle [2PI]. see oriented_angle().</param>
	/// <param name="radius">arc radius, if NULL, radius is computed</param>
	/// <param name="cw">if true arc is clockwise</param>
	/// <param name="result"></param>
	/// <param name="err"></param>
	/// <returns></returns>
	int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, float start, glm::vec2 center, float stop, float radius, bool cw, glm::vec2 result[2], float err = ERR_FLOAT);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a line and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <param name="result"></param>
	/// <param name="err"></param>
	/// <returns></returns>
	int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 result[2], float err = ERR_FLOAT);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a line and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <param name="result"></param>
	/// <param name="err"></param>
	/// <returns></returns>
	int line_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 result[2], float err = ERR_FLOAT);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a segment and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="start">angle is [2PI]. see oriented_angle().</param>
	/// <param name="center"></param>
	/// <param name="stop">angle is [2PI]. see oriented_angle().</param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, float start, glm::vec2 center, float stop, float radius, bool cw, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a segment and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="start"></param>
	/// <param name="center"></param>
	/// <param name="stop"></param>
	/// <param name="cw"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between a segment and an arc. Coordinates are stored in result.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="start">angle is [2PI]. see oriented_angle().</param>
	/// <param name="center"></param>
	/// <param name="stop">angle is [2PI]. see oriented_angle().</param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int segment_arc_intersect(glm::vec2 p1, glm::vec2 p2, glm::vec2 start, glm::vec2 center, glm::vec2 stop, float radius, bool cw, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between an arc and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="arc_start">angle is [2PI]. see oriented_angle().</param>
	/// <param name="arc_center"></param>
	/// <param name="arc_stop">angle is [2PI]. see oriented_angle().</param>
	/// <param name="arc_radius"></param>
	/// <param name="arc_cw">true if arc is clockwise</param>
	/// <param name="circle_center"></param>
	/// <param name="circle_radius"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_circle_intersect(float arc_start, glm::vec2 arc_center, float arc_stop, float arc_radius, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2]);
	
	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between an arc and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="arc_start"></param>
	/// <param name="arc_center"></param>
	/// <param name="arc_stop"></param>
	/// <param name="arc_cw">true if arc is clockwise</param>
	/// <param name="circle_center"></param>
	/// <param name="circle_radius"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_circle_intersect(glm::vec2 arc_start, glm::vec2 arc_center, glm::vec2 arc_stop, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2]);
	
	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between an arc and a circle. Coordinates are stored in result.
	/// </summary>
	/// <param name="arc_start"></param>
	/// <param name="arc_center"></param>
	/// <param name="arc_stop"></param>
	/// <param name="arc_radius"></param>
	/// <param name="arc_cw">true if arc is clockwise</param>
	/// <param name="circle_center"></param>
	/// <param name="circle_radius"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_circle_intersect(glm::vec2 arc_start, glm::vec2 arc_center, glm::vec2 arc_stop, float arc_radius, bool arc_cw, glm::vec2 circle_center, float circle_radius, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between two arcs. Coordinates are stored in result.
	/// </summary>
	/// <param name="start1">angle is [2PI]. see oriented_angle().</param>
	/// <param name="c1"></param>
	/// <param name="stop1">angle is [2PI]. see oriented_angle().</param>
	/// <param name="r1"></param>
	/// <param name="cw1">true if arc is clockwise</param>
	/// <param name="start2">angle is [2PI]. see oriented_angle().</param>
	/// <param name="c2"></param>
	/// <param name="stop2">angle is [2PI]. see oriented_angle().</param>
	/// <param name="r2"></param>
	/// <param name="cw2">true if arc is clockwise</param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_arc_intersect(float start1, glm::vec2 c1, float stop1, float r1, bool cw1, float start2, glm::vec2 c2, float stop2, float r2, bool cw2, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between two arcs. Coordinates are stored in result.
	/// </summary>
	/// <param name="start1"></param>
	/// <param name="c1"></param>
	/// <param name="stop1"></param>
	/// <param name="cw1">true if arc is clockwise</param>
	/// <param name="start2"></param>
	/// <param name="c2"></param>
	/// <param name="stop2"></param>
	/// <param name="cw2">true if arc is clockwise</param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_arc_intersect(glm::vec2 start1, glm::vec2 c1, glm::vec2 stop1, bool cw1, glm::vec2 start2, glm::vec2 c2, glm::vec2 stop2, bool cw2, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between two arcs. Coordinates are stored in result.
	/// </summary>
	/// <param name="start1"></param>
	/// <param name="c1"></param>
	/// <param name="stop1"></param>
	/// <param name="r1"></param>
	/// <param name="cw1">true if arc is clockwise</param>
	/// <param name="start2"></param>
	/// <param name="c2"></param>
	/// <param name="stop2"></param>
	/// <param name="r2"></param>
	/// <param name="cw2">true if arc is clockwise</param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_arc_intersect(glm::vec2 start1, glm::vec2 c1, glm::vec2 stop1, float r1, bool cw1, glm::vec2 start2, glm::vec2 c2, glm::vec2 stop2, float r2, bool cw2, glm::vec2 result[2]);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between an arc and a polyline. Coordinates are stored in result.
	/// </summary>
	/// <param name="start">angle is [2PI]. see oriented_angle().</param>
	/// <param name="c"></param>
	/// <param name="stop">angle is [2PI]. see oriented_angle().</param>
	/// <param name="r">radius</param>
	/// <param name="cw">true if arc is clockwise</param>
	/// <param name="coordinates"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int arc_polyline_intersect(float start, glm::vec2 c, float stop, float r, bool cw, std::vector<glm::vec2> coordinates, std::vector<glm::vec2>& result);

	/// <summary>
	/// Return the number of intersections (0, 1  or 2) between two polylines. Coordinates are stored in result.
	/// </summary>
	/// <param name="coord1"></param>
	/// <param name="coord2"></param>
	/// <param name="result"></param>
	/// <returns></returns>
	int polyline_polyline_intersect(std::vector<glm::vec2> coord1, std::vector<glm::vec2> coord2, std::vector<glm::vec2>& result);

	/// <summary>
	/// Return true if the points are oriented clockwise
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <param name="p4"></param>
	/// <returns></returns>
	bool cw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4);
	bool cw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);

	/// <summary>
	/// Return true if the points are oriented clockwise
	/// </summary>
	/// <param name="points"></param>
	/// <returns></returns>
	bool cw(std::vector<glm::vec2>& points);

	/// <summary>
	/// Convert a glm::vec2 into glm::vec3
	/// </summary>
	/// <param name="v"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	glm::vec3 v3(glm::vec2 v, float z = 0);


	//float distance_between_segments(glm::vec2 s1, glm::vec2 d1, glm::vec2 s2, glm::vec2 d2);

	//float distance_between_segment_arc(glm::vec2 s1, glm::vec2 d1, glm::vec2 s2, glm::vec2 c2, glm::vec2 d2, bool cw2);

	//float distance_between_arc_arc(glm::vec2 s1, glm::vec2 c1, glm::vec2 d1, bool cw1, glm::vec2 s2, glm::vec2 c2, glm::vec2 d2, bool cw2);
	//
	//float distance_between_segment_circle(glm::vec2 s1, glm::vec2 d1, glm::vec2 c2, float r2);

	//float distance_between_arc_circle(glm::vec2 s1, glm::vec2 c1, glm::vec2 d1, bool cw1, glm::vec2 c2, float r2);

	//float distance_between_circle_circle(glm::vec2 c1,float r1, glm::vec2 c2, float r2);
}


#endif // GLM_EXT
