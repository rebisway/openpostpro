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

#include "circle.h"
#include "geometry.h"
#include "imgui.h"
#include "logger.h"
#include <lang.h>
#include <strings.h>
#include <config.h>
#include <history.h>

Circle::Circle(Renderer* r) : Shape(r)
{
	_a1 = new Anchor(r);
	_a2 = new Anchor(r);
}

Circle::~Circle()
{
	// deleting anchors
	delete _a1;
	delete _a2;

	// deleting graphic buffer
	_render->delete_buffer(_buffer);
}

Shape* Circle::clone()
{
	Circle* c = new Circle(_render);
	c->center(_center);
	c->radius(_radius);
	c->done(true);
	c->compute();
	return c;
}

void Circle::center(glm::vec2 p)
{
	if (_center != p)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_center = p;
		if (_radius > 0)
		{
			done(true);
			compute();
		}
	}
}

void Circle::radius(float r)
{
	if (r > 0 && r != _radius) 
	{ 
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_radius = r;
		done(true); 
		compute(); 
	}
}

void Circle::set(glm::vec2 p, float r)
{
	if (_center != p || (r > 0 && r != _radius))
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_center = p;
		if (r > 0)
		{
			_radius = r;
			done(true);
			compute();
		}
	}
}

void Circle::transform(glm::tmat4x4<float> mat)
{
	auto p = glm::vec2(_center.x + _radius, _center.y);
	_center = mat * glm::vec4(_center.x, _center.y, 1, 1);
	p = mat * glm::vec4(p.x, p.y, 1, 1);
	auto r = geometry::distance(_center, p);
	if (r != _radius)
		_radius = _radius * (r / _radius);
	compute();
}

void Circle::move(glm::vec2 point)
{
	if (_center != geometry::vec2_empty)
	{
		if (modification() == _a1) // mouse on center, so we move the circle
			_center = point;
		else
			_radius = glm::distance(_center, point);
		compute();
	}
}

bool Circle::point(glm::vec2 point)
{
	if (_center == geometry::vec2_empty)
	{
		_center = point;
		return false;
	}
	else
	{
		_radius = glm::distance(_center, point);
		done(true);
		compute();
		return true;
	}
}

bool Circle::is_over(glm::vec2 point)
{
	float d1 = glm::abs(glm::distance(_center, point) - _radius);
	float d2 = glm::distance(_center, point);
	return (d1 < PRECISION || (_selected && d2  < PRECISION));
}

Anchor* Circle::anchor(glm::vec2 point)
{
	if (_a1->is_over(point))
		return _a1;
	if (_a2->is_over(point))
		return _a2;
	return nullptr;
}

std::vector<Anchor*> Circle::anchors()
{
	return std::vector<Anchor*>({_a1, _a2});
}

std::vector<glm::vec2> Circle::magnets()
{
	if (done())
	{
		return std::vector<glm::vec2>({
		   _a1->point(),
		   _a2->point(),
		   glm::vec2(_center.x - _radius, _center.y),
		   glm::vec2(_center.x, _center.y + _radius),
		   glm::vec2(_center.x, _center.y - _radius),
			});
	}
	else
		return std::vector<glm::vec2>();
}

Shape* Circle::symmetry(glm::vec2 p1, glm::vec2 p2)
{
	Circle* c = new Circle(_render);
	c->center(geometry::symmetry(_center, p1, p2));
	c->radius(_radius);
	c->done(true);
	c->compute();
	return c;
}

Shape* Circle::symmetry(glm::vec2 center)
{
	Circle* c = new Circle(_render);
	c->center(geometry::symmetry(_center, center));
	c->radius(_radius);
	c->done(true);
	c->compute();
	return c;
}

void Circle::compute()
{
	if (_center != geometry::vec2_empty)
	{
		_bounds = geometry::rectangle(_center.x - _radius, _center.y + _radius, _center.x + _radius, _center.y - _radius);

		std::vector<glm::vec2> coordinates = geometry::circle(_radius, 0.5f);

		// update vertices
		_vertices.clear();
		for (glm::vec2 v : coordinates)
			_vertices.push_back(glm::vec3(v.x + _center.x, v.y + _center.y, 0));
		_vertices.push_back(_vertices.front());
	}

	Graphic::compute();
}

void Circle::update()
{
	if (_center != geometry::vec2_empty)
	{
		if (_buffer == nullptr)
		{
			_buffer = _render->create_buffer(_vertices);
		}
		else
			_buffer->flush(_vertices);

		_a1->point(_center);
		_a2->point(glm::vec2(_center.x + _radius, _center.y));
	}
}

void Circle::draw()
{
	Graphic::draw();

	if (_buffer == nullptr) return;

	if (!done())
		_render->set_uniform("inner_color", config.decorationColor);

	_buffer->draw(_render->pr_line_strip());
}

void Circle::draw_anchors()
{
	// draw anchors
	if (_selected)
	{
		_a1->draw();
		_a2->draw();
	}
}

void Circle::ui()
{
	char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	sprintf_s(input, "%0.3f;%0.3f", _center.x, _center.y);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("CENTER"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	if (ImGui::InputText("##0", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string val = input;
		auto numbers = stringex::split(val, ';');
		auto point = _center;
		if (numbers.size() == 2)
		{
			try
			{
				point.x = std::stof(numbers[0]);
				point.y = std::stof(numbers[1]);
				if (point != _center)
				{
					History::undo(HistoryActionType::Modify, write());
					center(point);
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}

	ImGui::TableNextRow();
	sprintf_s(input, "%.3f", _radius);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("RADIUS"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##1", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto r = std::stof(input);
			if (r != _radius)
			{
				History::undo(HistoryActionType::Modify, write());
				radius(r);
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	ImGui::TableNextRow();
	sprintf_s(input, "%.3f", _radius*2);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("DIAMETER"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##2", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto r = std::stof(input) / 2.0f;
			if (r != _radius)
			{
				History::undo(HistoryActionType::Modify, write());
				radius(r);
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

std::string Circle::write()
{
	return
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		stringex::to_string(_center.x) + ';' +
		stringex::to_string(_center.y) + ';' +
		stringex::to_string(_radius);
}

void Circle::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	if (data.size() == 7)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		parent(data[2]);
		_name = data[3];
		_center.x = std::stof(data[4]);
		_center.y = std::stof(data[5]);
		_radius = std::stof(data[6]);

		done(true);
		compute();
	}
}
