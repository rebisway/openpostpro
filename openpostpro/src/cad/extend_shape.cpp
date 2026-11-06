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

#include "extend_shape.h"
#include "extend.h"
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <ellipse.h>
#include <polyline.h>
#include <spline.h>

Curve cad_extend::curve_of(Shape* s)
{
	Curve c;
	switch (s->type())
	{
	case GraphicType::Line:
		c.add(((Line*)s)->p1());
		c.add(((Line*)s)->p2());
		break;
	case GraphicType::Arc:
		c.add(((Arc*)s)->start(), ((Arc*)s)->center(), ((Arc*)s)->radius(), ((Arc*)s)->cw());
		c.add(((Arc*)s)->stop());
		break;
	case GraphicType::Circle:
		c.add(((Circle*)s)->center(), ((Circle*)s)->radius(), false);
		break;
	case GraphicType::Polyline:
		for (glm::vec2 p : ((Polyline*)s)->coordinates())
			c.add(p);
		break;
	case GraphicType::Ellipse:
		for (glm::vec2 p : ((Ellipse*)s)->coordinates())
			c.add(p);
		break;
	case GraphicType::Spline:
		c = ((Spline*)s)->curve();
		break;
	default:
		break;
	}
	return c;
}

namespace
{
	// the entities which stop a prolongation, as curves
	std::vector<Curve> boundaries_of(std::vector<Layer*>& layers, Shape* excluded)
	{
		std::vector<Curve> result;
		for (Layer* layer : layers)
		{
			if (!layer->visible())
				continue;
			for (Shape* s : layer->shapes())
			{
				if (s == excluded)
					continue;
				Curve c = cad_extend::curve_of(s);
				if (c.size() > 1)
					result.push_back(c);
			}
		}
		return result;
	}
}

int cad_extend::find(std::vector<Layer*>& layers, glm::vec2 point, Shape*& target, bool& at_start, glm::vec2& result)
{
	// the line or the arc under the mouse (the same search as for the hovered shape)
	target = nullptr;
	for (auto layer = layers.rbegin(); target == nullptr && layer != layers.rend(); ++layer)
	{
		if (!(*layer)->visible())
			continue;
		for (auto s = (*layer)->shapes().rbegin(); target == nullptr && s != (*layer)->shapes().rend(); ++s)
			if ((*s)->bounds().contains(point) && (*s)->is_over(point))
				target = *s;
	}

	if (target == nullptr || (target->type() != GraphicType::Line && target->type() != GraphicType::Arc))
	{
		target = nullptr;
		return 0;
	}

	// the end to prolong is the nearest one to the mouse
	at_start = glm::distance(point, target->first()) <= glm::distance(point, target->last());
	std::vector<Curve> boundaries = boundaries_of(layers, target);
	bool found = false;

	if (target->type() == GraphicType::Line)
	{
		Line* line = (Line*)target;
		found = at_start ? cad_extend::line(line->p2(), line->p1(), boundaries, result) : cad_extend::line(line->p1(), line->p2(), boundaries, result);
	}
	else
	{
		Arc* arc = (Arc*)target;
		// the start of an arc is prolonged by prolonging the reversed arc
		found = at_start ? cad_extend::arc(arc->stop(), arc->center(), arc->start(), !arc->cw(), boundaries, result) : cad_extend::arc(arc->start(), arc->center(), arc->stop(), arc->cw(), boundaries, result);
	}
	return found ? 2 : 1;
}
