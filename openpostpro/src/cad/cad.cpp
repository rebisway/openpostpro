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

#include <cad.h>
#include <line.h>
#include <circle.h>
#include <algorithm>
#include <array>
#include <functional>
#include <arc.h>
#include <polyline.h>
#include <ellipse.h>
#include <logger.h>
#include <text.h>


bool compare_vec2_x_smaller(glm::vec2 v1, glm::vec2 v2)
{
	return (v1.x < v2.x);
}

bool compare_vec2_x_greater(glm::vec2 v1, glm::vec2 v2)
{
	return (v1.x > v2.x);
}

bool compare_vec2_y_smaller(glm::vec2 v1, glm::vec2 v2)
{
	return (v1.y < v2.y);
}

bool compare_vec2_y_greater(glm::vec2 v1, glm::vec2 v2)
{
	return (v1.y > v2.y);
}

struct segment
{
	glm::vec2 vec1;
	glm::vec2 vec2;
	int index;

	segment() {
		vec1 = vec2 = geometry::vec2_empty;
		index = -1;
	}

	segment(glm::vec2 &v1, glm::vec2 &v2, int i)
	{
		vec1 = v1;
		vec2 = v2;
		index = i;
	}
};

bool compare_vector_x_greater(segment v1, segment v2)
{
	return (v1.vec1.x > v2.vec1.x);
}

bool compare_vector_x_smaller(segment v1, segment v2)
{
	return (v1.vec1.x < v2.vec1.x);
}

/// <summary>
/// generate a segment list with vec1 get always the lower x coordinates, then the list is sorted following vec1.x from lower to greater
/// </summary>
/// <param name="list"></param>
/// <returns></returns>
std::vector<segment> sort_vec2_x_smaller(std::vector<glm::vec2> list)
{
	std::vector<segment> result;
	result.reserve(list.size());
	for (int i = 0; i + 1 < list.size(); i++)
	{
		auto& v1 = list[i];
		auto& v2 = list[i+1];
		if (v1.x < v2.x)
		{
			result.push_back(segment(v1, v2, i));
		}
		else
		{
			result.push_back(segment(v2, v1, i));
		}
	}

	std::sort(result.begin(), result.end(), compare_vector_x_smaller);
	return result;
}

/// <summary>
/// return every segments that contains p
/// </summary>
/// <param name="list"></param>
/// <param name="p"></param>
/// <returns></returns>
std::vector<segment> get_segments(std::vector<segment>& list, glm::vec2 p)
{
	std::vector<segment> result;

	// we use diccotomie
	int pivot = (int)list.size() / 2, left = 0, right = (int)list.size();
	bool done = false;
	while (!done)
	{
		if (list[pivot].vec1.x > p.x)
		{
			// left
			right = pivot;
			pivot = left + (pivot - left) / 2;
		}
		else
		{
			left = pivot;
			pivot = right - (right - pivot) / 2;
			// right
		}

		if (/*pivot == left || pivot == left + 1 ||*/ pivot == right || pivot == right - 1)
		{
			done = true;
		}
	}

	// we have excluded every segments where vec1.x > p.x
	// we now have to test if vec2.x > p.x

	while ((pivot >= list.size() || list[pivot].vec1.x > p.x) && pivot > 0)
		pivot--;
	for (int i = pivot; i >= 0; i--)
	{
		if (list[i].vec2.x >= p.x)
			result.push_back(list[i]);
	}

	return result;
}

std::vector<Shape*> cad::split(Shape* shape, std::vector<glm::vec2> points)
{
	std::vector<Shape*> result;

	if (shape->type() == GraphicType::Line)
	{
		Line* s = (Line*)shape;
		
		// we sort following x coordinates
		if (s->p1().x == s->p2().x) // vertical
		{
			if (s->p1().y < s->p2().y)
				std::sort(points.begin(), points.end(), compare_vec2_y_smaller);
			else
				std::sort(points.begin(), points.end(), compare_vec2_y_greater);
		}
		else
		{
			if (s->p1().x < s->p2().x)
				std::sort(points.begin(), points.end(), compare_vec2_x_smaller);
			else
				std::sort(points.begin(), points.end(), compare_vec2_x_greater);
		}

		glm::vec2 from = s->p1();
		points.push_back(s->p2());
		for (int i=0; i<points.size(); i++)
		{
			Line* l = new Line(shape->render());

			l->set(from, points[i]);
			l->parent(s->parent());
			l->done(true);
			l->compute();
			result.push_back(l);

			from = points[i];
		}
	}
	else if (shape->type() == GraphicType::Circle)
	{
		Circle* c = (Circle*)shape;

		// we compute angles then sort them from 0 to 2*PI
		std::vector<float> angles;
		for (glm::vec2 p : points)
			angles.push_back(geometry::oriented_angle(p, c->center()));
		std::sort(angles.begin(), angles.end(), std::less<float>());

		// we can now split into arcs the circle
		float start = angles.back();
		for (float angle : angles)
		{
			auto p1 = geometry::position(start, c->radius(), c->center());
			auto p2 = geometry::position(angle, c->radius(), c->center());
			Arc* a = new Arc(shape->render());
			a->set(p1, c->center(), p2, false);
			a->parent(c->parent());
			a->done(true);
			a->compute();
			result.push_back(a);
			start = angle;
		}
	}
	else if (shape->type() == GraphicType::Arc)
	{
		Arc* c = (Arc*)shape;

		// cutting an arc, first, we get sure to have an arc in counterclock direction
		// then we retreive the oriented angle of start, stop and each split points
		// we add start and stop angles to the list of angles
		// we sort the list, here from lower to greater
		// because oriented angle start from 0 to 2*PI, an arc can pass through the 0 angle
		// so we will rotate the list to start angle, to be sure to get all arc angles from start
		// next, start angle is removed from the list, to iterate through it
		bool reversed;
		if (reversed = c->cw())
			c->reverse();

		float start = geometry::oriented_angle(c->start(), c->center());
		float stop = geometry::oriented_angle(c->stop(), c->center());

		// we compute angles then sort them following 0 to 2*PI
		std::vector<float> angles;
		for (glm::vec2 p : points)
			angles.push_back(geometry::oriented_angle(p, c->center()));

		angles.push_back(start);
		angles.push_back(stop);
		std::sort(angles.begin(), angles.end(), std::less<float>());

		// we needs to rotate list to start angle
		auto it = std::find(angles.begin(), angles.end(), start);
		std::rotate(angles.begin(), it, angles.end());
		angles.erase(angles.begin());
		
		// we can now split into arcs the circle
		for (float angle : angles)
		{
			auto p1 = geometry::position(start, c->radius(), c->center());
			auto p2 = geometry::position(angle, c->radius(), c->center());
			Arc* a = new Arc(shape->render());
			a->set(p1, c->center(), p2, false);
			a->parent(c->parent());
			a->done(true);
			a->compute();
			result.push_back(a);
			start = angle;
		}

		if (reversed)
			c->reverse();
	}
	else if (shape->type() == GraphicType::Ellipse)
	{
		Ellipse* ellipse = (Ellipse*)shape;
		auto lists = split(ellipse->coordinates(), points);
		for (std::vector<glm::vec2> list : lists)
		{
			if (list.size() == 2)
			{
				Line* l = new Line(shape->render());
				l->parent(shape->parent());
				l->set(list[0], list[1]);
				result.push_back(l);
			}
			else
			{
				Polyline* p = new Polyline(shape->render());
				p->parent(shape->parent());
				p->mode(PolylineMode::Polyline);
				p->points(list);
				result.push_back(p);
			}
		}
		}
	else if (shape->type() == GraphicType::Polyline)
	{
		Polyline* poly = (Polyline*)shape;
		std::vector<glm::vec2> coordinates;
		if (poly->mode() == PolylineMode::Polyline)
			coordinates = poly->points();
		else
			coordinates = poly->coordinates();

		auto lists = split(coordinates, points);
		for (std::vector<glm::vec2> list : lists)
		{
			if (list.size() == 2)
			{
				Line* l = new Line(poly->render());
				l->parent(poly->parent());
				l->set(list[0], list[1]);
				result.push_back(l);
			}
			else
			{
				Polyline* p = new Polyline(shape->render());
				p->parent(shape->parent());
				p->mode(PolylineMode::Polyline);
				p->points(list);
				result.push_back(p);
			}
		}

	}

	return result;
}

std::vector<std::vector<glm::vec2>> cad::split(std::vector<glm::vec2> coordinates, std::vector<glm::vec2> points, std::vector<std::vector<int>> indices)
{
	return std::vector<std::vector<glm::vec2>>();
}

struct intersection
{
	int a;
	int b;
	glm::vec2 point;

	intersection() {
		a = b = 0;
		point = geometry::vec2_empty;
	}

	intersection(int a, int b, glm::vec2 point) {
		this->a = a;
		this->b = b;
		this->point = point;
	}
};

std::vector<std::vector<glm::vec2>> cad::split(std::vector<glm::vec2> coordinates)
{
	std::vector < std::vector<glm::vec2>> result; // contains final result
	glm::vec2 p = glm::vec2();
	std::vector<int> indices;

	//auto sorted = sort_vec2_x_smaller(coordinates);

	std::vector<std::vector<glm::vec2>> intersections(coordinates.empty() ? 0 : coordinates.size() - 1);
	for (int i = 0; i + 1 < coordinates.size(); i++)
		intersections[i] = std::vector<glm::vec2>();

	for (int i = 0; i + 1 < coordinates.size(); i++)
	{
		for (int j = i + 2; j + 1 < coordinates.size(); j++)
		{
			glm::vec2 a1 = coordinates[i];
			glm::vec2 a2 = coordinates[i+1];
			glm::vec2 b1 = coordinates[j];
			glm::vec2 b2 = coordinates[j+1];

			if (geometry::segment_segment_intersect(a1, a2, b1, b2, p))
			{
				if (std::find(indices.begin(), indices.end(), i) == indices.end())
					indices.push_back(i);
				if (std::find(intersections[i].begin(), intersections[i].end(), p) == intersections[i].end())
					intersections[i].push_back(p);
				if (std::find(indices.begin(), indices.end(), j) == indices.end())
					indices.push_back(j);
				if (std::find(intersections[j].begin(), intersections[j].end(), p) == intersections[j].end())
					intersections[j].push_back(p);
			}
		}
	}

	//for (segment& s1 : sorted)
	//{
	//	auto segments = get_segments(sorted, s1.vec1);

	//	count++;
	//	for (segment& s2 : segments)
	//	{
	//		if (geometry::segment_segment_intersect(s1.vec1, s1.vec2, s2.vec1, s2.vec2, p))
	//		{
	//			if (std::find(indices.begin(), indices.end(), s1.index) == indices.end())
	//				indices.push_back(s1.index);
	//			if (std::find(intersections[s1.index].begin(), intersections[s1.index].end(), p) == intersections[s1.index].end())
	//				intersections[s1.index].push_back(p);
	//			if (std::find(indices.begin(), indices.end(), s2.index) == indices.end())
	//				indices.push_back(s2.index);
	//			if (std::find(intersections[s2.index].begin(), intersections[s2.index].end(), p) == intersections[s2.index].end())
	//				intersections[s2.index].push_back(p);
	//		}
	//	}
	//}

	// we have segments in indices and its intersections coordinates in intersections[]
	for (int i : indices)
	{
		glm::vec2 p1 = coordinates[i];
		glm::vec2 p2 = coordinates[i+1];

		if (p1.x == p2.x) // vertical
		{
			if (p1.y < p2.y)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_greater);
		}
		else 
		{
			if (p1.x < p2.x)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_greater);
		}
	}

	// the cleaning can empty the list of a segment which was shared with its neighbour: it must not be split any more
	indices.erase(std::remove_if(indices.begin(), indices.end(), [&](int i) { return intersections[i].empty(); }), indices.end());

	// we now construct the new list of polylines
	std::sort(indices.begin(), indices.end(), std::less<int>());
	int from = 0;
	result.push_back(std::vector<glm::vec2>());						// adding first empty polyline
	for (int i : indices)
	{
		for (int j = from; j <= i; j++)							// we go from to first indice to split
			result.back().push_back(coordinates[j]);
		result.back().push_back(intersections[i][0]);				// we add first intersection of current indice
		result.push_back(std::vector<glm::vec2>());					// then we construct a new polyline
		for (int j = 0; j + 1 < intersections[i].size(); j++)	// we add next intersection
		{
			result.back().push_back(intersections[i][j]);			// if there are several intersections, we add just segments
			result.back().push_back(intersections[i][j + 1]);
			result.push_back(std::vector<glm::vec2>());				// then we construct a new polyline 
		}
		result.back().push_back(intersections[i].back());			// and we loop until the last intersection ref_point
		from = i + 1;
	}
	for (int i = from; i < coordinates.size(); i++)				// we end the last polyline by adding from last intersection ref_point to the rest of original polyline
		result.back().push_back(coordinates[i]);

	return result;
}

std::vector < std::vector<glm::vec2>> cad::split(glm::vec2 p1, glm::vec2 p2, std::vector<glm::vec2> coordinates)
{
	int count = 0;
	std::vector < std::vector<glm::vec2>> result; // contains final result
	glm::vec2 p = glm::vec2();
	std::vector<int> indices;

	std::vector<std::vector<glm::vec2>> intersections(coordinates.empty() ? 0 : coordinates.size() - 1);
	for (int i = 0; i + 1 < coordinates.size(); i++)
		intersections[i] = std::vector<glm::vec2>();

	for (int i = 0; i + 1 < coordinates.size(); i++)
	{
		glm::vec2 a1 = coordinates[i];
		glm::vec2 a2 = coordinates[i];
		count++;
		if (geometry::segment_segment_intersect(p1, p2, a1, a2, p))
		{
			if (std::find(indices.begin(), indices.end(), i) == indices.end())
				indices.push_back(i);
			if (std::find(intersections[i].begin(), intersections[i].end(), p) == intersections[i].end())
				intersections[i].push_back(p);
		}
	}
	// we have segments in indices and its intersections coordinates in intersections[]
	for (int i : indices)
	{
		glm::vec2 p1 = coordinates[i];
		glm::vec2 p2 = coordinates[i + 1];

		if (p1.x == p2.x) // vertical
		{
			if (p1.y < p2.y)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_greater);
		}
		else
		{
			if (p1.x < p2.x)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_greater);
		}
	}

	std::sort(indices.begin(), indices.end(), std::less<int>());
	int from = 0;
	result.push_back(std::vector<glm::vec2>());
	for (int i : indices)
	{
		for (int j = from; j <= i; j++)
			result.back().push_back(coordinates[j]);
		result.back().push_back(intersections[i][0]);
		result.push_back(std::vector<glm::vec2>());
		for (int j = 0; j + 1 < intersections[i].size(); j++)
		{
			result.back().push_back(intersections[i][j]);
			result.back().push_back(intersections[i][j + 1]);
			result.push_back(std::vector<glm::vec2>());
		}
		result.back().push_back(intersections[i].back());
		from = i + 1;
	}
	for (int i = from; i < coordinates.size(); i++)
		result.back().push_back(coordinates[i]);

	Logger::trace("Nb comp=" + std::to_string(count));

	return result;
}

std::vector < std::vector<glm::vec2>> cad::split(std::vector<glm::vec2> coordinates, std::vector<glm::vec2> points)
{
	std::vector < std::vector<glm::vec2>> result; // contains final result
	if (points.size() == 0)
	{
		result.push_back(coordinates);
		return result;
	}

	auto sorted = sort_vec2_x_smaller(coordinates);

	glm::vec2 p = glm::vec2();
	std::vector<int> indices;

	std::vector<std::vector<glm::vec2>> intersections;
	intersections.reserve(coordinates.empty() ? 0 : coordinates.size() - 1);
	for (int i = 0; i + 1 < coordinates.size(); i++)
		intersections.push_back(std::vector<glm::vec2>());

	for (glm::vec2 p : points)
	{
		auto segments = get_segments(sorted, p);

		for (segment s : segments)
		{
			if (geometry::colinear_segment_point(s.vec1, s.vec2, p))
			{
				if (std::find(indices.begin(), indices.end(), s.index) == indices.end())
					indices.push_back(s.index);
				if (std::find(intersections[s.index].begin(), intersections[s.index].end(), p) == intersections[s.index].end())
					intersections[s.index].push_back(p);
			}
		}
	}
	// we have segments in indices and its intersections coordinates in intersections[]
	for (int i : indices)
	{
		glm::vec2 p1 = coordinates[i];
		glm::vec2 p2 = coordinates[i + 1];

		if (p1.x == p2.x) // vertical
		{
			if (p1.y < p2.y)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_y_greater);
		}
		else
		{
			if (p1.x < p2.x)
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_smaller);
			else
				std::sort(intersections[i].begin(), intersections[i].end(), compare_vec2_x_greater);
		}
	}

	//////////////////////////////// CLEANING SECTION //////////////////////////////
	// some of the intersection points may be referenced on two neighbouring segments due to float lake of accuracy
	// so we need to check and keep the intersection ref_point only for the closest segment
	// we start to list neighbouring segments
	std::vector<std::array<int, 2>> pairs;
	for (size_t i = 0; i + 1 < indices.size(); i++)	// (indices may be empty: no unsigned underflow)
	{
		if (indices[i] == indices[i + 1] - 1 || indices[i] - 1 == indices[i + 1])
		{
			std::array<int, 2> p;
			p[0] = indices[i];
			p[1] = indices[i + 1];
			pairs.push_back(p);
		}
	}
	// then we look for every intersection points that could be in both indices
	for (std::array<int, 2>& p : pairs)
	{
		auto iter = intersections[p[0]].begin();
		while(iter != intersections[p[0]].end())
		{
			auto pos = std::find(intersections[p[1]].begin(), intersections[p[1]].end(), *iter);
			if ( pos != intersections[p[1]].end())
			{
				// if a ref_point is in two neighbouring segments, then we test for the closest and remove the intersection ref_point in the farest segment intersection list
				float d1 = geometry::foretriangle2(coordinates[p[0]], coordinates[p[0] + 1], *iter);
				float d2 = geometry::foretriangle2(coordinates[p[1]], coordinates[p[1] + 1], *iter);
				if (d1 < d2)
				{
					iter++;
					intersections[p[1]].erase(pos);
					if (intersections[p[1]].size() == 0)
						indices.erase(std::find(indices.begin(), indices.end(), p[1]));
				}
				else
					iter = intersections[p[0]].erase(iter);
			}
			else
				iter++;
		}
	}
	//////////////////////////////////// END OF CLEANING SECTION ////////////////////////////////////////


	// the cleaning can empty the list of a segment which was shared with its neighbour: it must not be split any more
	indices.erase(std::remove_if(indices.begin(), indices.end(), [&](int i) { return intersections[i].empty(); }), indices.end());

	// we now construct the new list of polylines
	std::sort(indices.begin(), indices.end(), std::less<int>());
	int from = 0;
	result.push_back(std::vector<glm::vec2>());						// adding first empty polyline
	for (int i : indices)
	{
		for (int j = from; j <= i; j++)							// we go from to first indice to split
			result.back().push_back(coordinates[j]);
		result.back().push_back(intersections[i][0]);				// we add first intersection of current indice
		result.push_back(std::vector<glm::vec2>());					// then we construct a new polyline
		for (int j = 0; j + 1 < intersections[i].size(); j++)	// we add next intersection
		{
			result.back().push_back(intersections[i][j]);			// if there are several intersections, we add just segments
			result.back().push_back(intersections[i][j + 1]);
			result.push_back(std::vector<glm::vec2>());				// then we construct a new polyline 
		}
		result.back().push_back(intersections[i].back());			// and we loop until the last intersection ref_point
		from = i + 1;
	}
	for (int i = from; i < coordinates.size(); i++)				// we end the last polyline by adding from last intersection ref_point to the rest of original polyline
		result.back().push_back(coordinates[i]);

	return result;
}

std::vector<glm::vec2> cad::get_raw_coordinates(std::vector<Shape*> shapes)
{
	std::vector<glm::vec2> coordinates;

	for (Shape* s : shapes)
	{
		if (s->type() == GraphicType::Circle)
		{
			auto c = geometry::circle(((Circle*)s)->radius());
			for (glm::vec2 v : c)
				coordinates.push_back(v + ((Circle*)s)->center());
			std::reverse(coordinates.begin(), coordinates.end());
		}
		else if (s->type() == GraphicType::Ellipse)
		{
			auto cc = ((Ellipse*)s)->coordinates();
			coordinates.insert(coordinates.end(), cc.begin(), cc.end());
		}
		/*else if (s->type() == GraphicType::Arc)
		{
			std::vector<glm::vec2> temp;
			auto cc = ((Arc*)s)->coordinates();
			std::merge(coordinates.begin(), coordinates.end(), cc.begin(), cc.end(), temp);
			coordinates = temp;
		}*/
		else if (s->type() == GraphicType::Line)
		{
			coordinates.push_back(((Line*)s)->p1());
			coordinates.push_back(((Line*)s)->p2());
		}
		else if (s->type() == GraphicType::Polyline)
		{
			auto cc = ((Polyline*)s)->coordinates();
			coordinates.insert(coordinates.end(), cc.begin(), cc.end());
		}
	}


	return coordinates;
}

std::string cad::info(std::vector<Shape*> shapes)
{
	Curve curve = to_curve(shapes);

	return "area=" + std::to_string(curve.area()) + " clockwise=" + (curve.cw() ? "true" : "false");
}

Curve cad::to_curve(std::vector<Shape*> shapes)
{
	Curve curve;
	std::string result = "";
	std::vector<glm::vec2> temp;
	std::vector<std::vector<glm::vec2>> text;

	for (Shape* s : shapes)
	{
		switch (s->type())
		{
		case GraphicType::Line:
			curve.add(((Line*)s)->p1());
			break;
		case GraphicType::Arc:
			curve.add(((Arc*)s)->start(), ((Arc*)s)->center(), ((Arc*)s)->radius(), ((Arc*)s)->cw());
			break;
		case GraphicType::Polyline:
			temp = ((Polyline*)s)->coordinates();
			for (glm::vec2 v : temp)
				curve.add(v);
			break;
		case GraphicType::Ellipse:
			temp = ((Ellipse*)s)->coordinates();
			for (glm::vec2 v : temp)
				curve.add(v);
			break;
		case GraphicType::Circle:
			curve.add(((Circle*)s)->center(), ((Circle*)s)->radius(), false);
			break;
		case GraphicType::Spline:
			curve = ((Spline*)s)->curve();
			break;
		}
	}

	if (shapes.size() > 1 && shapes.front()->first() == shapes.back()->last())
		curve.close();

	return curve;
}

Curve cad::to_curve(std::vector<glm::vec2> coordinates)
{
	Curve curve;
	for (glm::vec2 v : coordinates)
		curve.add(v);

	return curve;
}

Spline* cad::to_spline(std::vector<Shape*> shapes, Renderer* render)
{
	if (shapes.size() == 0)
		return nullptr;

	Spline* spline = new Spline(render);
	std::vector<glm::vec2> coordinates;
	glm::vec2 last = glm::vec2();
	bool add_last = false;

	for (Shape* s : shapes)
	{
		switch (s->type())
		{
		case GraphicType::Line:
			spline->add(((Line*)s)->p1());
			last = ((Line*)s)->p2();
			add_last = true;
			break;
		case GraphicType::Arc:
			spline->add(((Arc*)s)->start(), ((Arc*)s)->center(), ((Arc*)s)->radius(), ((Arc*)s)->cw());
			last = ((Arc*)s)->stop();
			add_last = true;
			break;
		case GraphicType::Circle:
			spline->add(((Circle*)s)->center(), ((Circle*)s)->radius(), false);
			break;
		case GraphicType::Polyline:
			coordinates = ((Polyline*)s)->coordinates();
			if (coordinates.size() > 1)
			{
				for (int i = 0; i + 1 < coordinates.size(); i++)
					spline->add(coordinates[i]);
				last = coordinates.back();
			}
			add_last = true;
			break;
		case GraphicType::Ellipse:
			coordinates = ((Ellipse*)s)->coordinates();
			if (coordinates.size() > 1)
			{
				for (int i = 0; i + 1 < coordinates.size(); i++)
					spline->add(coordinates[i]);
				last = coordinates.back();
			}
			add_last = true;
			break;
		case GraphicType::Spline:
			spline->add(((Spline*)s)->curve());
		default:
			break;
		}
	}

	if (add_last)
		spline->add(last);

	if (spline->size() > 1)
	{
		spline->done(true);
		spline->compute();
		return spline;
	}

	return nullptr;
}


Spline* cad::to_spline(std::vector<glm::vec2> points, Renderer* render)
{
	if (points.size() > 1)
	{
		Spline* spline = new Spline(render);
		for (glm::vec2 p : points)
			spline->add(p);
		spline->done(true);
		spline->compute();
		return spline;
	}
	else
		return nullptr;
}

std::vector<Shape*> cad::chamfer(std::vector<Shape*> shapes, float radius)
{
	std::vector<Shape*> result;
	std::vector<Shape*> clones;

	for (Shape* c : shapes)
	{
		Shape* clone = c->clone();
		clone->id(c->id());
		clones.push_back(clone);
	}

	for (auto s : clones)
	{
		auto candidates = connected(s, clones);

		for (auto c : candidates)
		{
			// ensure that s->last == c->first
			if (c->first() == s->first() || c->last() == s->first())
				s->reverse();
			if (c->last() == s->last())
				c->reverse();
			if (s->last() == c->first())
			{
				bool done = false;
				glm::vec2 h1, h2;

				switch (s->type())
				{
				case GraphicType::Line:
					switch (c->type())
					{
					case GraphicType::Line:
						h1 = geometry::position(s->first(), radius, s->last());
						h2 = geometry::position(c->last(), radius, c->first());

						Line* l = new Line(s->render());
						l->set(h1, h2);
						result.push_back(l);
						done = true;
						break;
					}
					break;
				}

				if (done)
				{
					s->last(h1);
					if (std::find(result.begin(), result.end(), s) == result.end())
						result.push_back(s);

					c->first(h2);
					if (std::find(result.begin(), result.end(), c) == result.end())
						result.push_back(c);
				}
			}
		}
	}

	for (auto shape : clones)
	{
		if (std::find(result.begin(), result.end(), shape) == result.end())
			delete shape;
	}

	return result;
}

std::vector<Shape*> cad::fillet(std::vector<Shape*> shapes, float radius)
{
	std::vector<Shape*> result;
	std::vector<Shape*> clones;

	for (Shape* c : shapes)
	{
		Shape* clone = c->clone();
		clone->id(c->id());
		clones.push_back(clone);
	}

	for (auto s : clones)
	{
		auto candidates = connected(s, clones);

		for (auto c : candidates)
		{
			// ensure that s->last == c->first
			if (c->first() == s->first() || c->last() == s->first())
				s->reverse();
			if (c->last() == s->last())
				c->reverse();
			if (s->last() == c->first())
			{
				bool done = false;
				glm::vec2 h1, h2, p1, p2;
				float a1, a2;

				switch (s->type())
				{
				case GraphicType::Line:
					switch (c->type())
					{
					case GraphicType::Line:
						a1 = geometry::oriented_angle(s->first(), s->last());
						h1 = geometry::position(a1, radius, s->last());
						p1 = geometry::position(a1 + glm::half_pi<float>(), 10, h1);

						a2 = geometry::oriented_angle(c->last(), c->first());
						h2 = geometry::position(a2, radius, c->first());
						p2 = geometry::position(a2 + glm::half_pi<float>(), 10, h2);

						if (a1 - a2 != 0 && a1 + a2 != 0) // check if not colinear
						{
							glm::vec2 center = geometry::vec2_empty;
							geometry::line_line_intersect(h1, p1, h2, p2, center);

							// center hold the fillet center, h1 is start and h2 is stop
							// arc cw must follow the triangle cw formed by s->first, s->last, c->last
							Arc* a = new Arc(s->render());
							a->set(h1, center, h2, geometry::cw(s->first(), s->last(), c->last()));
							result.push_back(a);
							done = true;
						}
						break;
					}
					break;
				}

				if (done)
				{
					s->last(h1);
					if (std::find(result.begin(), result.end(), s) == result.end())
						result.push_back(s);

					c->first(h2);
					if (std::find(result.begin(), result.end(), c) == result.end())
						result.push_back(c);
				}
			}
		}
	}

	for (auto shape : clones)
	{
		if (std::find(result.begin(), result.end(), shape) == result.end())
			delete shape;
	}

	return result;
}

std::vector<Shape*> cad::connected(Shape* s, std::vector<Shape*> shapes)
{
	std::vector<Shape*> result;

	if (s->type() == GraphicType::Line /*|| s->type() == GraphicType::Arc*/ || s->type() == GraphicType::Polyline || s->type() == GraphicType::Spline)
	{
		for (Shape* c : shapes)
		{
			if (c->id() != s->id())
			{
				if (s->first() == c->first() || s->first() == c->last() || s->last() == c->first() || s->last() == c->last())
					result.push_back(c);
			}
		}
	}

	return result;
}
