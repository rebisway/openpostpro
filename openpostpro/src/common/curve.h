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

#ifndef _CURVE_H
#define _CURVE_H

#include <glm/glm.hpp>
#include <vector>
#include <variant>
#include <deque>
#include <forward_list>
#include <deque>
#include <list>
#include <ftree.h>

enum class SegmentType
{
	Line,
	Arc,
	Circle
};

class Segment
{
private:

public:
	glm::vec2 point = glm::vec2();
	glm::vec2 center = glm::vec2();
	float radius = 0;
	SegmentType type = SegmentType::Line;
	bool cw = false;
	int tag = -1;
	int next_tag = -1;
	float length = -1;
	std::list<Segment>::iterator next;
	bool excluded = false;
	int index = -1;

#ifdef _DEBUG
	float perimeter = 0;
#endif

	geometry::rectangle bounds = geometry::rectangle();

	Segment() {}
	Segment(glm::vec2 point, int tag = -1);
	Segment(glm::vec2 point, glm::vec2 center, float radius, bool cw, SegmentType type = SegmentType::Arc, int tag = -1, bool excluded=false);

	int intersect(glm::vec2 dst1, Segment s2, glm::vec2 dst2, glm::vec2 result[2]);

	glm::vec2 coordinates_at(float length, glm::vec2 dst);
	std::vector<Segment> split_at(float length, glm::vec2 dst);
};

class SegmentUntrim : public Segment
{
public:
	glm::vec2 dst = glm::vec2();

	SegmentUntrim(glm::vec2 point, glm::vec2 dst, int index, bool excluded = false) : Segment(point)
	{
		this->dst = dst;
		this->index = index;
		this->excluded = excluded;
	};

	SegmentUntrim(glm::vec2 point, glm::vec2 center, glm::vec2 dst, float radius, bool cw, int index, SegmentType type = SegmentType::Arc) : Segment(point, center, radius, cw, type)
	{
		this->dst = dst;
		this->index = index;
	};
};

enum class Position
{
	inside,
	outside,
	intersect,
	outline
};

/// <summary>
/// A curve is composed with segments and arcs
/// If arc, then arc start coordinates are current arc segment ref_point, end coordinates are next segment ref_point
/// </summary>
class Curve : public std::vector<Segment>
{
private:
	float _length = -1;
	float _area = -1;
	int _cw = -1;
	geometry::rectangle _bounds;

	std::vector<Segment> _sorted;
	QuadNode<Segment> _tree;
	int _tag = -1;
	int _reference = -1;
	int _index = -1;
	int _level = -1;
	Position _tag_inside = Position::outline;

public:
	static int counter[10];
	std::vector<glm::vec2> intersection_points; // for debug purpose

	int tag();
	void tag(int value);

	int reference();
	void reference(int value);

	int index();
	void index(int value);

	int level();
	void level(int value);

	Position tag_inside();
	void tag_inside(Position value);

	geometry::rectangle& bounds();

	void reset_bounds();

	/// <summary>
	/// Add a segment ref_point
	/// </summary>
	/// <param name="ref_point">ref_point coordinates</param>
	void add(glm::vec2 point);

	/// <summary>
	/// Add a arc ref_point, 
	/// </summary>
	/// <param name="ref_point">start coordinates</param>
	/// <param name="center">center coordinates</param>
	/// <param name="radius">radius</param>
	/// <param name="cw">if true, clockwise</param>
	void add(glm::vec2 point, glm::vec2 center, float radius, bool cw);

	/// <summary>
	/// Add a circle ref_point. This method will add twice the 0 radian coordinates, for start and stop
	/// </summary>
	/// <param name="center">center coordinates</param>
	/// <param name="radius">radius</param>
	/// <param name="cw">if true, clockwise</param>
	void add(glm::vec2 center, float radius, bool cw);

	/// <summary>
	/// Add following the type
	/// </summary>
	/// <param name="type"></param>
	/// <param name="ref_point"></param>
	/// <param name="center"></param>
	/// <param name="radius"></param>
	/// <param name="cw"></param>
	void add(SegmentType type, glm::vec2 point, glm::vec2 center, float radius, bool cw);

	/// <summary>
	/// Add following the type
	/// </summary>
	/// <param name="Segment"></param>
	void add(Segment& s);

	/// <summary>
	/// Return curve start coordinates
	/// </summary>
	/// <returns>coordinates</returns>
	glm::vec2 first();

	/// <summary>
	/// return curve stop coordinates
	/// </summary>
	/// <returns>corrdinates</returns>
	glm::vec2 last();

	/// <summary>
	/// return true if closed, start and stop coordinates are equal
	/// </summary>
	/// <returns>bool</returns>
	bool closed();

	/// <summary>
	/// close the curve
	/// </summary>
	void close() { if (!closed()) push_back(Segment(front().point)); }

	/// <summary>
	/// return true if curve is clockwise
	/// </summary>
	/// <returns>bool</returns>
	bool cw();

	/// <summary>
	/// set the curve direction
	/// </summary>
	/// <param name="direction">if true, clockwise</param>
	void cw(bool direction);

	/// <summary>
	/// reverse curve direction
	/// </summary>
	void reverse();

	/// <summary>
	/// return curve length
	/// </summary>
	/// <returns></returns>
	float length();

	/// <summary>
	/// return curve area
	/// </summary>
	/// <returns></returns>
	float area();

	/// <summary>
	/// return true if ref_point p is inside the curve. if not closed, start coordinate is added for computation
	/// </summary>
	/// <param name="p">coordinates to test</param>
	/// <returns></returns>
	bool inside(glm::vec2 p);

	/// <summary>
	/// return true if curve is inside b
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	bool inside(Curve& b);

	/// <summary>
	/// return true if curve is outside b curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	bool outside(Curve& b);

	/// <summary>
	/// return the relative position of curve against b curve
	/// inside : curve is inside b
	/// outside : curve is outside b
	/// intersect : curve interset b
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	Position relative(Curve& b);

	/// <summary>
	/// return true if curve intersects with b curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	bool intersect(Curve& b);

	/// <summary>
	/// rotate the curve to set the index as the start coordinates
	/// </summary>
	/// <param name="index">index to be first segement</param>
	void rotate(int index);

	/// <summary>
	/// return coordinates of the curve, arcs are interpolated
	/// </summary>
	/// <returns></returns>
	std::vector<glm::vec2> coordinates();

	/// <summary>
	/// Cut a curve at the ref_point coordinates. Point is supposed to be part of the segment curve. There is no verification
	/// </summary>
	/// <param name="ref_point"></param>
	/// <param name="index"></param>
	/// <returns></returns>
	std::vector<Curve> cut(glm::vec2 point, int index);

	/// <summary>
	/// Cut a curve at the ref_point coordinates. Point is supposed to be part of the segment curve. There is no verification
	/// </summary>
	/// <param name="ref_point"></param>
	/// <param name="it"></param>
	/// <returns></returns>
	std::vector<Curve> cut(glm::vec2 point, std::vector<Segment>::iterator it);

	/// <summary>
	/// Return a curve which append curve b to current curve 
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	std::vector<Curve> merge(Curve& b);

	/// <summary>
	/// Apply scale factor to the curve
	/// </summary>
	/// <param name="factor"></param>
	void scale(float factor);

	/// <summary>
	/// Return a curve that represent the boolean union of current curve with b curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	std::vector<Curve> boolean_union(Curve& b);

	/// <summary>
	/// Return a curve that represent the boolean substraction of b curve to current curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	std::vector<Curve> boolean_substract(Curve& b);

	/// <summary>
	/// Return a curve that represent the boolean intersection of current curve with b curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	std::vector<Curve> boolean_intersect(Curve& b);


	/// <summary>
	/// Return a curve that represent the boolean intersection of current curve with b curve
	/// </summary>
	/// <param name="b"></param>
	/// <returns></returns>
	std::vector<Curve> boolean_not_intersect(Curve& b);

	/// <summary>
	/// create an offseted curve. if cw && positive offset or !cw && negative offset, outside, else inside
	/// </summary>
	/// <param name="o">offset value</param>
	/// <returns>new curve</returns>
	std::vector<Curve> offset(float o, float max=0);

	/// <summary>
	/// fill position with the middle coordinates of the curve, angle with the derivative angle at position
	/// </summary>
	/// <param name="position"></param>
	/// <param name="angle"></param>
	void middle(glm::vec2& position, float& angle);

	/// <summary>
	/// split the actual curve at lenth position and return the two resulted curves
	/// </summary>
	/// <param name="length"></param>
	/// <returns></returns>
	std::vector<Curve> split(float length);

	/// <summary>
	/// Remove segments less than max
	/// </summary>
	/// <param name="max"></param>
	void reduce(float max);

public:
	/// <summary>
	/// Internal purpose
	/// Return an untrim segment list. Each original segment is offseted by distance o. 
	/// If o is negative, the offset is done inside the curve otherwise outside;
	/// If the original curve is not closed, the result depends of the clockwise direction computed by temporaly closing the original curve.
	/// </summary>
	/// <param name="o">Offset</param>
	/// <returns></returns>
	std::vector<SegmentUntrim> untrim(float o);

	/// <summary>
	/// Internal purpose
	/// Link untrimed segments to created a closed curve
	/// </summary>
	/// <param name="original"></param>
	/// <param name="o"></param>
	/// <returns></returns>
	Curve trim(std::vector<SegmentUntrim>& original, float o);

	/// <summary>
	/// Replace the spikes (a segment going back along the previous one) of a raw offset curve by a single segment
	/// </summary>
	void remove_spikes();

	/// <summary>
	/// Split the curve into several parts following the self intersections points detected
	/// </summary>
	/// <returns></returns>
	std::vector<Curve> split_at_intersections();

	/// <summary>
	/// Same as split_at_intersections() but the pieces to keep are chosen with the winding number of the raw curve, which does not
	/// depend on the order of intersections that are very close to each other
	/// </summary>
	/// <param name="orientation">1 if the valid area is on the left of the curve (counter clockwise), -1 if it is on the right</param>
	std::vector<Curve> split_at_intersections(int orientation);

	bool too_small(float o);

	bool too_close(Curve& test, float o);
	bool too_close2(Curve& test, float o);

	bool curve_intersect(Curve& test);

	std::vector<Segment> search(geometry::rectangle bounds);

	static void sort_level(std::vector<Curve>& curves);

private:

	/// <summary>
	/// Internal purpose
	/// Return curve area without arc interpolation for clockwise computing. if area < 0, then clockwise
	/// </summary>
	/// <returns></returns>
	float limited_area();

	/// <summary>
	/// Return the result of union/intersection/substraction using Weiler algorythm
	/// </summary>
	/// <param name="b">second curve</param>
	/// <param name="outside">if true start with outside segment for union/sub, false for intersect/xor</param>
	/// <returns></returns>
	std::vector<Curve> weiler(Curve& b, bool same_direction, bool outside);
};


struct TreeCurve
{
	Curve* curve = nullptr;
	int level = 0;
	std::vector<TreeCurve*> children;
	void add(TreeCurve* c);
	void sort();
	void cw_all(bool direction);
	void cw_alter();
	void cw_alter(bool direction);
	TreeCurve(Curve* c);
	TreeCurve(std::vector<Curve>& curves);
	~TreeCurve();
	size_t size();
	std::vector<Curve> curves();

	//TreeCurve& operator&(const TreeCurve& b);
};

#endif