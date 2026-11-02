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

#include "point.h"
#include "geometry.h"
#include "imgui.h"
#include <lang.h>
#include <strings.h>
#include <logger.h>
#include <history.h>
#include <config.h>

float Point::point_size = POINT_SIZE;

Point::Point(Renderer* r) : Shape(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) };
	_buffer = r->create_buffer(vertices);
	_a1 = new Anchor(r);
}

Point::~Point()
{
	delete _a1;
	_render->delete_buffer(_buffer);
}

Shape* Point::clone()
{
	Point* p = new Point(_render);
	p->p1(_p1);
	p->done(true);
	p->compute();
	return p;
}

glm::vec2 Point::p1() 
{ 
	return _p1; 
}

void Point::p1(glm::vec2 value) 
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

void Point::transform(glm::tmat4x4<float> mat)
{
	_p1 = mat * glm::vec4(_p1.x, _p1.y, 1, 1);
	compute();
}

bool Point::is_over(glm::vec2 point)
{
	// check if distance of p1 to mouse is less than precision
	return glm::distance(_p1, point) < Shape::precision;
}

Anchor* Point::anchor(glm::vec2 point)
{
	if (_a1->is_over(point))
		return _a1;
	return nullptr;
}

std::vector<Anchor*> Point::anchors()
{
	return std::vector<Anchor*>({_a1});
}

std::vector<glm::vec2> Point::magnets()
{
	if (done())
		return std::vector<glm::vec2>({ _a1->point() });
	else
		return std::vector<glm::vec2>();
}

Shape* Point::symmetry(glm::vec2 p1, glm::vec2 p2)
{
	Point* p = new Point(_render);
	p->p1(geometry::symmetry(_p1, p1, p2));
	p->done(true);
	p->compute();
	return p;
}

Shape* Point::symmetry(glm::vec2 center)
{
	Point* p = new Point(_render);
	p->p1(geometry::symmetry(_p1, center));
	p->done(true);
	p->compute();
	return p;
}

void Point::move(glm::vec2 point)
{
	_p1 = glm::vec2(point.x, point.y);
	compute();
}

bool Point::point(glm::vec2 point)
{
	_p1 = point;
	done(true);
	compute();

	return true;
}

void Point::compute()
{
	// update clip rect
	_bounds = geometry::rectangle(_p1.x - Shape::precision, _p1.y + Shape::precision, _p1.x + Shape::precision, _p1.y - Shape::precision);

	Graphic::compute();
}

void Point::scaled()
{
	update();
}

void Point::update()
{
	// update vertices
	if (config.show_point_as_cross)
	{
		std::vector<glm::vec3> vertices{
			glm::vec3(_p1.x - Point::point_size / _scale, _p1.y, 0.0f),
			glm::vec3(_p1.x + Point::point_size / _scale, _p1.y, 0.0f),
			glm::vec3(_p1.x, _p1.y - Point::point_size / _scale, 0.0f),
			glm::vec3(_p1.x, _p1.y + Point::point_size / _scale, 0.0f)
		};
		_buffer->flush(vertices);
	}
	else
	{
		std::vector<glm::vec3> vertices{
			glm::vec3(_p1.x, _p1.y, 0.0f)
		};
		_buffer->flush(vertices);
	}

	// update anchors
	_a1->point(_p1);
}

void Point::draw()
{
	Graphic::draw();

	float scale = _render->camera()->scale();
	if (scale != _scale)
	{
		_scale = scale;
		update();
	}

	if (done())
	{
		if (config.show_point_as_cross)
			_buffer->draw(_render->pr_lines());
		else
			_buffer->draw(_render->pr_points());
	}
}

void Point::draw_anchors()
{
	// draw anchors
	if (_selected)
		_a1->draw();
}

void Point::ui()
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
	ImGui::Text("Point");
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	if (ImGui::InputText("##LP1", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string val = input;
		auto numbers = stringex::split(val, ';');
		auto p = _p1;
		if (numbers.size() == 2)
		{
			try
			{
				p.x = std::stof(numbers[0]);
				p.y = std::stof(numbers[1]);
				if (p != _p1)
				{
					History::undo(HistoryActionType::Modify, write());
					p1(p);
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}

	ImGui::EndTable();
	ImGui::End();
}

std::string Point::write()
{
	return
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		stringex::to_string(_p1.x) + ';' +
		stringex::to_string(_p1.y);
}

void Point::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	if (data.size() == 6)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		parent(data[2]);
		_name = data[3];
		_p1.x = std::stof(data[4]);
		_p1.y = std::stof(data[5]);

		done(true);
		compute();
	}
}