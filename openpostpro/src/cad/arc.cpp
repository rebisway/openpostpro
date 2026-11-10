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

#include "arc.h"
#include "geometry.h"
#include "imgui.h"
#include "lang.h"
#include "stringex.h"
#include "logger.h"
#include "config.h"
#include <history.h>



Arc::Arc(Renderer* r) : Shape(r)
{
	_a1 = new Anchor(r);
	_a2 = new Anchor(r);
	_a3 = new Anchor(r);
}

Arc::~Arc()
{
	// deleting anchors
	delete _a1;
	delete _a2;
	delete _a3;

	// deleting graphic buffer
	_render->delete_buffer(_buffer);

	//_render->delete_buffer(_test);
}

Shape* Arc::clone()
{
	Arc* arc = new Arc(_render);
	arc->start(_start);
	arc->center(_center);
	arc->stop(_stop);
	arc->cw(cw());
	arc->done(true);
	arc->compute();
	return arc;
}

void Arc::reverse()
{
	auto p = _start;
	_start = _stop;
	_stop = p;
	_cw = !_cw;
	
	compute();
}

glm::vec2 Arc::first()
{
	return _start;
}

void Arc::first(glm::vec2 value)
{
	start(value);
}

glm::vec2 Arc::last()
{
	return _stop;
}

void Arc::last(glm::vec2 value)
{
	stop(value);
}

void Arc::start(glm::vec2 value)
{
	if (_start != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_start = value;
		if (_stop != geometry::vec2_empty && _center != geometry::vec2_empty)
		{
			if (glm::abs(_start.x - _stop.x) < 0.000001)
				_stop.x = _start.x;
			if (glm::abs(_start.y - _stop.y) < 0.000001)
				_stop.y = _start.y;
			done(true);
			compute();
		}
	}
}

void Arc::center(glm::vec2 value)
{
	if (_center != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_center = value;
		if (_start != geometry::vec2_empty && _stop != geometry::vec2_empty)
		{
			done(true);
			compute();
		}
	}
}

void Arc::stop(glm::vec2 value)
{
	if (_stop != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_stop = value;
		if (_start != geometry::vec2_empty && _center != geometry::vec2_empty)
		{
			if (glm::abs(_start.x - _stop.x) < 0.000001)
				_stop.x = _start.x;
			if (glm::abs(_start.y - _stop.y) < 0.000001)
				_stop.y = _start.y;
			done(true);
			compute();
		}
	}
}

void Arc::cw(bool value)
{
	if (_cw != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_cw = value;
		done(true);
		compute();
	}
}

void Arc::set(glm::vec2 s, glm::vec2 c, glm::vec2 d, bool cw)
{
	if (_start != s || _center != c || _stop != d || _cw != cw)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_start = s;
		_center = c;
		_stop = d;
		if (glm::abs(_start.x - _stop.x) < 0.00001)
			_stop.x = _start.x;
		if (glm::abs(_start.y - _stop.y) < 0.00001)
			_stop.y = _start.y;
		_cw = cw;
		done(true);
		compute();
	}
}

void Arc::transform(glm::tmat4x4<float> mat)
{
	_start = mat * glm::vec4(_start.x, _start.y, 1, 1);
	_stop = mat * glm::vec4(_stop.x, _stop.y, 1, 1);
	_center = mat * glm::vec4(_center.x, _center.y, 1, 1);
	compute();
}

void Arc::move(glm::vec2 point)
{
	if (done())
	{
		if (_a1 == modification()) // _start
		{
			// move start ref_point around the circle
			auto angle = geometry::oriented_angle(point, _center);
			_start = geometry::position(angle, _radius, _center);
		}
		else if (_a3 == modification()) // _stop
		{
			// move stop ref_point around the circle
			auto angle = geometry::oriented_angle(point, _center);
			_stop = geometry::position(angle, _radius, _center);
		}
		else // center
		{
			// move center along the mediatrice of (_start,_stop)
			auto middle = geometry::middle(_start, _stop);
			_center = geometry::projection(point, _center, middle);
		}
		compute();
	}
	else
	{
		if (_mode == ArcMode::Center)
		{
			if (_center == geometry::vec2_empty)
				return;

			if (_stop == geometry::vec2_empty)
				_start = point;
			else
			{
				_radius = glm::distance(_center, _start);
				_stop = geometry::position(geometry::oriented_angle(point, _center), _radius, _center);
				Logger::trace("angle=" + std::to_string(geometry::oriented_angle(point, _center)));
			}
		}
		else // ArcMode::ThreePoints
		{
			if (_points_count == 0)
				return;
			
			if (_points_count == 1)
				_stop = point;
			else
			{
				_center = geometry::circle_center(_start, _stop, point);
				_radius = glm::distance(_center, _start);

				if (!geometry::arc_point(_start, _center, _stop, _cw, point, geometry::ERR_FLOAT3))
					_cw = !_cw;
			}

		}
		compute();
	}
}

bool Arc::point(glm::vec2 point)
{
	if (_mode == ArcMode::Center)
	{
		if (_start == geometry::vec2_empty)
			_center = point;
		else if (_stop == geometry::vec2_empty)
			_stop = _start = point;
		else
		{
			// _stop ref_point must keep radius		
			_radius = glm::distance(_center, _start);
			_stop = geometry::position(geometry::oriented_angle(point, _center), _radius, _center);
			Logger::trace("angle=" + std::to_string(geometry::angle(point, _center)));
			done(true);
			compute();
			return true;
		}
	}
	else
	{
		if (_points_count == 0)
		{
			_start = point;
			_points_count++;
		}
		else if (_points_count == 1)
		{
			_stop = point;
			_points_count++;
		}
		else
		{
			_center = geometry::circle_center(_start, _stop, point);
			_radius = glm::distance(_center, _start);
			_points_count++;
			done(true);
			compute();
			return true;
		}
	}
	return false;
}

bool Arc::is_over(glm::vec2 point)
{
	for (glm::vec3 p : _vertices)
	{
		if (geometry::distance2(point, p) < PRECISION)
			return true;
	}
	return false;
}

Anchor* Arc::anchor(glm::vec2 point)
{
	if (_a1->is_over(point))
		return _a1;
	if (_a2->is_over(point))
		return _a2;
	if (_a3->is_over(point))
		return _a3;
	return nullptr;
}

std::vector<Anchor*> Arc::anchors()
{
	return std::vector<Anchor*>({_a1, _a2, _a3});
}

std::vector<glm::vec2> Arc::magnets()
{
	if (done())
		return std::vector<glm::vec2>({ _a1->point(), _a2->point(), _a3->point(), geometry::arc_middle(_start, _center, _stop, _radius, _cw)});
	else if (_center != geometry::vec2_empty && _start != geometry::vec2_empty)
		return std::vector<glm::vec2>({ _center, _start });
	else if (_center != geometry::vec2_empty )
		return std::vector<glm::vec2>({ _center });
	else
		return std::vector<glm::vec2>();
}

Shape* Arc::symmetry(glm::vec2 p1, glm::vec2 p2)
{
	Arc* arc = new Arc(_render);
	arc->start(geometry::symmetry(_start, p1, p2));
	arc->center(geometry::symmetry(_center, p1, p2));
	arc->stop(geometry::symmetry(_stop, p1, p2));
	arc->cw(!cw());
	arc->done(true);
	arc->compute();
	return arc;
}

Shape* Arc::symmetry(glm::vec2 center)
{
	Arc* arc = new Arc(_render);
	arc->start(geometry::symmetry(_start, center));
	arc->center(geometry::symmetry(_center, center));
	arc->stop(geometry::symmetry(_stop, center));
	arc->cw(cw());
	arc->done(true);
	arc->compute();
	return arc;
}

void Arc::compute()
{
	if (done() || _stop != geometry::vec2_empty && _mode == ArcMode::Center || _center != geometry::vec2_empty && _mode == ArcMode::ThreePoints)
	{
		std::vector<glm::vec2> coordinates = geometry::arc(_start, _center, _stop, _cw, 0.25f);

		_radius = glm::distance(_center, _start);

		// update clip rect
		_bounds = geometry::arc_bounds(_start, _center, _stop, _radius, _cw);

		// update vertices
		//std::vector<glm::vec3> vertices;
		_vertices.clear();
		for (glm::vec2 v : coordinates)
			_vertices.push_back(glm::vec3(v.x, v.y, 0));

		// add vertices to draw selected lines
		_vertices.push_back(glm::vec3(_start, 0.0f));
		_vertices.push_back(glm::vec3(_center, 0.0f));
		_vertices.push_back(glm::vec3(_center, 0.0f));
		_vertices.push_back(glm::vec3(_stop, 0.0f));
	}
	else if (_stop == geometry::vec2_empty)
	{
		_vertices.clear();
		if (_mode == ArcMode::Center)
		{
			_vertices.push_back(glm::vec3(_center, 0.0f));
			_vertices.push_back(glm::vec3(_start, 0.0f));
		}
		else
		{
			if (_center == geometry::vec2_empty)
			{
				_vertices.push_back(glm::vec3(_start, 0.0f));
				_vertices.push_back(glm::vec3(_stop, 0.0f));
			}
			//else
			//{
			//	std::vector<glm::vec2> coordinates = geometry::arc(_start, _center, _stop, _cw, 0.5f);
			//	for (glm::vec2 v : coordinates)
			//		_vertices.push_back(glm::vec3(v.x, v.y, 0));
			//	
			//	// add vertices to draw selected lines
			//	_vertices.push_back(glm::vec3(_start, 0.0f));
			//	_vertices.push_back(glm::vec3(_center, 0.0f));
			//	_vertices.push_back(glm::vec3(_center, 0.0f));
			//	_vertices.push_back(glm::vec3(_stop, 0.0f));
			//}
		}
	}

	Graphic::compute();
}

void Arc::update()
{
	if (done() || _stop != geometry::vec2_empty)
	{
		if (_buffer == nullptr)
			_buffer = _render->create_buffer(_vertices);
		else
			_buffer->flush(_vertices);

		_a1->point(_start);
		_a2->point(_center);
		//_a2->ref_point(geometry::arc_middle(_start, _center, _stop, _radius, _cw));
		_a3->point(_stop);
	}
	else if (_stop == geometry::vec2_empty)
	{
		if (_buffer == nullptr)
			_buffer = _render->create_buffer(_vertices);
		else
			_buffer->flush(_vertices);
	}

	//if (done())
	//{
	//	std::vector<glm::vec3> v{ 
	//		glm::vec3(_bounds.top_left.x, _bounds.top_left.y, 0), 
	//		glm::vec3(_bounds.top_left.x, _bounds.bottom_right.y,0), 
	//		glm::vec3(_bounds.bottom_right.x, _bounds.bottom_right.y,0), 
	//		glm::vec3(_bounds.bottom_right.x, _bounds.top_left.y, 0),
	//		glm::vec3(_bounds.top_left.x, _bounds.top_left.y, 0) };
	//		if (_test == nullptr)
	//		_test = _render->create_buffer(v);
	//	else
	//		_test->flush(v);
	//}
}

void Arc::draw()
{
	Graphic::draw();

	if (_buffer == nullptr) return;

	glm::vec4 color = _render->get_uniform_vec4("inner_color");

	if (selected() && config.show_decoration || !done() && _stop != geometry::vec2_empty)
	{
		_render->set_uniform("inner_color", config.decorationColor);
		_buffer->draw(_render->pr_lines(), (int)_buffer->size() - 4, 4);
	}
	else if (_buffer->size() <= 4)
	{
		_render->set_uniform("inner_color", config.decorationColor);
		_buffer->draw(_render->pr_lines());
	}
	
	_render->set_uniform("inner_color", color);

	if (_buffer->size() > 4)
		_buffer->draw(_render->pr_line_strip(), 0, (int)_buffer->size() - 4);

	//if (_test != nullptr)
	//{
	//	_render->set_uniform("inner_color", config.colors["Yellow"]);
	//	_test->draw(_render->pr_line_strip());
	//}
}

void Arc::draw_anchors()
{
	// draw anchors
	if (_selected)
	{
		if (config.show_decoration)
		{
			_render->set_uniform("inner_color", config.decorationColor);
			_buffer->draw(_render->pr_lines(), (int)_buffer->size() - 4, 4);
		}

		_a1->draw();
		_a2->draw();
		_a3->draw();
	}
}

void Arc::properties()
{
	auto format = [](glm::vec2 p) { char t[40]; sprintf_s(t, "%0.3f;%0.3f", p.x, p.y); return std::string(t); };

	property_text(Lang::l("START"), format(_start));
	property_text(Lang::l("END"), format(_stop));

	auto center = _center;
	if (property_point(Lang::l("CENTER"), "##CENTER", center))
		modify([&] { auto diff = center - _center; set(_start + diff, center, _stop + diff, _cw); });

	auto radius = _radius;
	if (property_float(Lang::l("RADIUS"), "##RADIUS", radius))
		modify([&] {
			auto s_a = geometry::angle(_start - _center);
			auto s_o = geometry::angle(_stop - _center);
			set(geometry::position(s_a, radius, _center), _center, geometry::position(s_o, radius, _center), _cw);
		});

	auto angle = glm::degrees(geometry::oriented_angle(_start, _stop, _center, _cw));
	if (property_float(Lang::l("ANGLE"), "##ANGLE", angle))
		modify([&] {
			float sa = geometry::oriented_angle(_start, _center);
			stop(geometry::position(sa + glm::radians(angle), _radius, _center));
		});

	bool clock = _cw;
	if (property_bool(Lang::l("CLOCKWISE"), "##CW", clock))
		cw(clock);

	property_text(Lang::l("LENGTH"), std::to_string(geometry::arc_length(_start, _center, _stop, _cw)));
	property_text(Lang::l("CORDE"), std::to_string(geometry::distance(_start, _stop)));
	property_text(Lang::l("THICKNESS"), std::to_string(geometry::arc_thickness(_start, _center, _stop, _cw)));
}

std::string Arc::write()
{
	return
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		stringex::to_string(_start.x) + ';' +
		stringex::to_string(_start.y) + ';' +
		stringex::to_string(_center.x) + ';' +
		stringex::to_string(_center.y) + ';' +
		stringex::to_string(_stop.x) + ';' +
		stringex::to_string(_stop.y) + ';' +
		std::to_string(_cw) + ';' +
		WS(block());
}

void Arc::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	if (data.size() == 11 || data.size() == 12)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		parent(data[2]);
		_name = data[3];
		_start.x = std::stof(data[4]);
		_start.y = std::stof(data[5]);
		_center.x = std::stof(data[6]);
		_center.y = std::stof(data[7]);
		_stop.x = std::stof(data[8]);
		_stop.y = std::stof(data[9]);
		_cw = std::stoi(data[10]);
		RS(_block, 11);

		done(true);
		compute();
	}
}