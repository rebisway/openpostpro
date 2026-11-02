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
		a = length / radius;
		a1 = geometry::oriented_angle(point, dst, center, cw);
		if (cw)
			a = geometry::oriented_angle(a1 - a);
		else
			a = geometry::oriented_angle(a1 + a);
		return glm::vec2(glm::cos(a), glm::sin(a)) + center;
	case SegmentType::Circle:
		a = length / radius;
		return glm::vec2(glm::cos(a), glm::sin(a)) + center;
	}

	return glm::vec2();
}

std::vector<Segment> Segment::split_at(float length, glm::vec2 dst)
{
	std::vector<Segment> result;
	switch (type)
	{
	case SegmentType::Line:
		auto p = geometry::position(dst, length, point);
		result.push_back(*this);
		result.push_back(Segment(p));
		break;
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


int Curve::counter[10];

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

		for (int i = 0; i < size() - 1; i++)
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
				// we compute area : if odd then CCW else CW
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

		std::vector<glm::vec2> points;

		if (size() == 1 && (*this)[0].type == SegmentType::Circle)
			return _area = (*this)[0].radius * glm::two_pi<float>();

		Segment from = front();
		auto it = begin() + 1;

		points.push_back(from.point);

		while (it != end())
		{
			if (from.type == SegmentType::Line)
			{
				points.push_back((*it).point);
			}
			else
			{
				auto arc = geometry::arc(from.point, from.center, (*it).point, from.cw);
				points.insert(points.end(), arc.begin() + 1, arc.end());
			}
			from = (*it);
			it++;
		}

		if (front().point != back().point)
			points.push_back(front().point);

		_area = 0;
		glm::vec2 f = points.front();
		auto pit = points.begin() + 1;
		while (pit != points.end())
		{
			_area += (f.x * (*pit).y) - ((*pit).x * f.y);
			f = (*pit);
			pit++;
		}

		_area /= 2;
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

	auto splitted = trimmed.split_at_intersections();

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
			cw_found = !c.cw();
		}
	}

	for (Curve& c : splitted)
	{
		// if outside offset or inside and offset is smaller than current
		if ((cross_found && c.cw() == cw_found || !cross_found) && (o > 0 || (!outside && c.bounds().width() * c.bounds().height() < _bounds.width() * _bounds.height())))
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
						if (radius > 0) 
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

		for (int i = 0; i < a.size() - 1; i++)
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


std::vector<Curve> Curve::boolean_union(Curve& b)
{
	// union means both curve have same direction
	return weiler(b, true, true);
}

std::vector<Curve> Curve::boolean_substract(Curve& b)
{
	// substration means curves direction are opposit
	return weiler(b, false, true);
}

std::vector<Curve> Curve::boolean_intersect(Curve& b)
{
	return weiler(b, true, false);
}

std::vector<Curve> Curve::boolean_not_intersect(Curve& b)
{
	return weiler(b, false, false);
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
		_tree.create(this->bounds(), 0, glm::min<int>(255, (int)size() / 4));
		for (Segment p : *this)
			_tree.add(p, p.bounds);
	}

	return _tree.search(bounds);
}

void Curve::sort_level(std::vector<Curve>& curves)
{
	std::sort(curves.begin(), curves.end(), compare_curve_level);
}

std::vector<Curve> Curve::weiler(Curve& b, bool same_direction, bool outside)
{
	std::vector<Curve> result;
	// same method as self intersection
	Curve& a = *this;

	a.length();
	b.length();

	if (a.bounds().outside(b.bounds()))
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
		for (int i = 0; i < a.size(); i++)
		{
			Segment s1 = a[i];
			glm::vec dst1 = (i == a.size() - 1)? a[0].point : a[i + 1].point;

			auto candidates = b.search(s1.bounds);

			//for (int j = i + 1; j < a.size(); j++)
			for (int k = 0; k < candidates.size(); k++)
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

		if (a_is_circle)
			b_a.pop_back();

		if (b_a.back().next_tag == b_a.back().tag + 1)
			b_a.back().next_tag = b_a.front().tag;

		if (b_is_circle)
			b_b.pop_back();

		if (b_b.back().next_tag == b_b.back().tag + 1)
			b_b.back().next_tag = b_b.front().tag;

		int tag_limit = b_b.front().tag - 1;

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

			do
			{
				c.push_back(*it);

				if ((*it).next_tag != first.tag)
				{
					if ((*it).tag < tag_limit && (*it).next_tag > tag_limit || (*it).tag > tag_limit && (*it).next_tag < tag_limit)
					{
						next = (*it).next;
					}
					else
					{
						if ((*it).tag < tag_limit && it == (--b_a.end()))
							next = b_a.begin();
						else if ((*it).tag >= tag_limit && it == (--b_b.end()))
							next = b_b.begin();
						else
							next = std::next(it);
					}

					if ((*it).tag < tag_limit)
						b_a.erase(it);
					else
						b_b.erase(it);
					it = next;
				}
				else
				{
					if ((*it).tag < tag_limit)
						b_a.erase(it);
					else
						b_b.erase(it);
					exit = true;
				}

			} while (exit == false && (*it).tag != first.tag);

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
