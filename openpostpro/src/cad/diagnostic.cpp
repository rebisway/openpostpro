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

#include "diagnostic.h"
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <polyline.h>
#include <ellipse.h>
#include <geometry.h>
#include <algorithm>

namespace
{
	bool same(glm::vec2 a, glm::vec2 b, float tolerance)
	{
		return glm::distance(a, b) <= tolerance;
	}

	// shapes which have two ends (an open contour may start or end on them)
	bool has_ends(Shape* s)
	{
		switch (s->type())
		{
		case GraphicType::Line:
		case GraphicType::Arc:
		case GraphicType::Polyline:
		case GraphicType::Spline:
			return true;
		default:
			return false;
		}
	}

	// the points of a shape, to compare two shapes
	std::vector<glm::vec2> points_of(Shape* s)
	{
		switch (s->type())
		{
		case GraphicType::Polyline:
			return ((Polyline*)s)->coordinates();
		case GraphicType::Ellipse:
			return ((Ellipse*)s)->coordinates();
		default:
			return std::vector<glm::vec2>();
		}
	}

	bool same_points(const std::vector<glm::vec2>& a, const std::vector<glm::vec2>& b, float tolerance)
	{
		if (a.size() != b.size() || a.empty())
			return false;

		bool forward = true, backward = true;
		for (size_t i = 0; i < a.size() && (forward || backward); i++)
		{
			forward = forward && same(a[i], b[i], tolerance);
			backward = backward && same(a[i], b[b.size() - 1 - i], tolerance);
		}
		return forward || backward;
	}

	bool zero_length(Shape* s, float tolerance)
	{
		switch (s->type())
		{
		case GraphicType::Line:
			return same(((Line*)s)->p1(), ((Line*)s)->p2(), tolerance);
		case GraphicType::Arc:
			return ((Arc*)s)->radius() <= tolerance;
		case GraphicType::Circle:
			return ((Circle*)s)->radius() <= tolerance;
		case GraphicType::Polyline:
		{
			auto points = ((Polyline*)s)->coordinates();
			for (glm::vec2 p : points)
				if (!same(p, points.front(), tolerance))
					return false;
			return true;
		}
		default:
			return false;
		}
	}

	// are the two shapes the same shape, at the same place?
	bool duplicate(Shape* a, Shape* b, float tolerance)
	{
		if (a->type() != b->type())
			return false;

		switch (a->type())
		{
		case GraphicType::Line:
		{
			Line* l1 = (Line*)a;
			Line* l2 = (Line*)b;
			return (same(l1->p1(), l2->p1(), tolerance) && same(l1->p2(), l2->p2(), tolerance))
				|| (same(l1->p1(), l2->p2(), tolerance) && same(l1->p2(), l2->p1(), tolerance));
		}
		case GraphicType::Arc:
		{
			Arc* a1 = (Arc*)a;
			Arc* a2 = (Arc*)b;
			if (!same(a1->center(), a2->center(), tolerance) || std::fabs(a1->radius() - a2->radius()) > tolerance)
				return false;
			return (same(a1->first(), a2->first(), tolerance) && same(a1->last(), a2->last(), tolerance))
				|| (same(a1->first(), a2->last(), tolerance) && same(a1->last(), a2->first(), tolerance));
		}
		case GraphicType::Circle:
			return same(((Circle*)a)->center(), ((Circle*)b)->center(), tolerance) && std::fabs(((Circle*)a)->radius() - ((Circle*)b)->radius()) <= tolerance;
		case GraphicType::Polyline:
		case GraphicType::Ellipse:
			return same_points(points_of(a), points_of(b), tolerance);
		default:
			return false;
		}
	}
}

std::vector<cad_diag::Issue> cad_diag::check(std::vector<Layer*>& layers, float tolerance)
{
	std::vector<Issue> result;

	std::vector<Shape*> shapes;
	for (Layer* layer : layers)
		if (layer->visible())
			for (Shape* s : layer->shapes())
				shapes.push_back(s);

	// a shape of length zero has no end to speak about, and is not compared
	std::vector<Shape*> valid;
	for (Shape* s : shapes)
	{
		if (zero_length(s, tolerance))
		{
			Issue issue;
			issue.type = IssueType::ZeroLength;
			issue.shape = s;
			issue.point = s->type() == GraphicType::Circle ? ((Circle*)s)->center() : s->first();
			result.push_back(issue);
		}
		else
			valid.push_back(s);
	}

	// two shapes at the same place: the second one is the one to delete
	std::vector<bool> is_duplicate(valid.size(), false);
	for (size_t i = 0; i < valid.size(); i++)
	{
		for (size_t j = i + 1; j < valid.size() && !is_duplicate[i]; j++)
		{
			if (!is_duplicate[j] && duplicate(valid[i], valid[j], tolerance))
			{
				is_duplicate[j] = true;
				Issue issue;
				issue.type = IssueType::Duplicate;
				issue.shape = valid[j];
				issue.point = valid[j]->type() == GraphicType::Circle ? ((Circle*)valid[j])->center() : valid[j]->first();
				result.push_back(issue);
			}
		}
	}

	// the ends of the shapes which are left (a duplicate would hide a free end)
	struct End
	{
		Shape* shape;
		glm::vec2 point;
	};
	std::vector<End> ends;
	for (size_t i = 0; i < valid.size(); i++)
	{
		Shape* s = valid[i];
		if (is_duplicate[i] || !has_ends(s))
			continue;
		// a closed shape (the end is the start) has no free end
		if (same(s->first(), s->last(), tolerance))
			continue;
		ends.push_back({ s, s->first() });
		ends.push_back({ s, s->last() });
	}

	for (size_t i = 0; i < ends.size(); i++)
	{
		bool touched = false;
		for (size_t j = 0; j < ends.size() && !touched; j++)
			touched = ends[j].shape != ends[i].shape && same(ends[i].point, ends[j].point, tolerance);

		if (!touched)
		{
			Issue issue;
			issue.type = IssueType::FreeEnd;
			issue.shape = ends[i].shape;
			issue.point = ends[i].point;
			result.push_back(issue);
		}
	}

	return result;
}

std::vector<Shape*> cad_diag::contour(const std::vector<Shape*>& selection, std::vector<Layer*>& layers, float tolerance)
{
	std::vector<Shape*> result = selection;

	std::vector<Shape*> candidates;
	for (Layer* layer : layers)
		if (layer->visible())
			for (Shape* s : layer->shapes())
				if (has_ends(s) && std::find(result.begin(), result.end(), s) == result.end())
					candidates.push_back(s);

	// each shape which joins the contour is then used to find the next ones
	for (size_t k = 0; k < result.size(); k++)
	{
		Shape* current = result[k];
		if (!has_ends(current))
			continue;

		auto it = candidates.begin();
		while (it != candidates.end())
		{
			Shape* other = *it;
			bool touch = same(current->first(), other->first(), tolerance) || same(current->first(), other->last(), tolerance)
				|| same(current->last(), other->first(), tolerance) || same(current->last(), other->last(), tolerance);
			if (touch)
			{
				result.push_back(other);
				it = candidates.erase(it);
			}
			else
				++it;
		}
	}

	return result;
}

std::string cad_diag::summary(const std::vector<Issue>& issues, std::string zero_length, std::string duplicate, std::string free_end)
{
	int zero = 0, duplicates = 0, free = 0;
	for (const Issue& i : issues)
	{
		if (i.type == IssueType::ZeroLength)
			zero++;
		else if (i.type == IssueType::Duplicate)
			duplicates++;
		else
			free++;
	}

	std::string result;
	auto add = [&](int count, const std::string& label)
	{
		if (count > 0)
			result += (result.empty() ? "" : ", ") + label + ": " + std::to_string(count);
	};
	add(zero, zero_length);
	add(duplicates, duplicate);
	add(free, free_end);
	return result;
}
