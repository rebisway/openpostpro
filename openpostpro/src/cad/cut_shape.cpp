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

#include "cut_shape.h"
#include "extend_shape.h"
#include <geometry.h>
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <polyline.h>
#include <ellipse.h>
#include <limits>

namespace
{
	const float END_DISTANCE = 0.001f;

	bool cuttable(Shape* s)
	{
		switch (s->type())
		{
		case GraphicType::Line:
		case GraphicType::Arc:
		case GraphicType::Circle:
		case GraphicType::Polyline:
		case GraphicType::Ellipse:
			return true;
		default:
			return false;
		}
	}

	// angle swept from `from` to `to` around the center, in the direction of the arc
	float sweep(glm::vec2 from, glm::vec2 to, glm::vec2 center, bool cw)
	{
		float a = geometry::oriented_angle(from, center);
		float b = geometry::oriented_angle(to, center);
		return cw ? geometry::oriented_angle(a - b) : geometry::oriented_angle(b - a);
	}
}

bool cad_cut::nearest(Curve& curve, glm::vec2 point, glm::vec2& result)
{
	float best = (std::numeric_limits<float>::max)();
	bool whole_circle = false;

	for (size_t i = 0; i < curve.size(); i++)
	{
		Segment& s = curve[i];
		glm::vec2 candidate;
		bool found = false;

		if (s.type == SegmentType::Circle)
		{
			whole_circle = true;
			glm::vec2 d = point - s.center;
			float l = glm::length(d);
			if (l < geometry::ERR_FLOAT)
				continue;
			candidate = s.center + d / l * s.radius;
			found = true;
		}
		else if (i + 1 < curve.size())
		{
			glm::vec2 end = curve[i + 1].point;
			if (s.type == SegmentType::Line)
			{
				glm::vec2 d = end - s.point;
				float l2 = glm::dot(d, d);
				if (l2 < geometry::ERR_FLOAT)
					continue;
				float t = glm::clamp(glm::dot(point - s.point, d) / l2, 0.0f, 1.0f);
				candidate = s.point + d * t;
				found = true;
			}
			else
			{
				// arc: the point of the circle in the direction of `point`, if the arc goes there, otherwise its nearest end
				glm::vec2 d = point - s.center;
				float l = glm::length(d);
				float radius = glm::distance(s.point, s.center);
				if (l < geometry::ERR_FLOAT)
					continue;
				glm::vec2 on_circle = s.center + d / l * radius;
				// an arc which ends where it starts goes all the way around
				float span = glm::distance(s.point, end) < END_DISTANCE ? glm::two_pi<float>() : sweep(s.point, end, s.center, s.cw);
				if (sweep(s.point, on_circle, s.center, s.cw) <= span)
					candidate = on_circle;
				else
					candidate = glm::distance(point, s.point) < glm::distance(point, end) ? s.point : end;
				found = true;
			}
		}

		if (found)
		{
			float distance = glm::distance(point, candidate);
			if (distance < best)
			{
				best = distance;
				result = candidate;
			}
		}
	}

	if (best == (std::numeric_limits<float>::max)())
		return false;

	// the ends of a curve can not be cut (a closed polyline is already open at its start)
	if (!whole_circle)
	{
		if (glm::distance(result, curve.front().point) < END_DISTANCE || glm::distance(result, curve.back().point) < END_DISTANCE)
			return false;
	}
	return true;
}

std::vector<glm::vec2> cad_cut::intersections(Curve& a, Curve& b)
{
	std::vector<glm::vec2> result;
	for (size_t i = 0; i < a.size(); i++)
	{
		for (size_t k = 0; k < b.size(); k++)
		{
			Segment& s1 = a[i];
			Segment& s2 = b[k];
			// the last segment of a curve is only its end point
			if (s1.type != SegmentType::Circle && i + 1 >= a.size())
				continue;
			if (s2.type != SegmentType::Circle && k + 1 >= b.size())
				continue;

			glm::vec2 dst1 = i + 1 < a.size() ? a[i + 1].point : s1.point;
			glm::vec2 dst2 = k + 1 < b.size() ? b[k + 1].point : s2.point;
			glm::vec2 hits[2];
			int count;
			if (s1.type == SegmentType::Circle && s2.type == SegmentType::Circle)
				count = geometry::circle_circle_intersect(s1.center, s1.radius, s2.center, s2.radius, hits);
			else
				count = s1.intersect(dst1, s2, dst2, hits);
			for (int h = 0; h < count; h++)
				result.push_back(hits[h]);
		}
	}
	return result;
}

std::vector<std::vector<glm::vec2>> cad_cut::split_points(const std::vector<glm::vec2>& points, glm::vec2 cut)
{
	std::vector<std::vector<glm::vec2>> result;
	if (points.size() < 2)
		return result;

	// the segment which is nearest to the cut point
	size_t index = 0;
	float best = (std::numeric_limits<float>::max)();
	for (size_t i = 0; i + 1 < points.size(); i++)
	{
		glm::vec2 d = points[i + 1] - points[i];
		float l2 = glm::dot(d, d);
		float t = l2 < geometry::ERR_FLOAT ? 0.0f : glm::clamp(glm::dot(cut - points[i], d) / l2, 0.0f, 1.0f);
		float distance = glm::distance(cut, points[i] + d * t);
		if (distance < best)
		{
			best = distance;
			index = i;
		}
	}
	if (best > 0.01f)
		return result;

	bool closed = glm::distance(points.front(), points.back()) < END_DISTANCE;

	// the cut is on a vertex (the end of the segment, or its start), or inside the segment
	std::vector<glm::vec2> first, second;
	size_t vertex = points.size();
	if (glm::distance(cut, points[index]) < END_DISTANCE)
		vertex = index;
	else if (glm::distance(cut, points[index + 1]) < END_DISTANCE)
		vertex = index + 1;

	if (vertex != points.size())
	{
		// the ends of an open polyline can not be cut, and a closed one is already open at its start
		if (vertex == 0 || vertex == points.size() - 1)
			return result;
		first.assign(points.begin(), points.begin() + vertex + 1);
		second.assign(points.begin() + vertex, points.end());
	}
	else
	{
		first.assign(points.begin(), points.begin() + index + 1);
		first.push_back(cut);
		second.push_back(cut);
		second.insert(second.end(), points.begin() + index + 1, points.end());
	}

	if (closed)
	{
		// opened at the cut: the end of the polyline goes on with its start (their common point is kept once)
		std::vector<glm::vec2> opened = second;
		opened.insert(opened.end(), first.begin() + 1, first.end());
		result.push_back(opened);
	}
	else
	{
		result.push_back(first);
		result.push_back(second);
	}
	return result;
}

std::vector<Shape*> cad_cut::split_shape(Shape* shape, glm::vec2 cut)
{
	std::vector<Shape*> result;
	std::vector<glm::vec2> points;

	if (shape->type() == GraphicType::Line)
	{
		Line* line = (Line*)shape;
		if (glm::distance(cut, line->p1()) < END_DISTANCE || glm::distance(cut, line->p2()) < END_DISTANCE)
			return result;
		Line* a = new Line(shape->render());
		a->parent(shape->parent());
		a->set(line->p1(), cut);
		Line* b = new Line(shape->render());
		b->parent(shape->parent());
		b->set(cut, line->p2());
		result = { a, b };
		return result;
	}
	if (shape->type() == GraphicType::Arc)
	{
		// both pieces keep the center and the direction of the arc. An arc which goes all the way around (start = end) is
		// cut the same way
		Arc* arc = (Arc*)shape;
		bool full = glm::distance(arc->start(), arc->stop()) < END_DISTANCE;
		if (glm::distance(cut, arc->start()) < END_DISTANCE || (!full && glm::distance(cut, arc->stop()) < END_DISTANCE))
			return result;
		Arc* a = new Arc(shape->render());
		a->parent(shape->parent());
		a->set(arc->start(), arc->center(), cut, arc->cw());
		Arc* b = new Arc(shape->render());
		b->parent(shape->parent());
		b->set(cut, arc->center(), arc->stop(), arc->cw());
		result = { a, b };
		return result;
	}
	if (shape->type() == GraphicType::Circle)
	{
		// a circle cut at one point is an arc which goes all the way around, open at that point
		Circle* circle = (Circle*)shape;
		Arc* a = new Arc(shape->render());
		a->parent(shape->parent());
		a->set(cut, circle->center(), cut, false);
		result = { a };
		return result;
	}

	if (shape->type() == GraphicType::Polyline)
	{
		Polyline* poly = (Polyline*)shape;
		points = poly->mode() == PolylineMode::Polyline ? poly->points() : poly->coordinates();
	}
	else if (shape->type() == GraphicType::Ellipse)
		points = ((Ellipse*)shape)->coordinates();
	else
		return result;

	for (std::vector<glm::vec2>& list : split_points(points, cut))
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
	return result;
}

std::vector<cad_cut::Cut> cad_cut::find(std::vector<Layer*>& layers, glm::vec2 mouse, glm::vec2 snapped)
{
	std::vector<Shape*> under;
	// the shapes of a locked layer are used to find a crossing, but they are never cut
	std::vector<bool> locked;
	for (auto layer = layers.rbegin(); layer != layers.rend(); ++layer)
	{
		if (!(*layer)->visible())
			continue;
		for (auto s = (*layer)->shapes().rbegin(); s != (*layer)->shapes().rend(); ++s)
		{
			Shape* shape = *s;
			if (!cuttable(shape))
				continue;
			if ((shape->bounds().contains(mouse) && shape->is_over(mouse)) || (shape->bounds().contains(snapped) && shape->is_over(snapped)))
			{
				under.push_back(shape);
				locked.push_back((*layer)->locked());
			}
		}
	}

	std::vector<Cut> result;
	if (under.empty())
		return result;

	std::vector<Curve> curves;
	for (Shape* s : under)
		curves.push_back(cad_extend::curve_of(s));

	// several shapes under the click: the aim is their crossing
	bool crossing = false;
	glm::vec2 point = snapped;
	if (under.size() > 1)
	{
		float best = (std::numeric_limits<float>::max)();
		for (glm::vec2 p : intersections(curves[0], curves[1]))
		{
			float d = glm::distance(p, snapped);
			if (d < best)
			{
				best = d;
				point = p;
				crossing = true;
			}
		}
	}

	for (size_t i = 0; i < under.size(); i++)
	{
		if (locked[i])
			continue;
		Cut cut;
		cut.shape = under[i];
		// at a crossing, the shapes which are not the first two are cut where they are nearest to it
		if (!nearest(curves[i], point, cut.point))
			continue;
		result.push_back(cut);
	}
	return result;
}
