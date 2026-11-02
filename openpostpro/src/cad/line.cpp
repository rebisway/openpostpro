// OpenPostPro - www.openpostpro.org
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

#include "line.h"
#include "geometry.h"
#include "imgui.h"
#include <lang.h>
#include <strings.h>
#include <logger.h>
#include <config.h>
#include <history.h>

Line::Line(Renderer* r) : Shape(r)
{
	std::vector<glm::vec3> vertices { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) };
	_buffer = r->create_buffer(vertices);
	_a1 = new Anchor(r);
	_a2 = new Anchor(r);
}

Line::~Line()
{
	delete _a1;
	delete _a2;
	_render->delete_buffer(_buffer);
}

Shape* Line::clone()
{
	Line* l = new Line(_render);
	l->p1(_p1);
	l->p2(_p2);
	l->done(true);
	l->compute();
	return l;
}

void Line::reverse()
{
	auto p = _p1;
	_p1 = _p2;
	_p2 = p;
	compute();
}

glm::vec2 Line::first()
{
	return _p1;
}

void Line::first(glm::vec2 value)
{
	p1(value);
}

glm::vec2 Line::last()
{
	return _p2;
}

void Line::last(glm::vec2 value)
{
	p2(value);
}

void Line::p1(glm::vec2 value)
{
	if (_p1 != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_p1 = value;
		done(true);
		compute();
	}
}

void Line::p2(glm::vec2 value)
{
	if (_p2 != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_p2 = value;
		done(true);
		compute();
	}
}

void Line::set(glm::vec2 p1, glm::vec2 p2)
{
	if (_p1 != p1 || _p2 != p2)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_p1 = p1;
		_p2 = p2;
		done(true);
		compute();
	}
}

void Line::transform(glm::tmat4x4<float> mat)
{
	_p1 = mat * glm::vec4(_p1.x, _p1.y, 1, 1); 
	_p2 = mat * glm::vec4(_p2.x, _p2.y, 1, 1);
	compute();
}


void Line::move(glm::vec2 point)
{
	if (done())
	{ // modification
		if (_a1 == modification())
			_p1 = glm::vec2(point.x, point.y);
		else
			_p2 = glm::vec2(point.x, point.y);
	}
	else
	{ // creation
		if (_p1 != geometry::vec2_empty)
			_p2 = point;
	}
	compute();
}

bool Line::point(glm::vec2 point)
{
	if (_p1 == geometry::vec2_empty)
	{
		_p1 = point;
		return false;
	}
	else
	{
		_p2 = point;
		done(true);
		compute();
		return true;
	}
}


Anchor* Line::anchor(glm::vec2 point)
{
	if (_a1->is_over(point))
		return _a1;
	if (_a2->is_over(point))
		return _a2;
	return nullptr;
}

std::vector<Anchor*> Line::anchors()
{
	return std::vector<Anchor*>({_a1, _a2});
}

std::vector<glm::vec2> Line::magnets()
{
	std::vector<glm::vec2> m;
	if (modification()) // if the shape is currently modified, add the fix ref_point to allow vertical/horizontal magnet
	{
		if (modification() == _a1)
			m.push_back(_p2);
		if (modification() == _a2)
			m.push_back(_p1);
	}
	else if (done())
	{
		m.push_back(_p1);
		m.push_back(_p2);
		if (_p1.x == _p2.x) // if vertical or horizontal, add middle ref_point
			m.push_back(glm::vec2(_p1.x, (_p1.y + _p2.y) / 2));
		if (_p1.y == _p2.y)
			m.push_back(glm::vec2((_p1.x + _p2.x) / 2, _p1.y));
	}
	else if (_p1 != geometry::vec2_empty)
		m.push_back(_p1);

	return m;
}

bool Line::is_over(glm::vec2 point)
{
	// check if distance of p1 to mouse is less than precision
	if (glm::distance(_p1, point) < Shape::precision)
		return true;
	// check if distance of p2 to mouse is less than precision
	if (glm::distance(_p2, point) < Shape::precision)
		return true;
	// check if mouse inside clip and if distance "mouse to the line" is less than precision
	if (_bounds.contains(point) && geometry::foretriangle(_p1, _p2, point) < Shape::precision)
		return true;
	if (_selected)
		return anchor(point) != nullptr;
	return false;
}

void Line::compute()
{
	if (done())
	{
		// update clip rect
		_bounds = geometry::rectangle(glm::min(_p1.x, _p2.x), glm::max(_p1.y, _p2.y), glm::max(_p1.x, _p2.x), glm::min(_p1.y, _p2.y));

		// is vertical ? if true offset the clip rectangle
		if (_bounds.top_left.x == _bounds.bottom_right.x)
		{
			_bounds.top_left.x -= Shape::precision;
			_bounds.bottom_right.x += Shape::precision;
		}
		// is horizontal ? if true offset the clip rectangle
		if (_bounds.top_left.y == _bounds.bottom_right.y)
		{
			_bounds.top_left.y += Shape::precision;
			_bounds.bottom_right.y -= Shape::precision;
		}
	}

	Graphic::compute();
}

void Line::update()
{
	if (done())
	{
		// update vertices
		std::vector<glm::vec3> vertices{ glm::vec3(_p1.x, _p1.y, 0.0f), glm::vec3(_p2.x, _p2.y, 0.0f) };
		_buffer->update(vertices);

		// update anchors
		_a1->point(_p1);
		_a2->point(_p2);
	}
	else
	{
		// update vertices
		if (_p1 != geometry::vec2_empty)
		{
			std::vector<glm::vec3> vertices{ glm::vec3(_p1.x, _p1.y, 0.0f), glm::vec3(_p2.x, _p2.y, 0.0f) };
			_buffer->update(vertices);
		}
	}
}

void Line::draw()
{
	Graphic::draw();

	if (!done())
		_render->set_uniform("inner_color", config.decorationColor);

	// draw line
	if (_buffer != nullptr)
		_buffer->draw(_render->pr_lines());

}

void Line::draw_anchors()
{
	// draw anchors
	if (_selected)
	{
		_a1->draw();
		_a2->draw();
	}
}


void Line::ui()
{
	char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	sprintf_s(input, "%0.3f;%0.3f", _p1.x, _p1.y);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text("P1");
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	if (ImGui::InputText("##LP1", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string val = input;
		auto numbers = stringex::split(val, ';');
		auto point = _p1;
		if (numbers.size() == 2)
		{
			try
			{
				point.x = std::stof(numbers[0]);
				point.y = std::stof(numbers[1]);
				if (point != _p1)
				{
					History::undo(HistoryActionType::Modify, write());
					p1(point);
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}


	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0); 
	ImGui::Spacing();
	ImGui::Text("P2");
	ImGui::TableSetColumnIndex(1);
	sprintf_s(input, "%0.3f;%0.3f", _p2.x, _p2.y);
	if (ImGui::InputText("##LP2", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string val = input;
		auto numbers = stringex::split(val, ';');
		auto point = _p2;
		if (numbers.size() == 2)
		{
			try
			{
				point.x = std::stof(numbers[0]);
				point.y = std::stof(numbers[1]);
				if (point != _p2)
				{
					History::undo(HistoryActionType::Modify, write());
					p2(point);
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0); 
	ImGui::Spacing();
	ImGui::Text(Lang::l("LENGTH"));
	ImGui::TableSetColumnIndex(1);
	sprintf_s(input, "%.3f", glm::length(_p2 - _p1));
	if (ImGui::InputText("##LL", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto len = std::stof(input);
			auto angle = geometry::angle(_p2 - _p1);
			auto point = geometry::position(angle, len);
			if (point + _p1 != _p2)
			{
				History::undo(HistoryActionType::Modify, write());
				p2(point + _p1);
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}
	
	ImGui::EndTable();
	ImGui::End();
}

Shape* Line::symmetry(glm::vec2 p1, glm::vec2 p2)
{
	Line* l = new Line(_render);
	l->p1(geometry::symmetry(_p1, p1, p2));
	l->p2(geometry::symmetry(_p2, p1, p2));
	l->done(true);
	l->compute();
	return l;
}

Shape* Line::symmetry(glm::vec2 center)
{
	Line* l = new Line(_render);
	l->p1(geometry::symmetry(_p1, center));
	l->p2(geometry::symmetry(_p2, center));
	l->done(true);
	l->compute();
	return l;
}

std::string Line::write()
{
	return
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		stringex::to_string(_p1.x) + ';' +
		stringex::to_string(_p1.y) + ';' +
		stringex::to_string(_p2.x) + ';' +
		stringex::to_string(_p2.y);
}

void Line::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	if (data.size() == 8)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		parent(data[2]);
		_name = data[3];
		_p1.x = std::stof(data[4]);
		_p1.y = std::stof(data[5]);
		_p2.x = std::stof(data[6]);
		_p2.y = std::stof(data[7]);

		done(true);
		compute();
	}
}
