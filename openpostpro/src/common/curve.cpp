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

#include "curve.h"

#include <geometry.h>
#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <iostream>
#include <logger.h>
#include <ftree.h>


Segment::Segment(glm::vec2 point, int tag)
{
	this->point = point;
	this->type = SegmentType::Line;
	this->tag = tag;
}

Segment::Segment(glm::vec2 point, glm::vec2 center, float radius, bool cw, SegmentType type, int tag, bool excluded)
{
	this->point = point;
	this->center = center;
	this->radius = radius;
	this->cw = cw;
	this->type = type;
	this->tag = tag;
	this->excluded = excluded;
#ifdef _DEBUG
	this->perimeter = glm::two_pi<float>() * radius;
#endif
}

int Segment::intersect(glm::vec2 dst1, Segment s2, glm::vec2 dst2, glm::vec2 result[2])
{
	Segment& s1 = *this;

	if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
	{
		if (geometry::segment_segment_intersect(s1.point, dst1, s2.point, dst2, result[0]))
			return 1;
		else
			return 0;
	}
	else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
	{
		return geometry::segment_arc_intersect(s1.point, dst1, s2.point, s2.center, dst2, s2.cw, result);
	}
	else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
	{
		return geometry::segment_arc_intersect(s2.point, dst2, s1.point, s1.center, dst1, s1.cw, result);
	}
	else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
	{
		return geometry::arc_arc_intersect(s2.point, s2.center, dst2, s2.cw, s1.point, s1.center, dst1, s1.cw, result);
	}
	else if (s1.type == SegmentType::Circle && s2.type == SegmentType::Line)
	{
		return geometry::segment_circle_intersect(s2.point, dst2, s1.center, s1.radius, result);
	}
	else if (s1.type == SegmentType::Line && s2.type == SegmentType::Circle)
	{
		return geometry::segment_circle_intersect(s1.point, dst1, s2.center, s2.radius, result);
	}
	else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Circle)
	{
		return geometry::arc_circle_intersect(s1.point, s1.center, dst1, s1.cw, s2.center, s2.radius, result);
	}
	else if (s1.type == SegmentType::Circle && s2.type == SegmentType::Arc)
	{
		return geometry::arc_circle_intersect(s2.point, s2.center, dst2, s2.cw, s1.center, s1.radius, result);
	}

	return 0;
}

glm::vec2 Segment::coordinates_at(float length, glm::vec2 dst)
{
	if (length == 0)
		return point;

	float a = 0, a1 = 0;

	switch (type)
	{
	case SegmentType::Line:
		return geometry::position(dst, length, point);
	case SegmentType::Arc:
	case SegmentType::Circle:
		a = length / radius;
		// start angle of the arc (or the circle), then moved along it by the requested length, in its direction
		a1 = geometry::oriented_angle(point, center);
		if (cw)
			a = geometry::oriented_angle(a1 - a);
		else
			a = geometry::oriented_angle(a1 + a);
		return glm::vec2(glm::cos(a), glm::sin(a)) * radius + center;
	}

	return glm::vec2();
}

std::vector<Segment> Segment::split_at(float length, glm::vec2 dst)
{
	std::vector<Segment> result;
	switch (type)
	{
	case SegmentType::Line:
	{
		auto p = geometry::position(dst, length, point);
		result.push_back(*this);
		result.push_back(Segment(p));
		break;
	}
	case SegmentType::Arc:
	case SegmentType::Circle:
		result.push_back(Segment(point, center, radius, cw, SegmentType::Arc));
		result.push_back(Segment(coordinates_at(length, dst), center, radius, cw, SegmentType::Arc));
		break;
	}
	return result;
}


bool compare_curve_size(TreeCurve* c1, TreeCurve* c2)
{
	return (c1->curve->size() > c2->curve->size());
}

bool compare_treecurve_tag(TreeCurve* v1, TreeCurve* v2)
{
	if (v1->curve->tag() > -1 && v2->curve->tag() > -1)
		return (v1->curve->tag() < v2->curve->tag());
	else
		return v1->curve->tag() > -1;
}

bool compare_curve_tag(Curve& v1, Curve& v2)
{
	if (v1.tag() > -1 && v2.tag() > -1)
		return (v1.tag() < v2.tag());
	else
		return v1.index() < v2.index();
}

bool compare_curve_level(Curve& v1, Curve& v2)
{
	return v1.level() < v2.level();
}


thread_local int Curve::counter[10];

int Curve::tag()
{
	return _tag;
}

void Curve::tag(int value)
{
	_tag = value;
}

int Curve::reference()
{
	return _reference;
}

void Curve::reference(int value)
{
	_reference = value;
}

int Curve::index()
{
	return _index;
}

void Curve::index(int value)
{
	_index = value;
}


int Curve::level()
{
	return _level;
}

void Curve::level(int value)
{
	_level = value;
}

Position Curve::tag_inside()
{
	return _tag_inside;
}

void Curve::tag_inside(Position value)
{
	_tag_inside = value;
}

geometry::rectangle& Curve::bounds()
{
	if (size() > 0 && _bounds.empty())
	{
		if ((*this)[0].type == SegmentType::Circle )
		{
			Segment s = (*this)[0];
			s.bounds.top_left = glm::vec2(s.center.x - s.radius, s.center.y + s.radius);
			s.bounds.bottom_right = glm::vec2(s.center.x + s.radius, s.center.y - s.radius);

			_bounds.top_left = s.bounds.top_left;
			_bounds.bottom_right = s.bounds.bottom_right;
		}
		else
		{
			_bounds.top_left.x = std::numeric_limits<float>::max();
			_bounds.top_left.y = -std::numeric_limits<float>::max();
			_bounds.bottom_right.x = -std::numeric_limits<float>::max();
			_bounds.bottom_right.y = std::numeric_limits<float>::max();


			for (int i = 0; i < size(); i++)
			{
				Segment& s1 = (*this)[i];
				Segment& s2 = (i == size() - 1) ? (*this)[0] : (*this)[i + 1];

				if (s1.type == SegmentType::Line)
				{
					s1.bounds.top_left.x = glm::min(s1.point.x, s2.point.x);
					s1.bounds.top_left.y = glm::max(s1.point.y, s2.point.y);
					s1.bounds.bottom_right.x = glm::max(s1.point.x, s2.point.x);
					s1.bounds.bottom_right.y = glm::min(s1.point.y, s2.point.y);
				}
				else // SegmentType::Arc
				{
					if (s1.point == s2.point) // same as circle
					{
						s1.bounds.top_left = glm::vec2(s1.center.x - s1.radius, s1.center.y + s1.radius);
						s1.bounds.bottom_right = glm::vec2(s1.center.x + s1.radius, s1.center.y - s1.radius);

						_bounds.top_left = s1.bounds.top_left;
						_bounds.bottom_right = s1.bounds.bottom_right;
					}
					else
					{
						auto r = geometry::arc_bounds(s1.point, s1.center, s2.point, s1.radius, s1.cw);
						s1.bounds.top_left = r.top_left;
						s1.bounds.bottom_right = r.bottom_right;
					}
				}

				_bounds.top_left.x = glm::min(s1.bounds.top_left.x, _bounds.top_left.x);
				_bounds.top_left.y = glm::max(s1.bounds.top_left.y, _bounds.top_left.y);
				_bounds.bottom_right.x = glm::max(s1.bounds.bottom_right.x, _bounds.bottom_right.x);
				_bounds.bottom_right.y = glm::min(s1.bounds.bottom_right.y, _bounds.bottom_right.y);
			}
		}
	}

	return _bounds;
}

void Curve::reset_bounds()
{
	int index = 0;
	for (Segment& p : *this)
		p.index = index++;

	_bounds.top_left = _bounds.bottom_right = geometry::vec2_empty;

	bounds();

	_length = -1;

	_tree.reset();

	_cw = -1;
}

void Curve::add(glm::vec2 point)
{
	push_back(Segment(point));
	back().index = (int)size() - 1;
	_length = _area = -1.0f;
	_cw = -1;
}

void Curve::add(glm::vec2 point, glm::vec2 center, float radius, bool cw)
{
	push_back(Segment(point, center, radius, cw, SegmentType::Arc));
	back().index = (int)size() - 1;
	_length = _area = -1.0f;
	_cw = -1;
}

void Curve::add(glm::vec2 center, float radius, bool cw)
{
	glm::vec2 point = geometry::position(0, radius) + center;
	push_back(Segment(point, center, radius, cw, SegmentType::Circle));
	back().index = (int)size() - 1;
	push_back(Segment(point));
	back().index = (int)size() - 1;
	_length = _area = -1.0f;
	_cw = -1;
}

void Curve::add(SegmentType type, glm::vec2 point, glm::vec2 center, float radius, bool cw)
{
	push_back(Segment(point, center, radius, cw, type));
	back().index = (int)size() - 1;
	_length = _area = -1.0f;
	_cw = -1;
}

void Curve::add(Segment& s)
{
	add(s.type, s.point, s.center, s.radius, s.cw);
}

glm::vec2 Curve::first()
{
	if (size() > 0)
		return (*this)[0].point;
	return glm::vec2();
}

glm::vec2 Curve::last()
{
	if (size() > 1)
		return (*this).back().point;
	return glm::vec2();
}

void Curve::reverse()
{
	if (_cw == -1)
		cw();

	if (size() > 1)
	{
		if (size() == 2)
		{
			if ((*this)[0].type == SegmentType::Circle || (*this)[0].type == SegmentType::Arc && (*this)[0].point == (*this)[1].point)
			{
				(*this)[0].cw = !(*this)[0].cw;
			}
			else
			{
				Curve temp;
				temp.add((*this)[0].type, (*this)[1].point, (*this)[0].center, (*this)[0].radius, !(*this)[0].cw);
				temp.add((*this)[1].type, (*this)[0].point, (*this)[1].center, (*this)[1].radius, !(*this)[1].cw);
				this->clear();
				this->insert(begin(), temp.begin(), temp.end());
			}
			_cw = (*this)[0].cw;
			return;
		}

		Segment s = back();
		std::reverse(begin(), end());

		for (int i = 0; i + 1 < size(); i++)
		{
			(*this)[i].type = (*this)[i + 1].type;
			(*this)[i].center = (*this)[i + 1].center;
			(*this)[i].radius = (*this)[i + 1].radius;
			(*this)[i].cw = !(*this)[i + 1].cw;
#ifdef _DEBUG
			(*this)[i].perimeter = (*this)[i + 1].radius * glm::two_pi<float>();
#endif
		}
		(*this)[size() - 1].type = s.type;
		(*this)[size() - 1].center = s.center;
		(*this)[size() - 1].radius = s.radius;
		(*this)[size() - 1].cw = !s.cw;


		auto direction = _cw;

		reset_bounds();

		_cw = direction ? 0 : 1;
	}
}

bool Curve::closed()
{
	if (size() == 2)
	{
		if ((*this)[0].type == SegmentType::Circle)
			return true;
		if ((*this)[0].type == SegmentType::Arc && first() == last())
			return true;
	}

	if (size() > 2)
		return first() == last();

	return false;
}

bool Curve::cw()
{
	if (_cw == -1)
	{
		if (size() == 2) {
			if ((*this)[0].type == SegmentType::Line)
			{
				auto angle = geometry::angle((*this)[1].point, (*this)[0].point);
				_cw = angle < glm::pi<float>() && angle >= 0;
			}
			else
				_cw = (*this)[0].cw;
		}
		else
		{
			if (size() == 0)
				_cw = true;
			else
				// we compute the signed area, the arcs are taken into account : the points alone are not enough for a curve which
				// has only a few arcs (a circle made of two arcs has no area when only the points are used)
				_cw = limited_area() < 0;
		}
	}

	return _cw;
}

void Curve::cw(bool direction)
{
	if (cw() != direction)
		reverse();
}

float Curve::length()
{
	if (_length != -1)
		return _length;

	std::vector<Segment>::iterator from = begin();
	std::vector<Segment>::iterator it = begin() + 1;
	float angle = 0;

	_length = 0;

	while (it != end())
	{
		switch ((*from).type)
		{
		case SegmentType::Line:
			(*from).length = geometry::distance((*it).point, (*from).point);
			_length += (*from).length; // geometry::distance((*it).ref_point, from.ref_point);
			break;
		case SegmentType::Circle:
			(*from).length = _length = (*from).radius * glm::two_pi<float>();
			break;
		case SegmentType::Arc:
			angle = geometry::oriented_angle((*from).point, (*it).point, (*from).center, (*from).cw);
			geometry::distance((*it).point, (*from).point);
			(*from).length = (*from).radius * angle;
			_length += (*from).length; // from.radius* angle;
			break;
		}
		from = it;
		it++;
	}

	if (size() > 1)
	{
		from = end() - 1;
		it = begin();

		switch ((*from).type)
		{
		case SegmentType::Line:
			(*from).length = geometry::distance((*it).point, (*from).point);
			_length += (*from).length; // geometry::distance((*it).ref_point, from.ref_point);
			break;
		case SegmentType::Arc:
			angle = geometry::oriented_angle((*from).point, (*it).point, (*from).center, (*from).cw);
			geometry::distance((*it).point, (*from).point);
			(*from).length = (*from).radius * angle;
			_length += (*from).length; // from.radius* angle;
			break;
		}
	}

	return _length;
}

float Curve::area()
{
	if (_area == -1) {

		if (empty())
			return _area = 0;

		if (size() == 1 && (*this)[0].type == SegmentType::Circle)
		{
			float full = glm::pi<float>() * (*this)[0].radius * (*this)[0].radius;
			return _area = (*this)[0].cw ? -full : full;
		}

		// Exact signed area (positive counter-clockwise): shoelace on the chords,
		// plus the circular segment between each arc and its chord.
		double sum = 0, extra = 0;
		// an open curve is implicitly closed back to its first point
		size_t count = closed() ? size() - 1 : size();
		for (size_t i = 0; i < count; i++)
		{
			const Segment& from = (*this)[i];
			const glm::vec2 p = from.point, q = (i + 1 < size()) ? (*this)[i + 1].point : front().point;
			sum += (double)p.x * q.y - (double)q.x * p.y;

			if (from.type == SegmentType::Line)
				continue;

			double r = from.radius;
			double theta;
			if (from.type == SegmentType::Circle || glm::distance(p, q) < geometry::ERR_FLOAT4)
				theta = glm::two_pi<double>();
			else
			{
				double ap = std::atan2((double)p.y - from.center.y, (double)p.x - from.center.x);
				double aq = std::atan2((double)q.y - from.center.y, (double)q.x - from.center.x);
				theta = from.cw ? ap - aq : aq - ap;
				theta = std::fmod(theta, glm::two_pi<double>());
				if (theta < 0) theta += glm::two_pi<double>();
			}
			double seg = 0.5 * r * r * (theta - std::sin(theta));
			extra += from.cw ? -seg : seg;
		}

		_area = (float)(sum / 2 + extra);
	}
	return _area;
}


float Curve::limited_area()
{
	std::vector<glm::vec2> points;

	if (size() == 1 && (*this)[0].type == SegmentType::Circle)
		return _area = (*this)[0].radius * glm::two_pi<float>();

	Segment from = front();
	auto it = begin() + 1;

	points.push_back(from.point);

	while (it != end())
	{
		points.push_back((*it).point);
		from = (*it);
		it++;
	}

	if (front().point != back().point)
		points.push_back(front().point);

	float area = 0;
	glm::vec2 f = points.front();
	auto pit = points.begin() + 1;
	while (pit != points.end())
	{
		area += (f.x * (*pit).y) - ((*pit).x * f.y);
		f = (*pit);
		pit++;
	}
	return area;
}

bool Curve::inside(glm::vec2 p)
{
	if (size() == 2)
	{
		if ((*this)[0].type == SegmentType::Circle || (*this)[0].point == (*this)[1].point)
		{
			return geometry::distance((*this)[0].center, p) <= (*this)[0].radius;
		}
		else if ((*this)[0].type == SegmentType::Arc)
		{
			// should test following the bulge
			return geometry::distance((*this)[0].center, p) <= (*this)[0].radius;
		}
	}

	std::vector<glm::vec2> points;
	auto it = begin();
	glm::vec2 dst = (*it).point;

	while (it != end())
	{
		if ((*it).type == SegmentType::Line)
		{
			points.push_back((*it).point);
		}
		else
		{
			auto dst = (it == end() - 1) ? front().point : (*(it + 1)).point;
			auto arc = geometry::arc((*it).point, (*it).center, dst, (*it).cw);
			points.insert(points.end(), arc.begin(), arc.end());
		}
		it++;
	}
	
	bool inside = false;

	//int e = 0; // points.front() == points.back() ? 1 : 0;
	//auto start_it = begin();
	//auto end_it = end() - 1;

	//while (start_it != end() - e)
	//{
	//	if ((((*start_it).ref_point.y > p.y) != ((*end_it).ref_point.y > p.y)) &&
	//		(p.x < ((*end_it).ref_point.x - (*start_it).ref_point.x) * (p.y - (*start_it).ref_point.y) / ((*end_it).ref_point.y - (*start_it).ref_point.y) + (*start_it).ref_point.x))
	//		inside = !inside;
	//	end_it = start_it;
	//	start_it++;
	//}

	int count = (int)(points.front() == points.back() ? points.size() - 1 : points.size());
	glm::vec2 p1 = points.front(), p2;

	for (int i = 1; i <= count; i++) {
		// Get the next ref_point in the polygon
		p2 = points[i % count];

		// Check if the ref_point is above the minimum y
		// coordinate of the edge
		if (p.y > glm::min(p1.y, p2.y)) {
			// Check if the ref_point is below the maximum y
			// coordinate of the edge
			if (p.y <= glm::max(p1.y, p2.y)) {
				// Check if the ref_point is to the left of the
				// maximum x coordinate of the edge
				if (p.x <= glm::max(p1.x, p2.x)) {
					// Calculate the x-intersection of the
					// line connecting the ref_point to the edge
					double x_intersection
						= (p.y - p1.y) * (p2.x - p1.x)
						/ (p2.y - p1.y)
						+ p1.x;

					// Check if the ref_point is on the same
					// line as the edge or to the left of
					// the x-intersection
					if (p1.x == p2.x
						|| p.x <= x_intersection) {
						// Flip the inside flag
						inside = !inside;
					}
				}
			}
		}

		// Store the current ref_point as the first ref_point for
		// the next iteration
		p1 = p2;
	}

	return inside;
}

bool Curve::inside(Curve& b)
{
	if (bounds().outside(b.bounds()))
		return false;

	// we check for first ref_point, if it is inside, then we look for an intersection
	if (b.inside((*this)[0].point))
	{
		// we test smaller against bigger
		Curve& a = *this, c = b;

		auto i1 = a.begin();
		while (i1 != a.end())
		{
			auto s1 = *i1;
			auto dst1 = (i1 == a.end() - 1) ? a.front().point : (*(i1+1)).point;
			auto candidates = c.search(s1.bounds);

			auto i2 = candidates.begin();
			while (i2 != candidates.end())
			{
				auto s2 = *i2;
				auto dst2 = ((*i2).index == c.size() - 1) ? c.front().point : c[(*i2).index+1].point;

				glm::vec2 result[2];
				if (s1.intersect(dst1, s2, dst2, result) > 0)
					return false;
				i2 = i2 + 1;
			}

			i1 = i1 + 1;
		}

		return true;
	}

	return false;
}

bool Curve::outside(Curve& b)
{
	if (bounds().outside(b.bounds()))
		return true;

	// we check for first ref_point, if it is inside, then we look for an intersection
	if (!b.inside((*this)[0].point))
	{
		// we test smaller against bigger
		Curve& a = *this, c = b;

		auto i1 = a.begin();
		while (i1 != a.end())
		{
			auto s1 = *i1;
			auto dst1 = (i1 == a.end() - 1) ? a.front().point : (*(i1 + 1)).point;
			auto candidates = c.search(s1.bounds);

			auto i2 = candidates.begin();
			while (i2 != candidates.end())
			{
				auto s2 = *i2;
				auto dst2 = ((*i2).index == c.size() - 1) ? c.front().point : c[(*i2).index + 1].point;

				glm::vec2 result[2];
				if (s1.intersect(dst1, s2, dst2, result) > 0)
					return false;
				i2 = i2 + 1;
			}

			i1 = i1 + 1;
		}

		return true;
	}

	return false;
}

Position Curve::relative(Curve& b)
{
	if (bounds().outside(b.bounds()))
		return Position::outside;

	auto c1 = *this;
	auto c2 = b;

	// we check for first ref_point, if it is inside, then we look for an intersection
	auto i1 = c1.begin();
	while (i1 != c1.end())
	{
		auto s1 = *i1;
		auto dst1 = (i1 == c1.end() - 1) ? c1.front().point : (*(i1 + 1)).point;
		auto candidates = c2.search(s1.bounds);

		auto i2 = candidates.begin();
		while (i2 != candidates.end())
		{
			auto s2 = *i2;
			auto dst2 = ((*i2).index == c2.size() - 1) ? c2.front().point : c2[(*i2).index + 1].point;

			glm::vec2 result[2];
			if (s1.intersect(dst1, s2, dst2, result) > 0)
				return Position::intersect;
			i2 = i2 + 1;
		}

		i1 = i1 + 1;
	}

	return Position::inside;
}

bool Curve::intersect(Curve& b)
{
	if (bounds().outside(b.bounds()))
		return false;

	// we check for first ref_point, if it is inside, then we look for an intersection
	auto i1 = begin();
	while (i1 != end())
	{
		auto s1 = *i1;
		auto dst1 = (i1 == end() - 1) ? front().point : (*(i1 + 1)).point;
		auto candidates = b.search(s1.bounds);

		auto i2 = candidates.begin();
		while (i2 != candidates.end())
		{
			auto s2 = *i2;
			auto dst2 = ((*i2).index == b.size() - 1) ? b.front().point : b[(*i2).index + 1].point;

			glm::vec2 result[2];
			if (s1.intersect(dst1, s2, dst2, result) > 0)
				return true;
			i2 = i2 + 1;
		}

		i1 = i1 + 1;
	}

	return false;
}

void Curve::rotate(int index)
{
	if (index > 0 && index < size())
	{
		Curve temp;
		temp.reserve(size());
		temp.insert(temp.end(), begin() + index, end());
		temp.insert(temp.end(), begin(), begin() + index - 1);
		clear();
		insert(end(), temp.begin(), temp.end());
	}
}

std::vector<glm::vec2> Curve::coordinates()
{
	if (size() == 2 && (*this)[0].type == SegmentType::Circle)
	{
		return geometry::circle((*this)[0].radius, (*this)[0].center);
	}

	std::vector<glm::vec2> points;
	Segment from = front();
	auto it = begin() + 1;

	while (it != end())
	{
		if (from.type == SegmentType::Line)
		{
			points.push_back(from.point);
		}
		else
		{
			auto arc = geometry::arc(from.point, from.center, (*it).point, from.cw, 0.25f);
			points.insert(points.end(), arc.begin(), arc.end() - 1);
		}
		from = (*it);
		it++;
	}
	points.push_back(from.point);

	return points;
}

std::vector<Curve> Curve::cut(glm::vec2 point, int index)
{
	return cut(point, begin() + index);
}

std::vector<Curve> Curve::cut(glm::vec2 point, std::vector<Segment>::iterator it)
{
	std::vector<Curve> result;

	if (it < end())
	{
		Curve left, right;

		if (it != begin())
			left.insert(left.begin(), begin(), it - 1);

		if ((*it).type == SegmentType::Line)
		{
			left.push_back(*it); // we add *it segment start ref_point
			left.push_back(Segment(point));	// we add new end ref_point

			right.push_back(Segment(point));
			right.insert(right.end(), it + 1, end());
		}
		else if ((*it).type == SegmentType::Arc)
		{
			left.push_back(*it);
			left.push_back(Segment(point));
			right.push_back(Segment(point, (*it).center, (*it).radius, (*it).cw));
			right.insert(right.end(), it + 1, end());
		}
		else // circle
		{
			left.push_back(Segment(point, (*it).center, (*it).radius, (*it).cw));
			left.push_back(Segment(point));
		}

		result.push_back(left);
		if (right.size() > 0)
			result.push_back(right);
	}
	else
		result.push_back(*this);

	return result;
}

std::vector<Curve> Curve::merge(Curve& b)
{
	std::vector<Curve> result;
	
	Curve& a = *this;

	if (a.last() == b.last())
		b.reverse();
	else if (a.first() == b.first())
		a.reverse();

	if (a.last() == b.first())
	{
		Curve c;
		c.insert(c.end(), a.begin(), a.end() - 1);
		c.insert(c.end(), b.begin(), b.end());

		result.push_back(c);
	}
	else if (b.last() == a.first())
	{
		Curve c;
		c.insert(c.end(), b.begin(), b.end() - 1);
		c.insert(c.end(), a.begin(), a.end());

		result.push_back(c);
	}
	else // using XOR algo
	{

	}

	return result;
}

void Curve::scale(float factor)
{
	for (Segment& s : *this)
	{
		s.point *= factor;
		s.center *= factor;
		s.radius *= factor;
	}
	reset_bounds();
}

// Offseting is creating a contour of the curve with the same distance all along the curve.
// It can be inner or outer
// If offset value 'o' is positive, then it is outer. If it is negative, then it is inner.
// 
// A curve can be composed of segments and arcs. For clarity, we only use segment term.
// 
// Step to generate the offset curve :
//  1 - untrim the curve. this is creating new segments away of offset value from original segments. Segments from untrim are not connected. 
//  2 - trim the previous result. Trimming is the process that calculate how to connect untrimmed segments.
//  3 - split the trimmed curve at intersection points. Indeed the trimmed result can create curve with self intersections.
//      we need to split the trimmed curve and output a new curve following the intersection points
//  4 - we have to check and refuse curves that are tout small (size of offset) or curves to close to the original curve
/// <summary>
/// When an arc of the original curve collapses during an offset, the raw offset curve folds back on itself along a line, which makes a
/// spike : [p0, p1] followed by [p1, p2] with p2 on the line (p0, p1). The result is the same as the single segment [p0, p2], but the
/// spike produces several intersections that are closer to each other than the precision of the coordinates, and the curve is then
/// split the wrong way. The spikes are replaced by the single segment.
/// </summary>
void Curve::remove_spikes()
{
	bool closed_curve = size() > 2 && front().point == back().point;
	if (closed_curve)
		pop_back();

	bool changed = true;
	bool modified = false;
	while (changed && size() > 3)
	{
		changed = false;
		for (size_t i = 0; i < size(); i++)
		{
			size_t k = (i + 1) % size(), l = (i + 2) % size();
			Segment& s1 = (*this)[i];
			Segment& s2 = (*this)[k];

			if (s1.type != SegmentType::Line || s2.type != SegmentType::Line)
				continue;

			glm::vec2 a = s2.point - s1.point, b = (*this)[l].point - s2.point;
			float la = glm::length(a), lb = glm::length(b);
			if (la == 0 || lb == 0)
				continue;

			// the second segment goes back along the first one, within less than a degree
			if (glm::dot(a, b) / (la * lb) < -0.9999f)
			{
				s1.excluded = s1.excluded || s2.excluded;
				erase(begin() + k);
				changed = modified = true;
				break;
			}
		}
	}

	if (closed_curve)
		push_back(Segment(front().point));

	if (modified)
	{
		_area = -1;
		_cw = -1;
		reset_bounds();
	}
}

/// <summary>
/// A tiny arc whose end lies a few microns before its start, along the arc direction, is a whole circle for the curve : the intersection
/// points at both ends are not in the right order. The arc is replaced by a straight connection.
/// </summary>
static void connect_reversed_arcs(Curve& c)
{
	bool modified = false;
	std::vector<bool> remove(c.size(), false);
	for (size_t i = 0; i < c.size(); i++)
	{
		Segment& s = c[i];
		if (s.type != SegmentType::Arc)
			continue;

		glm::vec2 dst = i + 1 == c.size() ? c[0].point : c[i + 1].point;
		if (glm::distance(s.point, dst) > 0.05f)
			continue;

		double a0 = std::atan2((double)s.point.y - s.center.y, (double)s.point.x - s.center.x);
		double a1 = std::atan2((double)dst.y - s.center.y, (double)dst.x - s.center.x);
		double sweep = s.cw ? a0 - a1 : a1 - a0;
		while (sweep < 0)
			sweep += glm::two_pi<double>();

		if (sweep > glm::pi<double>())
		{
			remove[i] = true;
			modified = true;
		}
	}

	if (modified)
	{
		// what is left of a sliver is not a curve anymore
		if (c.size() <= 3)
		{
			c.clear();
			return;
		}

		for (size_t i = 0; i < c.size(); i++)
			if (remove[i])
			{
				c[i].type = SegmentType::Line;
				c[i].center = glm::vec2();
				c[i].radius = 0;
			}
		c.reset_bounds();
	}
}

std::vector<Curve> Curve::offset(float o, float max)
{
	bool outside = o > 0;
	std::vector<Curve> result;

	//float factor = glm::abs(o) < 1 ? 1 / glm::abs(o) : 1;
	//if (factor != 1)
	//{
	//	scale(factor);
	//	o *= factor;
	//}

	bounds();

	if (!outside && (bounds().width() < glm::abs(o) || bounds().height() < glm::abs(o)))
		return result;

	auto segments = untrim(o);

	auto trimmed = trim(segments, o);

	trimmed.remove_spikes();

	// the direction of the original curve tells which side of the raw offset curve is the valid one
	auto splitted = trimmed.split_at_intersections(cw() ? -1 : 1);
	for (Curve& piece : splitted)
		connect_reversed_arcs(piece);
	splitted.erase(std::remove_if(splitted.begin(), splitted.end(), [](Curve& piece) { return piece.size() == 0; }), splitted.end());

	if (false)
	{
		result.push_back(trimmed);
	}

	if (false)
	{
		for (Curve c : splitted)
			result.push_back(c);
	}

	bool cross_found = false, cw_found = false;
	for (Curve& c : splitted)
	{
		c.close();
		c.reset_bounds();
	}
	for (Curve c : splitted)
	{
		if (curve_intersect(c))
		{
			cross_found = true;
			// the pieces to keep have the direction of the original curve, the loops created by the trim have the opposite one
			cw_found = cw();
		}
	}

	for (Curve& c : splitted)
	{
		// if outside offset or inside and offset is smaller than current
		// an inner offset keeps the direction of the curve : a loop which turns the other way is a tiny inverted piece left by the trim,
		// which would grow outwards when it is offset again
		if ((outside || c.cw() == cw()) && (cross_found && c.cw() == cw_found || !cross_found) && (o > 0 || (!outside && c.bounds().width() * c.bounds().height() < _bounds.width() * _bounds.height())))
		{
			if (!c.too_small(o) && !too_close(c, o))
			{
				c.level(_level);
				c.tag_inside(outside ? Position::outside : Position::inside);
				if (max > 0)
					c.reduce(max);
				result.push_back(c);
			}
		}
	}

	//if (factor != 1)
	//	for(Curve& c : result)
	//		c.scale(1 / factor);

	return result;
}

void Curve::middle(glm::vec2& position, float& angle)
{
	if (this->size() > 1)
	{
		if ((*this)[0].type == SegmentType::Circle)
		{
			position = (*this)[0].center + glm::vec2(-(*this)[0].radius, 0);
			angle = geometry::oriented_angle((*this)[0].cw ? glm::half_pi<float>() : -glm::half_pi<float>());
		}
		else
		{
			float pos = length() / 2, len = 0;

			auto from = (*this).begin();
			auto to = from + 1;
			// we loop to look for our middle length
			while (to != (*this).end() && len + (*from).length < pos)
			{
				len += (*from).length;
				from = to;
				to = to + 1;
			}

			// if middle length is inside a segment
			if (len < pos)
			{
				auto rest = pos - len;
				if ((*from).type == SegmentType::Line)
				{
					position = geometry::position((*to).point, rest, (*from).point);
					angle = geometry::oriented_angle((*to).point, (*from).point);
				}
				else // arc
				{
					float a_r = rest / (*from).radius;
					float a_s = geometry::oriented_angle((*from).point, (*from).center);
					float a_d = geometry::oriented_angle((*from).cw ? a_s - a_r : a_s + a_r);

					angle = geometry::oriented_angle((*from).cw ? a_d - glm::half_pi<float>(): a_d + glm::half_pi<float>());
					position.x = glm::cos(a_d) * (*from).radius + (*from).center.x;
					position.y = glm::sin(a_d) * (*from).radius + (*from).center.y;
				}
			}
		}
	}
}

std::vector<Curve> Curve::split(float length)
{
	return std::vector<Curve>();
}

void Curve::reduce(float max)
{
	length();

	auto it = begin();
	auto from = it;
	auto to = it;
	float len = (*from).length;

	while (it != end())
	{
		if ((*it).length < max)
		{
			if (it + 1 != end())
			{
				if ((*it).type == SegmentType::Line && (*(it + 1)).type == SegmentType::Line)
				{
					erase(it + 1);
					if (it != end() - 1)
						(*it).length = geometry::distance((*it).point, (*(it + 1)).point);
					else
						(*it).length = geometry::distance((*it).point, front().point);
				}
				else
					it++;
			}
			else
			{
				/*if ((*it).type == SegmentType::Line && front().type == SegmentType::Line)
				{
					erase(begin());
					(*it).length = geometry::distance((*it).ref_point, (*begin()).ref_point);
				}
				else */
					it++;
			}
		} else
			it++;
	}

	//while (it != end())
	//{
	//	if (len < max)
	//	{
	//		if (len + (*it).length >= max)
	//		{
	//			len = (*it).length;
	//			it = erase(from, to) + 1;
	//			from = to = it + 1;
	//		}
	//		else
	//		{
	//			to = it;
	//			len += (*it).length;
	//		}
	//	}
	//	else
	//		len = (*it).length;
	//	it++;
	//}

	reset_bounds();
}

// untrimming process
//  - if segment, translate segment with offset distance along orthogonal 
//  - if arc or circle, reduce or increase the radius
// the process depends of curve direction, clockwise or counter-clockwise

std::vector<SegmentUntrim> Curve::untrim(float o)
{
	length();

	std::vector<SegmentUntrim> result;
	float offset = glm::abs(o);
	bool inside = o < 0;
	bool cw = this->cw();

	if (size() > 0)
	{
		if ((*this)[0].type == SegmentType::Circle)
		{
			if ((*this)[0].radius + o > 0)
			{
				result.push_back(
					SegmentUntrim((*this)[0].point, (*this)[0].center, (*this)[1].point, (*this)[0].radius + o, (*this)[0].cw, 0, SegmentType::Circle)
				);
			}
		}
		else
		{
			auto from = begin();
			auto it = from + 1;
			int index = 0;
			bool exit = false;
			while (!exit)
			{
				if (it == end())
				{
					it = begin();
					exit = true;
				}

				auto s = (*from);
				auto dst = (*it).point;

				if (s.point != dst)
				{

					if (s.type == SegmentType::Line)
					{
						auto angle = geometry::oriented_angle(dst, s.point);
						if (cw)
						{
							if (inside)
								angle = angle - glm::half_pi<float>();
							else
								angle = angle + glm::half_pi<float>();
						}
						else
						{
							if (inside)
								angle = angle + glm::half_pi<float>();
							else
								angle = angle - glm::half_pi<float>();
						}
						auto start = geometry::position(angle, offset, s.point);
						auto stop = geometry::position(angle, offset, dst);
						
						//auto len = geometry::distance(start, stop);
						//auto off = geometry::distance(start, stop) / offset;
						//start = geometry::position(start, len-off, stop);
						//stop = geometry::position(stop, len-2*off, start);

						result.push_back(
							SegmentUntrim(start, stop, index)
						);
					}
					else // SegmentType::Arc
					{
						auto start_a = geometry::oriented_angle(s.point, s.center);
						auto stop_a = geometry::oriented_angle(dst, s.center);
						auto radius = s.radius;

						if (cw)
						{
							if (inside)
							{
								if (s.cw)
									radius = radius - offset;
								else
									radius = radius + offset;
							}
							else
							{
								if (s.cw)
									radius = radius + offset;
								else
									radius = radius - offset;
							}
						}
						else
						{
							if (inside)
							{
								if (s.cw)
									radius = radius + offset;
								else
									radius = radius - offset;
							}
							else
							{
								if (s.cw)
									radius = radius - offset;
								else
									radius = radius + offset;
							}
						}

						auto start = geometry::position(start_a, radius, s.center);
						auto stop = geometry::position(stop_a, radius, s.center);
						
						//if (radius > s.radius /*&& s.length <= s.radius * glm::pi<float>()*/)
						//{
						//	auto middle = geometry::arc_middle(start, s.center, stop, s.cw);
						//	auto a_angle = geometry::oriented_angle(middle, s.center);
						//	glm::vec2 pstart[2], pstop[2];
						//	geometry::line_arc_intersect(s.ref_point, geometry::position(a_angle, 10, s.ref_point), start, s.center, stop, s.cw, pstart);
						//	geometry::line_arc_intersect(dst, geometry::position(a_angle, 10, dst), start, s.center, stop, s.cw, pstop);
						//	start = pstart[0];
						//	stop = pstop[0];
						//}


						// if radius computed is negative, then we are sure that we will get a self intersection ref_point
						// so we add a segment instead of an arc, and we tag it as excluded, which will be used later
						// to revoke a curve when we will split it at self intersection ref_point, see trim() and split_at_intersections()
						// an arc that collapses to a point (radius below the tolerance, which is also below what a float
						// can represent at usual coordinates) becomes a sharp corner, the neighbour segments are linked by trim()
						if (glm::abs(radius) <= geometry::ERR_FLOAT4)
						{
						}
						else if (radius > 0) 
						{

							result.push_back(
								SegmentUntrim(start, s.center, stop, radius, s.cw, index)
							);
						}
						else
						{
							result.push_back(
								SegmentUntrim(start, stop, index, true)
							);
						}
					}
				}

				from = it;
				it++;
				index++;
			}
		}
	}

	return result;
}

Curve Curve::trim(std::vector<SegmentUntrim>& original, float o)
{
	Curve result;

	if (original.size() == 0)
		return result;

	bool inside = o < 0;
	bool clock = inside ? !cw() : cw();
	float v = glm::abs(o);

	if (original.size() == 1 && original[0].type == SegmentType::Circle)
	{
		result.add(original[0].center, original[0].radius, original[0].cw);
	}
	else
	{
		bool exit = false;
		SegmentUntrim s1 = original.front();
		SegmentUntrim s2 = original.front();

		auto it = original.begin() + 1;
		int cc = 0;
		while (!exit)
		{
			// we finish the closed curve
			if (it == original.end())
			{
				s2 = original.front();
				exit = true;
			}
			else
				s2 = (*it);

			//Logger::log(std::to_string(geometry::distance(s1.dst, s2.ref_point)));

			if (s1.dst == s2.point) // if s1 and s2 are already connected, we add s1
			{
				result.add(s2.type, s2.point, s2.center, s2.radius, s2.cw);
			}
			else
			{
				glm::vec2 pp[2], p = glm::vec2();
				bool left1 = false, left2 = false, tip1 = false, tip2 = false;
				int count = 0;

				if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
				{
					bool inter = geometry::line_line_intersect(s1.point, s1.dst, s2.point, s2.dst, p) ? 1 : 0;
					if (inter)
					{
						tip1 = geometry::colinear_segment_point(s1.point, s1.dst, p, geometry::ERR_FLOAT3);
						tip2 = geometry::colinear_segment_point(s2.point, s2.dst, p, geometry::ERR_FLOAT3);
						count = tip2 && tip1 ? 1 : 0;

						//if (count == 0 && tip1)
						//{
						//	// if p is a positive intersection ref_point, s2 needs to be discarded
						//	auto a1 = geometry::oriented_angle(s2.dst, s2.ref_point);
						//	auto a2 = geometry::oriented_angle(p, s2.ref_point);
						//	auto diff = geometry::oriented_angle(a1 - a2);
						//	if (diff < 1)
						//		s2.excluded = true;
						//}
					}
					pp[0] = p;
				}
				else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
				{
					count = geometry::segment_arc_intersect(s1.point, s1.dst, s2.point, s2.center, s2.dst, s2.radius, s2.cw, pp);
				}
				else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
				{
					count = geometry::segment_arc_intersect(s2.point, s2.dst, s1.point, s1.center, s1.dst, s1.radius, s1.cw, pp);
				}
				else // arc - arc
				{
					count = geometry::arc_arc_intersect(s1.point, s1.center, s1.dst, s1.radius, s1.cw, s2.point, s2.center, s2.dst, s2.radius, s2.cw, pp);
				}

				if (count == 1)
				{
					result.add(s2.type, pp[0], s2.center, s2.radius, s2.cw);
				}
				else if (count == 0)
				{
					p = s2.point;
					if (geometry::distance2(s1.dst, s2.point) > geometry::ERR_FLOAT3)
					{
						float a = geometry::oriented_angle(s1.dst, s2.point, (*this)[s2.index].point, clock);
						// if segment is tagged as excluded, we forward the tag 
						// if the linked arc angle is greater than PI, this means that there is already an intersection
						// earlier and this arc link will intersect original curve, so it needs to be excluded
						if (s1.excluded || s2.excluded || a > glm::pi<float>())
						{
							result.add(s1.dst);
							(&result.back())->excluded = true;
						}
						else
							result.add(SegmentType::Arc, s1.dst, (*this)[s2.index].point, v, clock);
					}
					result.add(s2.type, p, s2.center, s2.radius, s2.cw);
				}
				else
				{
					bool p1b1, p1b2, p2b1, p2b2;

					if (s1.type == SegmentType::Line)
					{
						p1b1 = geometry::colinear_segment_point(s1.point, s1.dst, pp[0]);
						p1b2 = geometry::colinear_segment_point(s1.point, s1.dst, pp[1]);
					}
					else
					{
						p1b1 = geometry::colinear_arc_point(s1.point, s1.center, s1.dst, s1.cw, pp[0]);
						p1b2 = geometry::colinear_arc_point(s1.point, s1.center, s1.dst, s1.cw, pp[1]);
					}


					if (s2.type == SegmentType::Line)
					{
						p2b1 = geometry::colinear_segment_point(s2.point, s2.dst, pp[0]);
						p2b2 = geometry::colinear_segment_point(s2.point, s2.dst, pp[1]);
					}
					else
					{
						p2b1 = geometry::colinear_arc_point(s2.point, s2.center, s2.dst, s2.cw, pp[0]);
						p2b2 = geometry::colinear_arc_point(s2.point, s2.center, s2.dst, s2.cw, pp[1]);
					}

					if (p1b1 && p2b1)
						result.add(s2.type, pp[0], s2.center, s2.radius, s2.cw);
					else if (p1b2 && p2b2)
						result.add(s2.type, pp[1], s2.center, s2.radius, s2.cw);
					else
					{
						if (geometry::distance2(s1.dst, s2.point) > geometry::ERR_FLOAT3)
						{
							//float t = geometry::arc_thickness(s1.dst, (*this)[s2.index].ref_point, s2.ref_point, clock);
							//if (t > geometry::ERR_FLOAT3)
							{
								auto a = geometry::oriented_angle(s1.dst, s2.point, (*this)[s2.index].point, clock);
								result.add(SegmentType::Arc, s1.dst, (*this)[s2.index].point, v, clock);
								// if segment is tagged as excluded, we forward the tag 
								// if the linked arc angle is greater than PI, this means that there is already an intersection
								// earlier and this arc link will intersect original curve, so it needs to be excluded
								if (s1.excluded || s2.excluded || a > glm::pi<float>())
									(&result.back())->excluded = true;
							}
						}
						result.add(s2.type, s2.point, s2.center, s2.radius, s2.cw);
					}
				}
			}

			s1 = s2;
			if (!exit)
				it++;
			cc++;
		}
	}

	result.length();
	return result;
}

/// <summary>
/// Intersection strucure hold information of an intersection ref_point
/// </summary>
struct Intersection
{
	int index1 = -1;		// index of first origin segment that intersects
	int index2 = -1;		// index of second origin segment that intersects
	glm::vec2 point;		// coordinates of intersection
	int tag = -1;			// the pair of Intersection will have the same tag
	int index = -1;			// index in the intersection list of the index1 segment
	float a = 0;			// oriented angle for arcs sorting :
							//		when one segment has multiple intersections, intersections needs to be sorted
							//		if for a segment, we can sort intersections based on x or y coordinates
							//		for arcs, we need to sort intersections based on the arc angle from start ref_point to intersection  ref_point


										// iterator reference allows to pick segments in the list for fast processing
	std::vector<Segment>::iterator it1;	// this holds the iterator which holds the segment before the intersection ref_point
	std::vector<Segment>::iterator it2;	// this holds the iterator which holds the segment after the intersection ref_point

	Intersection(int index1, glm::vec2 point)
	{
		this->index1 = index1;
		this->point = point;
	}

	Intersection(int index1, int index2, glm::vec2 point, int tag)
	{
		this->index1 = index1;
		this->index2 = index2;
		this->point = point;
		this->tag = tag;
	}
};

bool compare_intersection_x_smaller(Intersection& v1, Intersection& v2)
{
	return (v1.point.x < v2.point.x);
}

bool compare_intersection_x_greater(Intersection& v1, Intersection& v2)
{
	return (v1.point.x > v2.point.x);
}

bool compare_intersection_y_smaller(Intersection& v1, Intersection& v2)
{
	return (v1.point.y < v2.point.y);
}

bool compare_intersection_y_greater(Intersection& v1, Intersection& v2)
{
	return (v1.point.y > v2.point.y);
}

bool compare_intersection_a_greater(Intersection& v1, Intersection& v2)
{
	return (v1.a > v2.a);
}

bool compare_intersection_a_lower(Intersection& v1, Intersection& v2)
{
	return (v1.a < v2.a);
}

// to find and split at intersection points, what we to is :
// look for each intersection, it can't be between to adjacent line segment
// we store segment index that are intersected
// we store coordinates intersections, a segment can be intersected several times
// we then sort for each index intersection points to be in "line" from src to dst
// we then go through the curve, for each intersection we jump to next index
std::vector<Curve> Curve::split_at_intersections()
{
	//float s_f = 1;
	std::vector<Curve> result;

	Curve& a = *this; // just for ease of use

	bounds(); // be sure that bounds are already computed

	if (size() == 0)
		return result;

	if (a.size() == 1 && a[0].type == SegmentType::Circle)
	{
		result.push_back(a);
	}
	else
	{
		//scale(s_f);

		std::vector<int> indices; // hold indices of 
		std::vector<std::vector<Intersection>> intersections;

		// we construct an array to store intersection points for each index
		// a segment can have several intersections
		// so we will push into the list associated to the segment every intersections detected
		// as it is self intersection, we have to push the intersection for both segments i and j
		intersections.reserve(a.size());
		for (int i = 0; i < a.size(); i++)
			intersections.push_back(std::vector<Intersection>());

		int tag = 0;
		counter[5] = 0;

		for (int i = 0; i + 1 < a.size(); i++)
		{
			Segment s1 = a[i];
			glm::vec dst1 = a[i + 1].point;

			auto candidates = a.search(s1.bounds);

			//for (int j = i + 1; j < a.size(); j++)
			for (int k = 0; k < candidates.size(); k++)
			{
				int j = candidates[k].index;

				if (j > i)
				{
					Segment s2 = a[j];
					glm::vec2 dst2 = (j == a.size() - 1) ? a[0].point : a[j + 1].point;

					if (j == i + 1 && !(a[i].type == SegmentType::Line && a[j].type == SegmentType::Line) || j != i + 1) // we do not test adjacente line segments
					{
						counter[5]++;
						int count = 0;
						glm::vec2 pp[2];

						if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
						{
							glm::vec2 p = glm::vec2();
							if (geometry::segment_segment_intersect(s1.point, dst1, s2.point, dst2, p))
							{
								pp[0] = p;
								count = 1;
							}
						}
						else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
						{
							count = geometry::segment_arc_intersect(s1.point, dst1, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
						}
						else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
						{
							count = geometry::segment_arc_intersect(s2.point, dst2, s1.point, s1.center, dst1, s1.radius, s1.cw, pp);
						}
						else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
						{
							count = geometry::arc_arc_intersect(s1.point, s1.center, dst1, s1.radius, s1.cw, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
						}

						if (count == 1 && (glm::abs(i - j) == 1 || (i == 0 && j == a.size() - 1) || (j == 0 && i == a.size() - 1)))
							count = 0;

						for (int c = 0; c < count; c++)
						{
							intersections[i].push_back(Intersection(i, j, pp[c], tag));
							intersections[j].push_back(Intersection(j, i, pp[c], tag));
							if (std::find(indices.begin(), indices.end(), i) == indices.end())
								indices.push_back(i);
							if (std::find(indices.begin(), indices.end(), j) == indices.end())
								indices.push_back(j);
							tag++;
						}

					}
				}
			}
		}

		intersection_points.clear();

		if (indices.size() > 0)
		{
			for (const auto& ii : intersections)
				for (const auto& jj : ii)
					intersection_points.push_back(jj.point);

			// we have segments in indices and its intersections coordinates in intersections
			// we will sort for each segment its intersections
			// sorting will order same segment intersections along the segment. 
			// so if the segment [a,b] has 3 intersections d,e,c we order them as first is the closest to [a], etc, to get [a, c, d, e, b]
			// for line segment, we use 'y' coordinates if vertical, 'x' coordinate otherwise
			// for arcs, we use the angle between [a] and intersection ref_point
			// so we go through 'indices' which hold where intersections have occured to avoid looking the entire 'intersections' list

			float _3_4_pi = 3 * glm::quarter_pi<float>();
			float _5_4_pi = 5 * glm::quarter_pi<float>();
			float _7_4_pi = 7 * glm::quarter_pi<float>();

			for (int i = 0; i < indices.size(); i++)
			{
				// if segment has more than one intersections, need to be sorted
				if (intersections[indices[i]].size() > 1) 
				{
					Segment s1 = a[indices[i]];
					Segment s2 = a[indices[i] == a.size() - 1 ? 0 : indices[i] + 1];

					if (s1.type == SegmentType::Line)
					{
						auto a = geometry::oriented_angle(s2.point, s1.point);
						if (a > glm::quarter_pi<float>() && a < _3_4_pi || a > _5_4_pi && a < _7_4_pi)
						//if (s1.ref_point.x == s2.ref_point.x) // vertical
						{
							if (s1.point.y < s2.point.y)
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_y_smaller);
							else
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_y_greater);
						}
						else
						{
							if (s1.point.x < s2.point.x)
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_x_smaller);
							else
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_x_greater);
						}
					}
					else // for arcs, we have to sort following the oriented angle
					{
						auto a1 = geometry::oriented_angle(s1.point, s1.center);
						float at = 0;
						if (s1.cw)
						{
							for (Intersection& inter : intersections[indices[i]])
							{
								inter.a = geometry::oriented_angle(inter.point, s1.center);
								if (inter.a > a1)
									inter.a -= glm::two_pi<float>();
							}
							std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_a_greater);
						}
						else
						{
							for (Intersection& inter : intersections[indices[i]])
							{
								inter.a = geometry::oriented_angle(inter.point, s1.center);
								if (inter.a < a1)
									inter.a += glm::two_pi<float>();
							}
							std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_a_lower);
						}
					}
				}
			}

			// we have to loop into the curve and intersections to create the new curves
			// we will construct a new curve with splitted intersected segments 
			// then we link into segment's tag  with the intersected segment
			// then we are able to go throught the curve and split it into several curves

			std::list<Segment> b;

			std::map<int, std::list<Segment>::iterator> tags;

			for (int i = 0; i < a.size(); i++)
			{
				Segment& s = a[i];
				s.tag = (int)b.size(); // we store actual list index into tag, this helps to know where we are and avoid comparaison with iterators
				b.push_back(s);
				if (intersections[i].size() > 0)
				{
					// for each intersection, we add a segment, same as original but starting at intersection.ref_point
					for (int j = 0; j < intersections[i].size(); j++)
					{
						b.push_back(Segment(intersections[i][j].point, s.center, s.radius, s.cw, s.type, (int)b.size(), s.excluded));
						auto it = --b.end(); // last segment iterator is end() - 1
								
						// we look for existing key, if true, then we set reference to "next" field for both
						// else we inject in tags list this one

						// but as the intersection is done with another segment, we need to keep a reference on it
						// this is the role of tags list. we store this new segment reference 'it' to the intersection tag (not related to segment.tag we so earlier)
						// the intersection tag refers to both segment that intersect
						// if tag is already in tags, this means we are dealing with the second segment
						// so we can then create the reference which is as follow, segment s1 and segment s2 :
						// first = iterator(s1) - 1                     c\      /b                     
						// first.next = iterator(s2)                      \    /                    it(s1)-1 [a;i]
						// first.next_tag = s2.tag      iterator(s2)-1     \  /						it(s1)   [i;b]
						// second = iterator(s2) - 1			           i\/    iterator(s1)		it(s2)-1 [c;i]
						// second.next = s1.tag                             /\						it(s2)   [i;d]
						// second.next_tag = s1.next    iterator(s1) - 1  a/  \d  iterator(s2)
						if (tags.find(intersections[i][j].tag) == tags.end())
						{
							tags[intersections[i][j].tag] = it; 
						}
						else
						{
							auto first = (tags[intersections[i][j].tag] == b.begin()) ? --b.end() : std::prev(tags[intersections[i][j].tag]);
							(*first).next = it;
							(*first).next_tag = (*it).tag;

							auto second = (it == b.begin()) ? --b.end() : std::prev(it);
							(*second).next = tags[intersections[i][j].tag];
							(*second).next_tag = (*(*second).next).tag;

							tags.erase(intersections[i][j].tag);
						}
					}
				}
			}

			//int index = 0;
			//for (auto it : b)
			//	it.tag = index++;
			//for (auto it : b)
			//{
			//	if (it.next_tag != -1)
			//		it.next_tag = (*it.next).tag;
			//}

			// following previous comments, to split the curve, the process is following :
			// when meeting it(s1)-1, we can see that next_tag is filled
			// so we add it(s1)-1 segment wich is [a;i], then we go to next segment wich is it(s2) and add [i;d]
			// we do it as next tag equal to first, meaning that the curve is closed
			// we do it again until list is empty

			while (b.size() > 1)
			{
				Curve c;
				auto it = b.begin();
				auto next = it;
				auto first = *it;
				bool excluded = false;

				do
				{
					c.push_back(*it);
					if ((*it).excluded) // 
						excluded = true;

					if ((*it).next_tag != first.tag)
					{
						if ((*it).next_tag != -1)
						{
							next = (*it).next;
						}
						else
							next = std::next(it);

						b.erase(it);
						it = next;
					}
					else
					{
						b.erase(it);
						it = b.end();
					}

				} while (b.size() > 0 && it != b.end() && (*it).tag != first.tag);

				c.reset_bounds();
				if (!excluded)
				{
					result.push_back(c);
				}
			}
		}
		else
			result.push_back(a);
	}

	//for (Curve& c : result)
	//	c.scale(1.0f / s_f);

	//for (glm::vec2& p : intersection_points)
	//	p = p * (1.0f / s_f);

	return result;
}


// ---------------------------------------------------------------------------------------------------------------------
// Robust split of a raw offset curve.
//
// The raw offset curve (see trim()) is full of loops that must be removed (swallow tails). Linking the pieces at each
// intersection point only works if the order of the intersections along each segment is consistent with the order along
// the curve, which is not the case when intersections are closer than what a float can represent (a cusp folded on a line).
// Here the decision does not rely on that order : the curve is cut at every intersection into sub edges, and each sub edge
// is kept or not by looking at the winding number of the whole raw curve on each side of it. The edges that bound the area
// with a positive winding (relatively to the direction of the original curve) are the valid ones. They are then chained into
// closed curves.
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
	const double TWO_PI = glm::two_pi<double>();

	double norm_angle(double a)
	{
		a = std::fmod(a, TWO_PI);
		if (a < 0)
			a += TWO_PI;
		return a;
	}

	// intersections are computed again in double precision, from the same float inputs, to get a reliable position of each
	// intersection along each of the two segments
	glm::dvec2 refine_line_line(glm::vec2 a1, glm::vec2 a2, glm::vec2 b1, glm::vec2 b2, glm::vec2 guess)
	{
		glm::dvec2 p1(a1), d1 = glm::dvec2(a2) - p1, p2(b1), d2 = glm::dvec2(b2) - p2;
		double den = d1.x * d2.y - d1.y * d2.x;
		if (den == 0)
			return glm::dvec2(guess);
		double t = ((p2.x - p1.x) * d2.y - (p2.y - p1.y) * d2.x) / den;
		return p1 + d1 * t;
	}

	glm::dvec2 refine_line_arc(glm::vec2 a1, glm::vec2 a2, glm::vec2 center, float radius, glm::vec2 guess)
	{
		glm::dvec2 p(a1), d = glm::dvec2(a2) - p, c(center), g(guess);
		glm::dvec2 f = p - c;
		double a = glm::dot(d, d), b = 2 * glm::dot(f, d), cc = glm::dot(f, f) - (double)radius * (double)radius;
		if (a == 0)
			return g;
		double disc = b * b - 4 * a * cc;
		if (disc < 0)
			disc = 0;
		double s = std::sqrt(disc);
		glm::dvec2 r1 = p + d * ((-b - s) / (2 * a)), r2 = p + d * ((-b + s) / (2 * a));
		return glm::dot(r1 - g, r1 - g) <= glm::dot(r2 - g, r2 - g) ? r1 : r2;
	}

	glm::dvec2 refine_arc_arc(glm::vec2 center1, float radius1, glm::vec2 center2, float radius2, glm::vec2 guess)
	{
		glm::dvec2 c1(center1), c2(center2), g(guess);
		glm::dvec2 v = c2 - c1;
		double dist = glm::length(v);
		if (dist == 0)
			return g;
		double r1 = radius1, r2 = radius2;
		double a = (r1 * r1 - r2 * r2 + dist * dist) / (2 * dist);
		double h2 = r1 * r1 - a * a;
		if (h2 < 0)
			h2 = 0;
		double h = std::sqrt(h2);
		glm::dvec2 m = c1 + v * (a / dist), n(-v.y / dist, v.x / dist);
		glm::dvec2 q1 = m + n * h, q2 = m - n * h;
		return glm::dot(q1 - g, q1 - g) <= glm::dot(q2 - g, q2 - g) ? q1 : q2;
	}

	glm::dvec2 refine_intersection(const Segment& s1, glm::vec2 dst1, const Segment& s2, glm::vec2 dst2, glm::vec2 guess)
	{
		if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
			return refine_line_line(s1.point, dst1, s2.point, dst2, guess);
		if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
			return refine_line_arc(s1.point, dst1, s2.center, s2.radius, guess);
		if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
			return refine_line_arc(s2.point, dst2, s1.center, s1.radius, guess);
		if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
			return refine_arc_arc(s1.center, s1.radius, s2.center, s2.radius, guess);
		return glm::dvec2(guess);
	}

	// position of point p along the segment, growing from the start to the end of the segment
	double position_along(const Segment& s, glm::vec2 dst, glm::dvec2 p)
	{
		if (s.type == SegmentType::Line)
		{
			glm::dvec2 a(s.point), d = glm::dvec2(dst) - a;
			double len2 = glm::dot(d, d);
			return len2 == 0 ? 0 : glm::dot(p - a, d) / len2;
		}

		glm::dvec2 c(s.center);
		double a0 = std::atan2((double)s.point.y - c.y, (double)s.point.x - c.x);
		double a1 = std::atan2(p.y - c.y, p.x - c.x);
		double a2 = std::atan2((double)dst.y - c.y, (double)dst.x - c.x);
		double sweep = s.cw ? a0 - a2 : a2 - a0;
		while (sweep <= 0) sweep += TWO_PI;
		double angle = s.cw ? a0 - a1 : a1 - a0;
		// angles outside of the arc are centered on the gap, so a point at the very start or end of the arc is not wrapped
		double low = -(TWO_PI - sweep) / 2;
		while (angle < low) angle += TWO_PI;
		while (angle >= low + TWO_PI) angle -= TWO_PI;
		return angle;
	}

	// a segment of the raw curve, in the frame used to cast the winding rays
	struct WindingSegment
	{
		bool arc = false;
		glm::dvec2 p, q, c;
		double r = 0, a0 = 0, sweep = 0;
		bool cw = false;
	};

	class Winding
	{
	private:
		std::vector<WindingSegment> _segments;
		// the rays are cast along a direction which is not aligned with the coordinates, so a ray never goes exactly through a vertex
		double _cos = std::cos(0.7390851332), _sin = std::sin(0.7390851332);

		glm::dvec2 rotate(glm::dvec2 v) const { return glm::dvec2(v.x * _cos + v.y * _sin, -v.x * _sin + v.y * _cos); }

	public:
		void add(const Segment& s, glm::vec2 dst)
		{
			WindingSegment w;
			w.p = rotate(glm::dvec2(s.point));
			w.q = rotate(glm::dvec2(dst));
			if (s.type == SegmentType::Arc)
			{
				w.arc = true;
				w.cw = s.cw;
				w.c = rotate(glm::dvec2(s.center));
				w.r = glm::length(w.p - w.c);
				w.a0 = std::atan2(w.p.y - w.c.y, w.p.x - w.c.x);
				double a1 = std::atan2(w.q.y - w.c.y, w.q.x - w.c.x);
				w.sweep = s.cw ? norm_angle(w.a0 - a1) : norm_angle(a1 - w.a0);
			}
			_segments.push_back(w);
		}

		int at(glm::dvec2 point) const
		{
			glm::dvec2 p = rotate(point);
			int winding = 0;
			for (const WindingSegment& s : _segments)
			{
				if (!s.arc)
				{
					if ((s.p.y <= p.y) != (s.q.y <= p.y))
					{
						double x = s.p.x + (p.y - s.p.y) * (s.q.x - s.p.x) / (s.q.y - s.p.y);
						if (x > p.x)
							winding += s.q.y > s.p.y ? 1 : -1;
					}
				}
				else
				{
					double dy = p.y - s.c.y;
					if (std::abs(dy) < s.r)
					{
						double dx = std::sqrt(s.r * s.r - dy * dy);
						for (int sign = -1; sign <= 1; sign += 2)
						{
							double x = s.c.x + sign * dx;
							if (x > p.x)
							{
								double theta = std::atan2(dy, x - s.c.x);
								double t = s.cw ? norm_angle(s.a0 - theta) : norm_angle(theta - s.a0);
								if (t < s.sweep)
								{
									double vy = s.cw ? -std::cos(theta) : std::cos(theta);
									winding += vy > 0 ? 1 : -1;
								}
							}
						}
					}
				}
			}
			return winding;
		}
	};

	struct Hit
	{
		double key;
		glm::vec2 point;
		int node;
	};

	struct SubEdge
	{
		int segment;
		int from, to;
		glm::vec2 p1, p2;
		bool keep = false;
		glm::dvec2 tangent_start, tangent_end;
	};

	int find_root(std::vector<int>& parent, int i)
	{
		while (parent[i] != i)
		{
			parent[i] = parent[parent[i]];
			i = parent[i];
		}
		return i;
	}
}

std::vector<Curve> Curve::split_at_intersections(int orientation)
{
	Curve& a = *this;
	const int n = (int)a.size();

	for (const Segment& s : a)
		if (s.type == SegmentType::Circle)
			return split_at_intersections();

	if (n < 3 || orientation == 0)
		return split_at_intersections();

	bounds();

	// real segments of the curve, the last one goes back to the first point
	auto dst_of = [&](int i) { return i == n - 1 ? a[0].point : a[i + 1].point; };
	auto real = [&](int i) { return a[i].point != dst_of(i); };

	// 1. every intersection, with its position along each of the two segments
	std::vector<glm::vec2> points;
	for (int i = 0; i < n; i++)
		points.push_back(a[i].point);

	std::vector<std::vector<Hit>> hits(n);
	int hit_count = 0;

	// a curve which does not lie on itself has a few intersections. When there are way more than the number of segments, parts of the curve
	// are on each other (offsets that touch), the intersections can not be linked and there is nothing valid to extract. The search stops
	// there because the number of intersections can reach the millions.
	const int max_hits = 4 * n + 64;

	for (int i = 0; i < n && hit_count <= max_hits; i++)
	{
		if (!real(i))
			continue;

		Segment s1 = a[i];
		glm::vec2 dst1 = dst_of(i);

		auto candidates = a.search(s1.bounds);
		for (size_t k = 0; k < candidates.size() && hit_count <= max_hits; k++)
		{
			int j = candidates[k].index;
			if (j <= i || !real(j))
				continue;

			Segment s2 = a[j];
			glm::vec2 dst2 = dst_of(j);

			// adjacent line segments only meet at their common point
			if (j == i + 1 && s1.type == SegmentType::Line && s2.type == SegmentType::Line)
				continue;

			int count = 0;
			glm::vec2 pp[2];

			if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
			{
				glm::vec2 p = glm::vec2();
				if (geometry::segment_segment_intersect(s1.point, dst1, s2.point, dst2, p))
				{
					pp[0] = p;
					count = 1;
				}
			}
			else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
				count = geometry::segment_arc_intersect(s1.point, dst1, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
				count = geometry::segment_arc_intersect(s2.point, dst2, s1.point, s1.center, dst1, s1.radius, s1.cw, pp);
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
				count = geometry::arc_arc_intersect(s1.point, s1.center, dst1, s1.radius, s1.cw, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);

			if (count == 1 && (j == i + 1 || (i == 0 && j == n - 1)))
				count = 0;

			for (int c = 0; c < count; c++)
			{
				glm::dvec2 exact = refine_intersection(s1, dst1, s2, dst2, pp[c]);
				int node = (int)points.size();
				points.push_back(pp[c]);
				hits[i].push_back({ position_along(s1, dst1, exact), pp[c], node });
				hits[j].push_back({ position_along(s2, dst2, exact), pp[c], node });
				hit_count++;
			}
		}
	}

	if (hit_count > max_hits)
		return std::vector<Curve>();

	if (hit_count == 0)
		return split_at_intersections();

	// 2. nodes : points closer than the tolerance are the same node
	const float eps = 2 * geometry::ERR_FLOAT3;
	const float eps2 = eps * eps;
	std::vector<int> parent(points.size());
	for (size_t i = 0; i < parent.size(); i++)
		parent[i] = (int)i;
	for (size_t i = 0; i < points.size(); i++)
		for (size_t j = i + 1; j < points.size(); j++)
		{
			glm::vec2 d = points[i] - points[j];
			if (d.x * d.x + d.y * d.y <= eps2)
			{
				int ri = find_root(parent, (int)i), rj = find_root(parent, (int)j);
				if (ri != rj)
					parent[std::max(ri, rj)] = std::min(ri, rj);
			}
		}
	// an intersection which is found a few microns before the start (or after the end) of a segment is the vertex itself : it is the same node.
	// Taken as a stop of the segment, it would give a sub edge going backwards, which is a whole circle for an arc.
	auto out_of_range = [&](int i, double key)
	{
		double end = 1;
		if (a[i].type == SegmentType::Arc)
		{
			glm::dvec2 c(a[i].center);
			double a0 = std::atan2((double)a[i].point.y - c.y, (double)a[i].point.x - c.x);
			double a2 = std::atan2((double)dst_of(i).y - c.y, (double)dst_of(i).x - c.x);
			end = a[i].cw ? a0 - a2 : a2 - a0;
			while (end <= 0)
				end += TWO_PI;
		}
		return key < 0 ? -1 : key > end ? 1 : 0;
	};
	for (int i = 0; i < n; i++)
	{
		if (!real(i))
			continue;
		for (const Hit& h : hits[i])
		{
			int side = out_of_range(i, h.key);
			if (side == 0)
				continue;
			int ri = find_root(parent, h.node), rj = find_root(parent, side < 0 ? i : (i == n - 1 ? 0 : i + 1));
			if (ri != rj)
				parent[std::max(ri, rj)] = std::min(ri, rj);
		}
	}
	auto node_of = [&](int point_index) { return find_root(parent, point_index); };

	// 3. sub edges
	std::vector<SubEdge> edges;
	for (int i = 0; i < n; i++)
	{
		if (!real(i))
			continue;

		std::sort(hits[i].begin(), hits[i].end(), [](const Hit& h1, const Hit& h2) { return h1.key < h2.key; });

		struct Stop { int node; glm::vec2 point; };
		std::vector<Stop> stops;
		stops.push_back({ node_of(i), a[i].point });
		for (const Hit& h : hits[i])
			if (out_of_range(i, h.key) == 0)
				stops.push_back({ node_of(h.node), h.point });
		stops.push_back({ node_of(i == n - 1 ? 0 : i + 1), dst_of(i) });

		for (size_t k = 0; k + 1 < stops.size(); k++)
		{
			SubEdge e;
			e.segment = i;
			e.from = stops[k].node;
			e.to = stops[k + 1].node;
			e.p1 = stops[k].point;
			e.p2 = stops[k + 1].point;
			edges.push_back(e);
		}
	}

	// 4. winding number of the raw curve
	Winding winding;
	for (int i = 0; i < n; i++)
		if (real(i))
			winding.add(a[i], dst_of(i));

	const double delta = 2e-4;
	auto inside = [&](int w) { return orientation * w > 0; };

	for (SubEdge& e : edges)
	{
		const Segment& s = a[e.segment];
		glm::dvec2 mid, tangent;
		double length;

		if (s.type == SegmentType::Line)
		{
			glm::dvec2 p1(e.p1), p2(e.p2), d = p2 - p1;
			length = glm::length(d);
			if (length == 0)
				continue;
			tangent = d / length;
			mid = (p1 + p2) / 2.0;
			e.tangent_start = e.tangent_end = tangent;
		}
		else
		{
			glm::dvec2 c(s.center), p1(e.p1), p2(e.p2);
			// the radius is the one used by the winding (distance from the segment start to the center) : the end points of the edge
			// are intersections which can be off the arc by more than the sampling distance
			double r = glm::length(glm::dvec2(s.point) - c);
			double a1 = std::atan2(p1.y - c.y, p1.x - c.x), a2 = std::atan2(p2.y - c.y, p2.x - c.x);
			double sweep = s.cw ? norm_angle(a1 - a2) : norm_angle(a2 - a1);
			length = r * sweep;
			if (sweep < 1e-12)
				continue;
			double am = s.cw ? a1 - sweep / 2 : a1 + sweep / 2;
			mid = c + glm::dvec2(std::cos(am), std::sin(am)) * r;
			tangent = s.cw ? glm::dvec2(std::sin(am), -std::cos(am)) : glm::dvec2(-std::sin(am), std::cos(am));
			e.tangent_start = s.cw ? glm::dvec2(std::sin(a1), -std::cos(a1)) : glm::dvec2(-std::sin(a1), std::cos(a1));
			e.tangent_end = s.cw ? glm::dvec2(std::sin(a2), -std::cos(a2)) : glm::dvec2(-std::sin(a2), std::cos(a2));
		}

		// an edge which starts and ends on the same node and which is tiny is just noise
		if (e.from == e.to && length < 3 * eps)
			continue;

		glm::dvec2 left(-tangent.y, tangent.x);
		bool inside_left = inside(winding.at(mid + left * delta));
		bool inside_right = inside(winding.at(mid - left * delta));

		// the area to keep is on the left of the edge for a counter clockwise curve and on the right otherwise
		e.keep = orientation > 0 ? (inside_left && !inside_right) : (inside_right && !inside_left);
	}

	// a tiny edge can not be classified by looking at both of its sides, the sampling distance is too close to its size (a join arc which
	// is only a few hundredths long for instance). When it is not crossed by another part of the curve at one of its ends, it simply
	// follows the edge it is linked to.
	{
		std::vector<std::vector<int>> incident(points.size());
		for (size_t k = 0; k < edges.size(); k++)
		{
			incident[edges[k].from].push_back((int)k);
			incident[edges[k].to].push_back((int)k);
		}
		for (size_t k = 0; k < edges.size(); k++)
		{
			if (glm::distance(edges[k].p1, edges[k].p2) >= 0.1f || edges[k].from == edges[k].to)
				continue;
			for (int end = 0; end < 2; end++)
			{
				const int node = end == 0 ? edges[k].from : edges[k].to;
				if (incident[node].size() != 2)
					continue;
				const int other = incident[node][0] == (int)k ? incident[node][1] : incident[node][0];
				if (other != (int)k && glm::distance(edges[other].p1, edges[other].p2) >= 0.1f)
				{
					edges[k].keep = edges[other].keep;
					break;
				}
			}
		}
	}

	// two kept edges which run along each other between the same two nodes (a spike folded on itself, a loop which collapsed) are one edge
	for (size_t k = 0; k < edges.size(); k++)
	{
		if (!edges[k].keep)
			continue;
		for (size_t m = k + 1; m < edges.size(); m++)
		{
			if (!edges[m].keep || edges[m].from != edges[k].from || edges[m].to != edges[k].to)
				continue;
			const Segment& s1 = a[edges[k].segment];
			const Segment& s2 = a[edges[m].segment];
			// two tiny edges between the same nodes are the two sides of a sliver, which is not a piece of the result either
			const bool tiny = glm::distance(edges[k].p1, edges[k].p2) < 0.05f;
			if (tiny || (s1.type == s2.type && (s1.type == SegmentType::Line || (s1.cw == s2.cw && glm::distance(s1.center, s2.center) <= eps && glm::abs(s1.radius - s2.radius) <= eps))))
				edges[m].keep = false;
		}
	}

	// 5. chain the edges which are kept into closed curves
	std::vector<std::vector<int>> outgoing(points.size());
	for (size_t k = 0; k < edges.size(); k++)
		if (edges[k].keep)
			outgoing[edges[k].from].push_back((int)k);

	std::vector<bool> used(edges.size(), false);
	std::vector<Curve> result;
	bool ok = true;

	for (size_t k = 0; k < edges.size() && ok; k++)
	{
		if (!edges[k].keep || used[k])
			continue;

		std::vector<int> loop;
		int current = (int)k;
		int start = edges[k].from;

		while (true)
		{
			used[current] = true;
			loop.push_back(current);
			int node = edges[current].to;
			if (node == start)
				break;

			// next edge : on a node where several edges leave, we keep the one which turns the most towards the area to keep,
			// which gives loops that do not cross each other
			int best = -1;
			double best_turn = 0;
			for (int candidate : outgoing[node])
			{
				if (used[candidate])
					continue;
				glm::dvec2 t1 = edges[current].tangent_end, t2 = edges[candidate].tangent_start;
				double turn = std::atan2(t1.x * t2.y - t1.y * t2.x, glm::dot(t1, t2));
				if (best == -1 || (orientation > 0 ? turn > best_turn : turn < best_turn))
				{
					best = candidate;
					best_turn = turn;
				}
			}

			if (best == -1)
			{
				ok = false;
				break;
			}
			current = best;
		}

		if (!ok)
			break;

		Curve c;
		for (int index : loop)
		{
			const Segment& s = a[edges[index].segment];
			glm::vec2 from = edges[index].p1;
			if (s.type == SegmentType::Line)
				c.add(from);
			else
				c.add(from, s.center, s.radius, s.cw);
		}
		c.close();
		c.reset_bounds();
		result.push_back(c);
	}

	// if the edges can not be chained, we use the previous way of splitting
	if (!ok)
		return split_at_intersections();

	return result;
}

std::vector<Curve> Curve::boolean_union(Curve& b)
{
	// union means both curve have same direction
	std::vector<Curve> result = weiler(b, true, true);
	for (Curve& c : result)
		connect_reversed_arcs(c);
	result.erase(std::remove_if(result.begin(), result.end(), [](Curve& c) { return c.size() == 0; }), result.end());
	return result;
}

std::vector<Curve> Curve::boolean_substract(Curve& b)
{
	// substration means curves direction are opposit
	std::vector<Curve> result = weiler(b, false, true);
	for (Curve& c : result)
		connect_reversed_arcs(c);
	result.erase(std::remove_if(result.begin(), result.end(), [](Curve& c) { return c.size() == 0; }), result.end());
	return result;
}

std::vector<Curve> Curve::boolean_intersect(Curve& b)
{
	std::vector<Curve> result = weiler(b, true, false);
	for (Curve& c : result)
		connect_reversed_arcs(c);
	result.erase(std::remove_if(result.begin(), result.end(), [](Curve& c) { return c.size() == 0; }), result.end());
	return result;
}

std::vector<Curve> Curve::boolean_not_intersect(Curve& b)
{
	std::vector<Curve> result = weiler(b, false, false);
	for (Curve& c : result)
		connect_reversed_arcs(c);
	result.erase(std::remove_if(result.begin(), result.end(), [](Curve& c) { return c.size() == 0; }), result.end());
	return result;
}

bool Curve::too_small(float o)
{
	return /*bounds().width() + bounds().height() < glm::abs(o)/2 ||*/ bounds().height() < 0.05 * glm::abs(o) || bounds().width() < 0.05 * glm::abs(o);
}

bool Curve::too_close(Curve& test, float o)
{
	auto r = glm::abs(o);
	auto hr = r * 0.98f;
	auto hr2 = hr * hr;
	for (int i = 0; i < test.size(); i++)
	{
		Segment& s1 = test[i];
		glm::vec2 d1 = test[i < test.size() - 1 ? i + 1 : 0].point;
		glm::vec2 m1 = s1.type == SegmentType::Line ? geometry::middle(s1.point, d1) : geometry::arc_middle(s1.point, s1.center, d1, s1.cw);

		auto candidates = search(s1.bounds.offset(r));

		for (int k = 0; k < candidates.size(); k++)
		{
			counter[0]++;
			int j = candidates[k].index;

			Segment& s2 = candidates[k]; //original[j];
			glm::vec2 d2 = (*this)[j < size() - 1 ? j + 1 : 0].point;
			glm::vec2 m2 = s2.type == SegmentType::Line ? geometry::middle(s2.point, d2) : geometry::arc_middle(s2.point, s2.center, d2, s2.cw);

			glm::vec2 p, pp[2];
			float d;
			d = geometry::distance2(s2.point, m1);
			if (d < hr2)
				return true;
			d = geometry::distance2(d2, m1);
			if (d < hr2)
				return true;
			d = geometry::distance2(m2, m1);
			if (d < hr2)
				return true;
			d = geometry::distance2(s2.point, s1.point);
			if (d < hr2)
				return true;
			d = geometry::distance2(d2, s1.point);
			if (d < hr2)
				return true;
			d = geometry::distance2(m2, s1.point);
			if (d < hr2)
				return true;
			d = geometry::distance2(s2.point, d1);
			if (d < hr2)
				return true;
			d = geometry::distance2(d2, d1);
			if (d < hr2)
				return true;
			d = geometry::distance2(m2, d1);
			if (d < hr2)
				return true;


			//if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc && geometry::arc_arc_intersect(s2.ref_point, s2.center, d2, s2.cw, m1, hr, pp) == 2)
			//	return true;

			//// more checking
			//if (s1.type == SegmentType::Line && s2.type == SegmentType::Line && geometry::segment_segment_intersect(s1.ref_point, d1, s2.ref_point, d2, p))
			//	return true;
			if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
			{
				// if segment intersect arc
				if ( geometry::segment_arc_intersect(s2.point, d2, s1.point, s1.center, d1, s1.cw, pp))
					return true;

				// get projection (p) of arc center (c) on segment, if p is a segment ref_point and the line going through (p,c) cross arc in (r), check (p,r) distance
				p = geometry::projection(s1.center, s2.point, d2);
				if (geometry::colinear_segment_point(s2.point, d2, p))
				{
					int count = geometry::line_arc_intersect(s1.center, p, s1.point, s1.center, d1, s1.cw, pp);
					for (int ip=0; ip<count; ip++)
					{
						if (geometry::distance(pp[ip], p) < hr)
							return true;
					}
				}
			}
			if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
			{
				if (geometry::segment_arc_intersect(s1.point, d1, s2.point, s2.center, d2, s2.cw, pp))
					return true;

				// get projection (p) of arc center (c) on segment, if p is a segment ref_point and the line going through (p,c) cross arc in (r), check (p,r) distance
				p = geometry::projection(s2.center, s1.point, d1);
				if (geometry::colinear_segment_point(s1.point, d1, p))
				{
					int count = geometry::line_arc_intersect(s2.center, p, s2.point, s2.center, d2, s2.cw, pp);
					for (int ip = 0; ip < count; ip++)
					{
						if (geometry::distance(pp[ip], p) < hr)
							return true;
					}
				}
			}
			if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc && geometry::arc_arc_intersect(s1.point, s1.center, d1, s1.cw, s2.point, s2.center, d2, s2.cw, pp))
				return true;
		}
	}

	//auto r = glm::abs(o);
	//auto hr = r * 0.999f;

	//for (int i = 0; i < test.size(); i++)
	//{
	//	Segment& s1 = test[i];
	//	glm::vec2 src1 = s1.ref_point, dst1 = test[i < test.size() - 1 ? i + 1 : 0].ref_point;
	//	//glm::vec2 m1 = s1.type == SegmentType::Line ? geometry::middle(s1.ref_point, d1) : geometry::arc_middle(s1.ref_point, s1.center, d1, s1.cw);

	//	auto candidates = search(s1.bounds.offset(r));

	//	for (int k = 0; k < candidates.size(); k++)
	//	{
	//		counter[0]++;
	//		int j = candidates[k].index;
	//		float d = 0;

	//		Segment& s2 = candidates[k]; //original[j];
	//		glm::vec2 src2 = s2.ref_point, dst2 = (*this)[j < size() - 1 ? j + 1 : 0].ref_point;

	//		if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
	//		{
	//			int count = 0;
	//			glm::vec2 pp[2];
	//			count = geometry::segment_circle_intersect(src1, dst1, src2, hr, pp);
	//			if (count > 0)
	//				for (int k = 0; k < count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], src2)) < hr)
	//						return true;
	//			count = geometry::segment_circle_intersect(src1, dst1, dst2, hr, pp);
	//			if (count > 0)
	//				for (int k = 0; k < count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], dst2)) < hr)
	//					return true;
	//			count = geometry::segment_circle_intersect(src2, dst2, src1, hr, pp);
	//			if (count > 0)
	//				for (int k = 0; k < count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], src1)) < hr)
	//					return true;
	//			count = geometry::segment_circle_intersect(src2, dst2, dst1, hr, pp);
	//			if (count > 0)
	//				for (int k = 0; k < count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], dst1)) < hr)
	//					return true;
	//		}
	//		else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
	//		{
	//			int count = 0;
	//			glm::vec2 pp[2];
	//			auto p = geometry::projection(s2.center, src1, dst1);
	//			// if projection of arc center on the segment is outside the segment, then closest ref_point to arc is the closest segment end ref_point
	//			if (!geometry::colinear_segment_point(src1, dst1, p))
	//			{
	//				if (geometry::distance2(s2.center, src1) < geometry::distance2(s2.center, dst1))
	//					p = src1;
	//				else
	//					p = dst1;
	//			}
	//			// if intersection [s2.center, p] is on arc, then shortest distance is distance(p, intersection), else
	//			// it is the shortest distance between arc end points and p
	//			count = geometry::line_arc_intersect(s2.center, p, src2, s2.center, dst2, s2.cw, pp);
	//			if (count > 0)
	//			{
	//				for (int k=0; k<count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], p)) < hr)
	//						return true;
	//			}
	//			else
	//			{
	//				if (d = glm::min(geometry::distance(p, dst2), geometry::distance(p, src2)) < hr)
	//					return true;
	//			}
	//		}
	//		else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
	//		{
	//			int count = 0;
	//			glm::vec2 pp[2];
	//			auto p = geometry::projection(s1.center, src2, dst2);
	//			// if projection of arc center on the segment is outside the segment, then closest ref_point to arc is the closest segment end ref_point
	//			if (!geometry::colinear_segment_point(src2, dst2, p))
	//			{
	//				if (geometry::distance2(s1.center, src2) < geometry::distance2(s1.center, dst2))
	//					p = src2;
	//				else
	//					p = dst2;
	//			}
	//			// if intersection [s2.center, p] is on arc, then shortest distance is distance(p, intersection), else
	//			// it is the shortest distance between arc end points and p
	//			count = geometry::line_arc_intersect(s1.center, p, src1, s1.center, dst1, s1.cw, pp);
	//			if (count > 0)
	//			{
	//				for (int k = 0; k < count; k++)
	//					if (d = glm::max(d, geometry::distance(pp[k], p)) < hr)
	//						return true;
	//			}
	//			else
	//			{
	//				if (d = glm::min(geometry::distance(p, dst1), geometry::distance(p, src1)) < hr)
	//					return true;
	//			}
	//		}
	//		else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
	//		{
	//			int count1 = 0, count2 = 0;
	//			glm::vec2 pp1[2], pp2[2];
	//			
	//			count1 = geometry::line_arc_intersect(s1.center, s2.center, src1, s1.center, dst1, s1.cw, pp1);
	//			count2 = geometry::line_arc_intersect(s1.center, s2.center, src2, s2.center, dst2, s2.cw, pp2);

	//			if (count1 == 1 && count2 == 1)
	//			{
	//				if (d = geometry::distance(pp1[0], pp2[0]) < hr)
	//					return true;
	//			}
	//			else if (count1 == 1 && count2 == 0)
	//			{
	//				if (d = glm::min(geometry::distance(pp1[0], src2), geometry::distance(pp1[0], src2)) < hr)
	//					return true;
	//			}
	//			else if (count1 == 0 && count2 == 1)
	//			{
	//				if (d = glm::min(geometry::distance(pp2[0], src1), geometry::distance(pp2[0], src1)) < hr)
	//					return true;
	//			}
	//			else
	//			{
	//				auto m = 0;
	//			}
	//			
	//		}
	//	}
	//}

	return false;
}

bool Curve::too_close2(Curve& test, float o)
{
	auto r = glm::abs(o);
	auto hr = r * 0.999;

	for (int i = 0; i < test.size(); i++)
	{
		Segment& s1 = test[i];
		glm::vec2 src1 = s1.point, dst1 = test[i < test.size() - 1 ? i + 1 : 0].point;

		auto candidates = search(s1.bounds.offset(r));

		for (int k = 0; k < candidates.size(); k++)
		{
			counter[0]++;
			int j = candidates[k].index;
			float d = 0;

			Segment& s2 = candidates[k]; //original[j];
			glm::vec2 src2 = s2.point, dst2 = (*this)[j < size() - 1 ? j + 1 : 0].point;

			if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
			{
				// check all cases :
				// projection of src1 on [src2,dst2]
				auto p1 = geometry::projection(src1, src2, dst2);
				if (geometry::colinear_segment_point(src2, dst2, p1))
				{
					if (d = glm::max(d, geometry::distance(p1, src1)) < hr)
						return true;
				}
				// projection of dst1 on [src2,dst2]
				auto p2 = geometry::projection(dst1, src2, dst2);
				if (geometry::colinear_segment_point(src2, dst2, p2))
				{
					if (d = glm::max(d, geometry::distance(p2,dst1)) < hr)
						return true;
				}
				// projection of src2 on [src1,dst1]
				auto p3 = geometry::projection(src2, src1, dst1);
				if (geometry::colinear_segment_point(src1, dst1, p3))
				{
					if (d = glm::max(d, geometry::distance(p3, src2)) < hr)
						return true;
				}
				// projection of dst2 on [src1,dst1]
				auto p4 = geometry::projection(dst2, src1, dst1);
				if (geometry::colinear_segment_point(src1, dst1, p4))
				{
					if (d = glm::max(d, geometry::distance(p4, dst2)) < hr)
						return true;
				}
				// distance of src1 to src2
				if (d = glm::max(d, geometry::distance(src1, src2)) < hr)
					return true;
				// distance of src1 to dst2
				if (d = glm::max(d, geometry::distance(src1, dst2)) < hr)
					return true;
				// distance of dst1 to src2
				if (d = glm::max(d, geometry::distance(dst1, src2)) < hr)
					return true;
				// distance of dst1 to dst2
				if (d = glm::max(d, geometry::distance(dst1, dst2)) < hr)
					return true;
			}
		}
	}

	return false;
}

bool Curve::curve_intersect(Curve& test)
{
	Curve& a = size() < test.size() ? *this : test;
	Curve& b = size() < test.size() ? test : *this;

	for (int i = 0; i < a.size(); i++)
	{
		auto candidates = b.search(a[i].bounds);

		for (int j = 0; j < candidates.size(); j++)
		{
			Segment& s1 = a[i];
			glm::vec2 d1 = i == a.size() - 1 ? a[0].point : a[i + 1].point;
			Segment& s2 = candidates[j];
			glm::vec2 d2 = candidates[j].index == b.size() - 1 ? b[0].point : b[candidates[j].index + 1].point;

			if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
			{
				glm::vec2 p;
				if (geometry::segment_segment_intersect(s1.point, d1, s2.point, d2, p))
					return true;
			}
			else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
			{
				glm::vec2 pp[2];
				if (geometry::segment_arc_intersect(s1.point, d1, s2.point, s2.center, d2, s2.cw, pp) > 0)
					return true;
			}
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
			{
				glm::vec2 pp[2];
				if (geometry::segment_arc_intersect(s2.point, d2, s1.point, s1.center, d1, s1.cw, pp) > 0)
					return true;
			}
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
			{
				glm::vec2 pp[2];
				if (geometry::arc_arc_intersect(s2.point, s2.center, d2, s2.cw, s1.point, s1.center, d1, s1.cw, pp) > 0)
					return true;
			}
		}
	}

	return false;
}


//bool compare_segment_x_smaller2(Segment& v1, Segment& v2)
//{
//	return (v1.bounds.bottom_right.x > v2.bounds.bottom_right.x);
//}

std::vector<Segment> Curve::search(geometry::rectangle bounds)
{
	// Method one, segments are sorted following the x coordinate
	// then we compare the bounds with segments bounds. 
	// As the segments bounds are sorted with left pos,
	// we can exclude until x is reach, then exclude if left is outside bounds
	//if (_sorted2.size() == 0)
	//{
	//	for (Segment& p : *this)
	//		_sorted2.push_back(p);

	//	std::sort(_sorted2.begin(), _sorted2.end(), compare_segment_x_smaller2);
	//}
	// 
	//for (Segment p : _sorted2)
	//{

	//	if (top_left.x > p.bottom_right.x)
	//		break;

	//	counter[1]++;

	//	if (!(geometry::rectangle_outside(top_left, bottom_right, p.top_left, p.bottom_right) || geometry::rectangle_outside(p.top_left, p.bottom_right, top_left, bottom_right)))
	//		result.push_back(p);
	//}
	//return _sorted;
	
	// Second method, a quadtree is created with bounds which is divided into four areas
	// then each segment is classified following the 1/4 bounds
	// a tree of quad bounds is generated
	// searching will then be a tree branch comparison
	if (!_tree.created())
	{
		// float precision does not allow to divide more, deeper nodes would only be copies of their parents
		_tree.create(this->bounds(), 0, glm::min<int>(16, (int)size() / 4));
		for (Segment p : *this)
			_tree.add(p, p.bounds);
	}

	return _tree.search(bounds);
}

void Curve::sort_level(std::vector<Curve>& curves)
{
	std::sort(curves.begin(), curves.end(), compare_curve_level);
}

// ---------------------------------------------------------------------------------------------------------------------
// Robust boolean operations between two closed curves, same principle as split_at_intersections(int) : the two curves are cut at
// their intersections, each sub edge is kept or not following where it lies relatively to the other curve, and the edges which are
// kept are chained into closed curves.
// Returns false if the operation can not be done this way (edges lying on the other curve, edges which can not be chained...), the
// caller then uses the previous implementation.
// ---------------------------------------------------------------------------------------------------------------------
bool Curve::robust_boolean(Curve& other, bool same_direction, bool outside, std::vector<Curve>& result)
{
	// the not intersection operation is not available
	if (!same_direction && !outside)
		return false;

	for (const Segment& s : *this)
		if (s.type == SegmentType::Circle)
			return false;
	for (const Segment& s : other)
		if (s.type == SegmentType::Circle)
			return false;

	if (size() < 3 || other.size() < 3)
		return false;

	// curves which bounds do not touch can not intersect, the previous implementation decides (one inside the other, or apart)
	{
		geometry::rectangle& r1 = bounds();
		geometry::rectangle& r2 = other.bounds();
		const float margin = 0.01f;
		if (glm::max(r1.top_left.x, r1.bottom_right.x) + margin < glm::min(r2.top_left.x, r2.bottom_right.x)
			|| glm::max(r2.top_left.x, r2.bottom_right.x) + margin < glm::min(r1.top_left.x, r1.bottom_right.x)
			|| glm::max(r1.top_left.y, r1.bottom_right.y) + margin < glm::min(r2.top_left.y, r2.bottom_right.y)
			|| glm::max(r2.top_left.y, r2.bottom_right.y) + margin < glm::min(r1.top_left.y, r1.bottom_right.y))
			return false;
	}

	// the result has the direction of the first curve, the second one is turned to be in the right direction relatively to it
	// the bounds of both curves have just been computed, they are copied with them : only a reversed curve needs them again
	Curve a = *this, b = other;
	const bool a_cw = a.cw();
	if (b.cw() != (same_direction ? a_cw : !a_cw))
	{
		b.reverse();
		b.reset_bounds();
	}

	const int orientation = a_cw ? -1 : 1;
	const int na = (int)a.size(), nb = (int)b.size();

	auto dst_a = [&](int i) { return i == na - 1 ? a[0].point : a[i + 1].point; };
	auto dst_b = [&](int i) { return i == nb - 1 ? b[0].point : b[i + 1].point; };
	auto real_a = [&](int i) { return a[i].point != dst_a(i); };
	auto real_b = [&](int i) { return b[i].point != dst_b(i); };

	// 1. intersections between the two curves
	std::vector<glm::vec2> points;
	for (int i = 0; i < na; i++)
		points.push_back(a[i].point);
	for (int i = 0; i < nb; i++)
		points.push_back(b[i].point);
	const int offset_b = na;

	std::vector<std::vector<Hit>> hits_a(na), hits_b(nb);
	int hit_count = 0;

	for (int i = 0; i < na; i++)
	{
		if (!real_a(i))
			continue;

		Segment s1 = a[i];
		glm::vec2 dst1 = dst_a(i);

		auto candidates = b.search(s1.bounds);
		for (size_t k = 0; k < candidates.size(); k++)
		{
			int j = candidates[k].index;
			if (!real_b(j))
				continue;

			Segment s2 = b[j];
			glm::vec2 dst2 = dst_b(j);

			int count = 0;
			glm::vec2 pp[2];

			if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
			{
				glm::vec2 p = glm::vec2();
				if (geometry::segment_segment_intersect(s1.point, dst1, s2.point, dst2, p))
				{
					pp[0] = p;
					count = 1;
				}
			}
			else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
				count = geometry::segment_arc_intersect(s1.point, dst1, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
				count = geometry::segment_arc_intersect(s2.point, dst2, s1.point, s1.center, dst1, s1.radius, s1.cw, pp);
			else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
				count = geometry::arc_arc_intersect(s1.point, s1.center, dst1, s1.radius, s1.cw, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);

			for (int c = 0; c < count; c++)
			{
				glm::dvec2 exact = refine_intersection(s1, dst1, s2, dst2, pp[c]);
				int node = (int)points.size();
				points.push_back(pp[c]);
				hits_a[i].push_back({ position_along(s1, dst1, exact), pp[c], node });
				hits_b[j].push_back({ position_along(s2, dst2, exact), pp[c], node });
				hit_count++;
			}
		}
	}

	// without intersection, the previous implementation decides (the curves can be one inside the other, or apart)
	if (hit_count == 0)
		return false;

	// 2. nodes
	const float eps = 2 * geometry::ERR_FLOAT3;
	const float eps2 = eps * eps;
	std::vector<int> parent(points.size());
	for (size_t i = 0; i < parent.size(); i++)
		parent[i] = (int)i;
	for (size_t i = 0; i < points.size(); i++)
		for (size_t j = i + 1; j < points.size(); j++)
		{
			glm::vec2 d = points[i] - points[j];
			if (d.x * d.x + d.y * d.y <= eps2)
			{
				int ri = find_root(parent, (int)i), rj = find_root(parent, (int)j);
				if (ri != rj)
					parent[std::max(ri, rj)] = std::min(ri, rj);
			}
		}
	auto node_of = [&](int point_index) { return find_root(parent, point_index); };

	// 3. sub edges of both curves
	struct BoolEdge
	{
		int owner;		// 0 : first curve, 1 : second curve
		int segment;
		int from, to;
		glm::vec2 p1, p2;
		bool keep = false;
		glm::dvec2 tangent_start, tangent_end;
	};
	std::vector<BoolEdge> edges;

	auto cut = [&](Curve& c, int owner, int n, int point_offset, std::vector<std::vector<Hit>>& hits)
	{
		for (int i = 0; i < n; i++)
		{
			glm::vec2 dst = i == n - 1 ? c[0].point : c[i + 1].point;
			if (c[i].point == dst)
				continue;

			std::sort(hits[i].begin(), hits[i].end(), [](const Hit& h1, const Hit& h2) { return h1.key < h2.key; });

			struct Stop { int node; glm::vec2 point; };
			std::vector<Stop> stops;
			stops.push_back({ node_of(point_offset + i), c[i].point });
			for (const Hit& h : hits[i])
				stops.push_back({ node_of(h.node), h.point });
			stops.push_back({ node_of(point_offset + (i == n - 1 ? 0 : i + 1)), dst });

			for (size_t k = 0; k + 1 < stops.size(); k++)
			{
				BoolEdge e;
				e.owner = owner;
				e.segment = i;
				e.from = stops[k].node;
				e.to = stops[k + 1].node;
				e.p1 = stops[k].point;
				e.p2 = stops[k + 1].point;
				edges.push_back(e);
			}
		}
	};
	cut(a, 0, na, 0, hits_a);
	cut(b, 1, nb, offset_b, hits_b);

	// 4. each edge is classified following the other curve
	Winding winding_a, winding_b;
	for (int i = 0; i < na; i++)
		if (real_a(i))
			winding_a.add(a[i], dst_a(i));
	for (int i = 0; i < nb; i++)
		if (real_b(i))
			winding_b.add(b[i], dst_b(i));

	const double delta = 2e-4;
	bool coincident = false;

	for (BoolEdge& e : edges)
	{
		const Segment& s = e.owner == 0 ? a[e.segment] : b[e.segment];
		glm::dvec2 mid, tangent;
		double length;

		if (s.type == SegmentType::Line)
		{
			glm::dvec2 p1(e.p1), p2(e.p2), d = p2 - p1;
			length = glm::length(d);
			if (length == 0)
				continue;
			tangent = d / length;
			mid = (p1 + p2) / 2.0;
			e.tangent_start = e.tangent_end = tangent;
		}
		else
		{
			glm::dvec2 c(s.center), p1(e.p1), p2(e.p2);
			double r = glm::length(p1 - c);
			double a1 = std::atan2(p1.y - c.y, p1.x - c.x), a2 = std::atan2(p2.y - c.y, p2.x - c.x);
			double sweep = s.cw ? norm_angle(a1 - a2) : norm_angle(a2 - a1);
			length = r * sweep;
			if (sweep < 1e-12)
				continue;
			double am = s.cw ? a1 - sweep / 2 : a1 + sweep / 2;
			mid = c + glm::dvec2(std::cos(am), std::sin(am)) * r;
			tangent = s.cw ? glm::dvec2(std::sin(am), -std::cos(am)) : glm::dvec2(-std::sin(am), std::cos(am));
			e.tangent_start = s.cw ? glm::dvec2(std::sin(a1), -std::cos(a1)) : glm::dvec2(-std::sin(a1), std::cos(a1));
			e.tangent_end = s.cw ? glm::dvec2(std::sin(a2), -std::cos(a2)) : glm::dvec2(-std::sin(a2), std::cos(a2));
		}

		if (e.from == e.to && length < 3 * eps)
			continue;

		glm::dvec2 left(-tangent.y, tangent.x);
		const Winding& other_winding = e.owner == 0 ? winding_b : winding_a;

		// far from the other curve, there is nothing to count
		geometry::rectangle& ob = e.owner == 0 ? b.bounds() : a.bounds();
		const double margin = 1e-2;
		if (mid.x < glm::min(ob.top_left.x, ob.bottom_right.x) - margin || mid.x > glm::max(ob.top_left.x, ob.bottom_right.x) + margin
			|| mid.y < glm::min(ob.top_left.y, ob.bottom_right.y) - margin || mid.y > glm::max(ob.top_left.y, ob.bottom_right.y) + margin)
		{
			e.keep = same_direction ? outside : e.owner == 0;
			continue;
		}

		bool inside_left = other_winding.at(mid + left * delta) != 0;
		bool inside_right = other_winding.at(mid - left * delta) != 0;

		// the edge is on the other curve : the two sides are not on the same side of it
		if (inside_left != inside_right)
		{
			coincident = true;
			break;
		}

		bool inside_other = inside_left;
		if (same_direction)
			e.keep = outside ? !inside_other : inside_other;			// union, intersection
		else
			e.keep = e.owner == 0 ? !inside_other : inside_other;		// substraction : the first curve outside, the second one inside
	}

	if (coincident)
		return false;

	// a kept edge which starts and ends on the same node is a whole circle touching the other curve at one point : the pinched result
	// can not be handled by the following operations, the previous implementation is used
	for (const BoolEdge& e : edges)
		if (e.keep && e.from == e.to)
			return false;

	// 5. chain the edges
	std::vector<std::vector<int>> outgoing(points.size());
	for (size_t k = 0; k < edges.size(); k++)
		if (edges[k].keep)
			outgoing[edges[k].from].push_back((int)k);

	std::vector<bool> used(edges.size(), false);
	std::vector<Curve> chained;

	for (size_t k = 0; k < edges.size(); k++)
	{
		if (!edges[k].keep || used[k])
			continue;

		std::vector<int> loop;
		int current = (int)k;
		int start = edges[k].from;

		while (true)
		{
			used[current] = true;
			loop.push_back(current);
			int node = edges[current].to;
			if (node == start)
				break;

			int best = -1;
			double best_turn = 0;
			for (int candidate : outgoing[node])
			{
				if (used[candidate])
					continue;
				glm::dvec2 t1 = edges[current].tangent_end, t2 = edges[candidate].tangent_start;
				double turn = std::atan2(t1.x * t2.y - t1.y * t2.x, glm::dot(t1, t2));
				if (best == -1 || (orientation > 0 ? turn > best_turn : turn < best_turn))
				{
					best = candidate;
					best_turn = turn;
				}
			}

			if (best == -1)
				return false;
			current = best;
		}

		Curve c;
		for (int index : loop)
		{
			const BoolEdge& e = edges[index];
			const Segment& s = e.owner == 0 ? a[e.segment] : b[e.segment];
			glm::vec2 from = points[e.from];
			if (s.type == SegmentType::Line)
				c.add(from);
			else
				c.add(from, s.center, s.radius, s.cw);
		}
		c.close();
		c.reset_bounds();
		chained.push_back(c);
	}

	result = chained;
	return true;
}

std::vector<Curve> Curve::weiler(Curve& b, bool same_direction, bool outside)
{
	std::vector<Curve> result;

	// the robust way first, the curves are cut at their intersections and the pieces are chained, whatever the order of the intersections
	if (robust_boolean(b, same_direction, outside, result))
		return result;
	result.clear();
	// same method as self intersection
	Curve& a = *this;

	a.length();
	b.length();

	if (a.bounds().outside(b.bounds()))
		return result;

	// a curve with less than 3 points (except the circles) is not an area, and the walk below needs a closed curve
	auto degenerate = [](Curve& c)
	{
		if (c.size() >= 3)
			return false;
		bool circle = c.size() > 0 && (c.front().type == SegmentType::Circle || (c.front().type == SegmentType::Arc && c.size() > 1 && c[1].point == c.front().point));
		return !circle;
	};
	if (degenerate(a) || degenerate(b))
		return result;

	bool a_is_circle = false, b_is_circle = false;

	if (a[0].type == SegmentType::Circle || (a[0].type == SegmentType::Arc && a.size() > 1 && a[1].point == a[0].point))
	{
		a[0].type = SegmentType::Arc;
		a.reset_bounds();
		a_is_circle = true;
	}
	else if (a.back().point == a.front().point)
	{
		a.pop_back();
		a.reset_bounds();
	}


	if (b[0].type == SegmentType::Circle || (b[0].type == SegmentType::Arc && b.size() > 1 && b[1].point == b[0].point))
	{
		b[0].type = SegmentType::Arc;
		b.reset_bounds();
		b_is_circle = true;
	}
	else if (b.back().point == b.front().point)
	{
		b.pop_back();
		b.reset_bounds();
	}

	if (same_direction)
		b.cw(a.cw());
	else
		b.cw(!a.cw());

	// if bounds are outside, there is no intersection
	if (!a.bounds().outside(b.bounds()))
	{
		std::vector<int> indices_a, indices_b; // hold indices of 
		std::vector<std::vector<Intersection>> intersections_a, intersections_b;

		// we construct an array to store intersection points for each index
		intersections_a.reserve(a.size());
		for (int i = 0; i < a.size(); i++)
			intersections_a.push_back(std::vector<Intersection>());

		intersections_b.reserve(b.size());
		for (int i = 0; i < b.size(); i++)
			intersections_b.push_back(std::vector<Intersection>());

		int tag = 0;
		counter[5] = 0;

		// we construct an array to store intersection points for each index
		// a segment can have several intersections
		// so we will push into the list associated to the segment every intersections detected
		// two curves which are not lying on each other have a few intersections. When there are way more than the number of segments, the
		// curves are on each other (touching offsets) and the intersections can not be linked : there is nothing to merge, and the search
		// stops here because the number of intersections can reach the millions
		const int max_tag = 4 * (int)(a.size() + b.size()) + 64;

		for (int i = 0; i < a.size() && tag <= max_tag; i++)
		{
			Segment s1 = a[i];
			glm::vec dst1 = (i == a.size() - 1)? a[0].point : a[i + 1].point;

			auto candidates = b.search(s1.bounds);

			//for (int j = i + 1; j < a.size(); j++)
			for (int k = 0; k < candidates.size() && tag <= max_tag; k++)
			{
				int j = candidates[k].index;

				Segment s2 = b[j];
				glm::vec2 dst2 = (j == b.size() - 1) ? dst2 = b[0].point : dst2 = b[j + 1].point;

				int count = 0;
				glm::vec2 pp[2];

				if (s1.type == SegmentType::Line && s2.type == SegmentType::Line)
				{
					glm::vec2 p = glm::vec2();
					if (geometry::segment_segment_intersect(s1.point, dst1, s2.point, dst2, p))
					{
						pp[0] = p;
						count = 1;
					}
				}
				else if (s1.type == SegmentType::Line && s2.type == SegmentType::Arc)
				{
					count = geometry::segment_arc_intersect(s1.point, dst1, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
				}
				else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Line)
				{
					count = geometry::segment_arc_intersect(s2.point, dst2, s1.point, s1.center, dst1, s1.radius, s1.cw, pp);
				}
				else if (s1.type == SegmentType::Arc && s2.type == SegmentType::Arc)
				{
					count = geometry::arc_arc_intersect(s1.point, s1.center, dst1, s1.radius, s1.cw, s2.point, s2.center, dst2, s2.radius, s2.cw, pp);
				}

				for (int c = 0; c < count; c++)
				{
					intersections_a[i].push_back(Intersection(i, j, pp[c], tag));
					intersections_b[j].push_back(Intersection(j, i, pp[c], tag));
					if (std::find(indices_a.begin(), indices_a.end(), i) == indices_a.end())
						indices_a.push_back(i);
					if (std::find(indices_b.begin(), indices_b.end(), j) == indices_b.end())
						indices_b.push_back(j);
					tag++;
				}
			}
		}

		if (tag > max_tag)
		{
			if (a_is_circle)
				a[0].type = SegmentType::Circle;
			if (b_is_circle)
				b[0].type = SegmentType::Circle;
			return std::vector<Curve>();
		}

		// we have segments in indices and its intersections coordinates in intersections
		// we will sort for each segment its intersections
		// sorting will order same segment intersections along the segment. 
		// so if the segment [a,b] has 3 intersections d,e,c we order them as first is the closest to [a], etc, to get [a, c, d, e, b]
		// for line segment, we use 'y' coordinates if vertical, 'x' coordinate otherwise
		// for arcs, we use the angle between [a] and intersection ref_point
		// so we go through 'indices' which hold where intersections have occured to avoid looking the entire 'intersections' list
		Curve* c = &a;
		std::vector<int> indices = indices_a;
		std::vector<std::vector<Intersection>> intersections = intersections_a;
		int pass = 0;
		if (indices.size() > 0)
		{
			float _3_4_pi = 3 * glm::quarter_pi<float>();
			float _5_4_pi = 5 * glm::quarter_pi<float>();
			float _7_4_pi = 7 * glm::quarter_pi<float>();

			do
			{
				// we have segments in indices and its intersections coordinates in intersections
				// we will sort for each segment its intersections
				for (int i = 0; i < indices.size(); i++)
				{
					if (intersections[indices[i]].size() > 1)
					{
						Segment s1 = (*c)[indices[i]];
						Segment s2 = (*c)[indices[i] == (*c).size() - 1 ? 0 : indices[i] + 1];

						if (s1.type == SegmentType::Line)
						{
							//if (s1.ref_point.x == s2.ref_point.x) // vertical
							auto a = geometry::oriented_angle(s2.point, s1.point);
							if (a > glm::quarter_pi<float>() && a < _3_4_pi || a > _5_4_pi && a < _7_4_pi)
							{
								if (s1.point.y < s2.point.y)
									std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_y_smaller);
								else
									std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_y_greater);
							}
							else
							{
								if (s1.point.x < s2.point.x)
									std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_x_smaller);
								else
									std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_x_greater);
							}
						}
						else // for arcs, we have to sort following the oriented angle
						{
							auto a1 = geometry::oriented_angle(s1.point, s1.center);
							float at = 0;
							if (s1.cw)
							{
								for (Intersection& inter : intersections[indices[i]])
								{
									inter.a = geometry::oriented_angle(inter.point, s1.center);
									if (inter.a > a1)
										inter.a -= glm::two_pi<float>();
								}
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_a_greater);
							}
							else
							{
								for (Intersection& inter : intersections[indices[i]])
								{
									inter.a = geometry::oriented_angle(inter.point, s1.center);
									if (inter.a < a1)
										inter.a += glm::two_pi<float>();
								}
								std::sort(intersections[indices[i]].begin(), intersections[indices[i]].end(), compare_intersection_a_lower);
							}
						}
					}
				}

				// we swap to sort the b curve intersections
				if (pass == 0)
				{
					c = &b;
					indices = indices_b;
					intersections_a = intersections;
					intersections = intersections_b;
				}
				else
					intersections_b = intersections;
				pass++;

			} while (pass != 2);
		}
		else
		{
			// we restore circle type
			if (a_is_circle)
				a[0].type = SegmentType::Circle;
			if (b_is_circle)
				b[0].type = SegmentType::Circle;
			return result;
		}

		// we have to loop into the curve and intersections to create the new curves
		// we will construct a new curve with splitted intersected segments 
		// then we link into segment's tag  with the intersected segment
		// then we are able to go throught the curve and split it into several curves

		std::map<int, std::list<Segment>::iterator> tags;
		std::list<Segment> b_a, b_b;
		pass = 0;
		c = &a;
		std::list<Segment>* l = &b_a;
		intersections = intersections_a;

		while (pass != 2)
		{
			for (int i = 0; i < c->size(); i++)
			{
				Segment& s = (*c)[i];
				s.tag = (int)(l->size() + ((l == &b_a) ? 0 : b_a.size() + 1));
				s.next_tag = s.tag + 1;
				l->push_back(s);
				auto cnt = intersections[i].size();
				if (intersections[i].size() > 0)
				{
					for (int j = 0; j < intersections[i].size(); j++)
					{
						l->push_back(Segment(intersections[i][j].point, s.center, s.radius, s.cw, s.type, (int)(l->size() + ((l == &b_a) ? 0 : b_a.size() + 1)), s.excluded));
						auto it = --l->end(); // last segment iterator is end() - 1
						(*it).next_tag = (*it).tag + 1;

						// we look for existing key, if true, then we set reference to "next" field for both
						// else we inject in tags list this one

						// but as the intersection is done with another segment, we need to keep a reference on it
						// this is the role of tags list. we store this new segment reference 'it' to the intersection tag (not related to segment.tag we so earlier)
						// the intersection tag refers to both segment that intersect
						// if tag is already in tags, this means we are dealing with the second segment
						// so we can then create the reference which is as follow, segment s1 and segment s2 :
						// first = iterator(s1) - 1                     c\      /b                     
						// first.next = iterator(s2)                      \    /                    it(s1)-1 [a;i]
						// first.next_tag = s2.tag      iterator(s2)-1     \  /						it(s1)   [i;b]
						// second = iterator(s2) - 1			           i\/    iterator(s1)		it(s2)-1 [c;i]
						// second.next = s1.tag                             /\						it(s2)   [i;d]
						// second.next_tag = s1.next    iterator(s1) - 1  a/  \d  iterator(s2)

						if (tags.find(intersections[i][j].tag) == tags.end())
						{
							tags[intersections[i][j].tag] = it; // last segment iterator is end() - 1
						}
						else
						{
							auto first = (tags[intersections[i][j].tag] == b_a.begin()) ? --b_a.end() : std::prev(tags[intersections[i][j].tag]);
							(*first).next = it;
							(*first).next_tag = (*it).tag;

							auto second = (it == b_b.begin()) ? --b_b.end() : std::prev(it);
							(*second).next = tags[intersections[i][j].tag];
							(*second).next_tag = (*(*second).next).tag;

							tags.erase(intersections[i][j].tag);
						}
					}
				}
			}

			pass++;

			if (pass == 1)
			{
				c = &b;
				l = &b_b;
				intersections = intersections_b;
			}
		}

		// the segments which are removed can still be referenced by the links between the intersections : they are tagged as used
		std::set<const Segment*> used;

		if (a_is_circle)
		{
			used.insert(&b_a.back());
			b_a.pop_back();
		}

		if (b_a.back().next_tag == b_a.back().tag + 1)
			b_a.back().next_tag = b_a.front().tag;

		if (b_is_circle)
		{
			used.insert(&b_b.back());
			b_b.pop_back();
		}

		if (b_b.back().next_tag == b_b.back().tag + 1)
			b_b.back().next_tag = b_b.front().tag;

		int tag_limit = b_b.front().tag - 1;

		std::map<const Segment*, int> owner;
		for (const Segment& s : b_a)
			owner[&s] = 0;
		for (const Segment& s : b_b)
			owner[&s] = 1;

		// we have the curve with the intersection tag in segment tag, we have now to convert the intersection tag to curve index
		while (b_a.size() > 0 || b_b.size() > 0)
		{
			Curve c;
			bool excluded = false;
			bool exit = false, first_segment_from_ba = true;

			std::list<Segment>::iterator it, next;
			Segment first = Segment(geometry::vec2_empty);
			if (b_a.size() > 0)
			{
				it = b_a.begin();
				next = it;
				first = *it;
			}
			else
			{
				it = b_b.begin();
				next = it;
				first = *it;
				first_segment_from_ba = false;
			}

			// segments which are already used. The intersections of curves which touch each other can make the walk reach a segment that
			// was used before, or the segment it is on : the walk is then abandoned instead of reading a segment that does not exist anymore
			bool broken = false;

			do
			{
				c.push_back(*it);

				// the list which holds the segment : it is not always the one the tag tells when intersections are very close
				std::list<Segment>& own = owner[&*it] == 0 ? b_a : b_b;

				if ((*it).next_tag != first.tag)
				{
					if ((*it).tag < tag_limit && (*it).next_tag > tag_limit || (*it).tag > tag_limit && (*it).next_tag < tag_limit)
					{
						next = (*it).next;
					}
					else
					{
						next = std::next(it);
						if (next == own.end())
							next = own.begin();
					}

					// (the addresses are compared: next can be in the other list, and the debug library forbids comparing
					// the iterators of two lists)
					if (&*next == &*it || used.count(&*next) > 0)
						broken = true;

					used.insert(&*it);
					own.erase(it);

					if (broken)
						break;
					it = next;
				}
				else
				{
					used.insert(&*it);
					own.erase(it);
					exit = true;
				}

			} while (exit == false && (*it).tag != first.tag);

			if (broken)
				continue;

			if (c.size() > 1)
			{
				glm::vec2 dst;
				if (c[0].type == SegmentType::Line)
					dst = geometry::middle(c[0].point, c[1].point);
				else
					dst = geometry::arc_middle(c[0].point, c[0].center, c[1].point, c[0].cw);

				//bool append = false;
				//if (same_direction)
				//{
				//	if (outside) // union
				//	{

				//	}
				//	else // intersection
				//	{

				//	}
				//}
				//else
				//{
				//	if (outside) // substract
				//	{

				//	}
				//	else // not intersection
				//	{

				//	}
				//}
				
				if ( !((first_segment_from_ba && outside == b.inside(dst)) || (!first_segment_from_ba && outside == a.inside(dst))) )
				//if (append)
				{
					c.reset_bounds();
					result.push_back(c);
				}
			}
		}
	}

	// we restore circle type
	if (a_is_circle)
		a[0].type = SegmentType::Circle;
	if (b_is_circle)
		b[0].type = SegmentType::Circle;

	std::sort(result.begin(), result.end(), compare_curve_level);

	return result;
}

void TreeCurve::add(TreeCurve* c)
{
	children.push_back(c);
}

void TreeCurve::sort()
{
	if (children.size() > 1)
	{
		std::sort(children.begin(), children.end(), compare_curve_size);
		auto it1 = children.begin();
		while (it1 != children.end() && children.size() > 1)
		{
			bool inc_it1 = true;
			for (auto it2 = it1 + 1; it2 != children.end(); ++it2)
			{
				if ((*it1)->curve != (*it2)->curve && !(*it2)->curve->bounds().outside((*it1)->curve->bounds()))
				{
					if ((*it2)->curve->inside(*((*it1)->curve)))
					{
						(*it1)->add(*it2);
						auto prev = std::prev(it2);
						children.erase(it2);
						it2 = prev;
					}
					else if ((*it1)->curve->inside(*((*it2)->curve)))
					{
						(*it2)->add(*it1);
						if (it1 != children.begin())
						{
							auto prev = std::prev(it1);
							children.erase(it1);
							it1 = prev;
							
						}
						else
						{
							children.erase(it1);
							it1 = children.begin();
							inc_it1 = false;
						}
						break;
					}
				}
			}
			if (inc_it1)
				++it1;
		}

		std::sort(children.begin(), children.end(), compare_treecurve_tag);

		for (TreeCurve* c : children)
			c->sort();
	}
}

void TreeCurve::cw_all(bool direction)
{
	if (curve->size() > 0)
		curve->cw(direction);

	for (TreeCurve* c : children)
	{
		c->cw_all(direction);
	}
}

void TreeCurve::cw_alter()
{
	for (TreeCurve* c : children)
	{
		c->level = level + 1;
		c->cw_alter(c->curve->cw());
	}
}

void TreeCurve::cw_alter(bool direction)
{
	if (curve != nullptr && curve->size() > 0)
	{
		curve->cw(direction);
		curve->level(level);
	}
	for (TreeCurve* c : children)
	{
		c->level = level + 1;
		c->cw_alter(!direction);
	}
}

TreeCurve::TreeCurve(Curve* c)
{
	curve = c;
}

TreeCurve::TreeCurve(std::vector<Curve>& curves)
{
	for (Curve& c : curves)
	{
		Curve* curve = new Curve();
		curve->tag(c.tag());
		curve->reference(c.reference());
		for (Segment& s : c)
			curve->add(s.type, s.point, s.center, s.radius, s.cw);

		children.push_back(new TreeCurve(curve));
	}
}

TreeCurve::~TreeCurve()
{
	if (curve != nullptr)
	{
		curve->clear();
		delete curve;
		curve = nullptr;
	}
	for (TreeCurve* c : children)
		delete c;
	children.clear();
}

size_t TreeCurve::size()
{
	size_t count = children.size();
	for (TreeCurve* c : children)
		count += c->size();
	return count;
}

std::vector<Curve> TreeCurve::curves()
{
	std::vector<Curve> result;
	if (curve != nullptr && curve->size() > 0)
		result.push_back(*curve);
	for (TreeCurve* c : children)
	{
		auto list = c->curves();
		result.insert(result.end(), list.begin(), list.end());
	}

	std::sort(result.begin(), result.end(), compare_curve_level);

	for (int i = 0; i < result.size(); i++)
		result[i].index(i);

	return result;
}
