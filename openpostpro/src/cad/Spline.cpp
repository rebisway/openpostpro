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

#include "Spline.h"
#include <imgui.h>
#include <lang.h>
#include <strings.h>
#include <config.h>
#include <logger.h>
#include<chrono>

void Spline::add(glm::vec2 point)
{
	_curve.add(point);
}

void Spline::add(glm::vec2 point, glm::vec2 center, float radius, bool cw, SegmentType type)
{
	_curve.add(type, point, center, radius, cw);
}

void Spline::add(glm::vec2 center, float radius, bool cw, SegmentType type)
{
	_curve.add(center, radius, cw);
}

void Spline::add(Curve& c)
{
	for (Segment& s : c)
	{
		if (s.type == SegmentType::Line)
			add(s.point);
		else
			add(s.point, s.center, s.radius, s.cw, s.type);
	}
}

void Spline::close()
{
	if (first() != last())
	{
		add(first());
	}
}

Spline::Spline(Renderer* r) : Shape(r)
{
}

Spline::~Spline()
{
	// deleting graphic buffer
	_render->delete_buffer(_buffer);
	_render->delete_buffer(_untrim_buffer);
	_render->delete_buffer(_inter_buffer);
}

Shape* Spline::clone()
{
	return nullptr;
	Spline* s = new Spline(_render);
	for (auto c : _curve)
	{
		if (c.type == SegmentType::Line)
			s->add(c.point);
		else if (c.type == SegmentType::Arc)
			s->add(c.point, c.center, c.radius, c.cw);
		else
			s->add(c.center, c.radius, c.cw);
	}
	s->done(true);
	s->compute();
	return s;
}

std::vector<glm::vec2> Spline::coordinates()
{
	return _curve.coordinates();
}

void Spline::reverse()
{
	_curve.reverse();
	update();
}

glm::vec2 Spline::first()
{
	return _curve.first();
}

glm::vec2 Spline::last()
{
	return _curve.last();
}

void Spline::transform(glm::tmat4x4<float> mat)
{
	for (int i = 0; i < _curve.size(); i++)
	{
		_curve[i].point = mat * glm::vec4(_curve[i].point.x, _curve[i].point.y, 1, 1);
		if (_curve[i].type != SegmentType::Line)
			_curve[i].center = mat * glm::vec4(_curve[i].center.x, _curve[i].center.y, 1, 1);
	}

	for (int i = 0; i < _coordinates.size(); i++)
		_coordinates[i] = mat * glm::vec4(_coordinates[i].x, _coordinates[i].y, 1, 1);

	update();
}

bool Spline::is_over(glm::vec2 point)
{
	auto p = PRECISION * 2;
	// check if distance of first ref_point to mouse is less than precision
	if (glm::distance(first(), point) < p)
		return true;
	// check if distance of last ref_point to mouse is less than precision
	if (glm::distance(last(), point) < p)
		return true;
	// check if mouse inside clip and if distance "mouse to the line" is less than precision
	if (_bounds.offset(p).contains(point)) {
		glm::vec2 last = _coordinates.front();
		for (int i = 1; i < _coordinates.size(); i++)
		{
			auto next = _coordinates.at(i);
			if (geometry::foretriangle(last, next, point) < p)
			{
				if (glm::distance(last, point) + glm::distance(next, point) - glm::distance(last, next) < p)
					return true;
			}
			last = next;
		}
	}
	return false;
}

std::vector<glm::vec2> Spline::magnets()
{
	return std::vector<glm::vec2>({ _curve.first(), _curve.last() });
}

void Spline::scaled()
{
	if (_inter_points.size() > 0)
	{
		std::vector<glm::vec3> vertices;
		auto o = 20 / _render->camera()->scale();
		_inter_indices.clear();
		for (auto p : _inter_points)
		{
			auto coordinates = geometry::circle(o / 2, p, glm::min(1 / _render->camera()->scale(), o / 40));
			for (auto p : coordinates)
				vertices.push_back(geometry::v3(p));
			_inter_indices.push_back((int)vertices.size());
		}
		if (_inter_buffer == nullptr)
			_inter_buffer = _render->create_buffer(vertices);
		else
			_inter_buffer->flush(vertices);
	}
}

void Spline::compute()
{
	if (done())
	{
		// update clip rect
		_bounds.max();

		_coordinates = _curve.coordinates();

		for (glm::vec2 p : _coordinates)
		{
			_bounds.top_left.x = glm::min(_bounds.top_left.x, p.x);
			_bounds.top_left.y = glm::max(_bounds.top_left.y, p.y);
			_bounds.bottom_right.x = glm::max(_bounds.bottom_right.x, p.x);
			_bounds.bottom_right.y = glm::min(_bounds.bottom_right.y, p.y);
		}
	}
	//update();
	needs_update();
}

void Spline::update()
{
	std::vector<glm::vec3> vertices;
	for (glm::vec2 v : _coordinates)
		vertices.push_back(glm::vec3(v.x, v.y, 0));

 	if (_buffer == nullptr)
		_buffer = _render->create_buffer(vertices);
	else
		_buffer->flush(vertices);
}

void Spline::draw()
{
	Graphic::draw();

	if (_buffer == nullptr) return;

	if (done())
		_buffer->draw(_render->pr_line_strip());

	if (_untrim_buffer != nullptr /*&& _selected*/)
	{
		int from = 0, i = 1;
		if (_indices.size() > 0)
		{
			for (int index : _indices)
			{
				if (config.field_f4_toggled && ! config.field_f1_toggled)
				{
					_render->set_uniform("inner_color", config.colors[config.vividColorNames[0]]);
					_untrim_buffer->draw(_primitive, from, 2);
					_render->set_uniform("inner_color", config.colors[config.vividColorNames[i++]]);
					_untrim_buffer->draw(_primitive, from + 2, index - from);
					if (i == config.vividColorNames.size()) i = 1;
				}
				else
				{
					_render->set_uniform("inner_color", config.colors[config.colorNames[i++]]);
					_untrim_buffer->draw(_primitive, from, index - from + 2);
					if (i == config.colorNames.size()) i = 1;
				}
				from = index + 1;
			}
		}
		//else
		//{
		//	for (int j = 0; j < _untrim_buffer->size() / 2; j++)
		//	{
		//		_render->set_uniform("inner_color", config.colors[config.vividColorNames[j % 2 == 0 ? 0 : 1]]);
		//		_untrim_buffer->draw(_primitive, j*2, 2);
		//	}
		//}

		if (config.field_f5_toggled)
		{
			from = 0;
			for (auto to : _inter_indices)
			{
				_inter_buffer->draw(_render->pr_line_strip(), from, to - from);
				from = to;
			}
		}
	}
}

void Spline::ui()
{
	ImGui::Begin((_name + "###OBJECT").c_str());
	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("NB_POINTS"));
	ImGui::TableSetColumnIndex(1);
	ImGui::Spacing();
	ImGui::Text(std::to_string(_curve.size()).c_str());

	ImGui::EndTable();
	ImGui::End();
}

Shape* Spline::symmetry(glm::vec2 p1, glm::vec2 p2)
{
	Spline* s = new Spline(_render);
	for (auto c : _curve)
	{
		if (c.type == SegmentType::Line)
			s->add(geometry::symmetry(c.point, p1, p2));
		else if (c.type == SegmentType::Arc)
			s->add(geometry::symmetry(c.point, p1, p2), geometry::symmetry(c.center, p1, p2), c.radius, !c.cw);
		else
			s->add(geometry::symmetry(c.center, p1, p2), c.radius, !c.cw);
	}
	s->done(true);
	s->compute();
	return s;
}

Shape* Spline::symmetry(glm::vec2 center)
{
	Spline* s = new Spline(_render);
	for (auto c : _curve)
	{
		if (c.type == SegmentType::Line)
			s->add(geometry::symmetry(c.point, center));
		else if (c.type == SegmentType::Arc)
			s->add(geometry::symmetry(c.point, center), geometry::symmetry(c.center, center), c.radius, c.cw);
		else
			s->add(geometry::symmetry(c.center, center), c.radius, c.cw);
	}
	s->done(true);
	s->compute();
	return s;
}

std::string Spline::write()
{
	std::string result =
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		std::to_string((int)_curve.size()) + ';';


	for (Segment& s : _curve)
	{
		result += std::to_string((int)s.type) + ';' + stringex::to_string(s.point.x) + ';' + stringex::to_string(s.point.y) + ';';
		if (s.type != SegmentType::Line)
			result += stringex::to_string(s.center.x) + ';' + stringex::to_string(s.center.y) + ';' + stringex::to_string(s.radius) + ';' + std::to_string(s.cw) + ';';
	}

	return result.substr(0, result.size() - 1);
}

void Spline::read(std::string value, float version)
{
	_curve.clear();
	auto data = stringex::split(value, ';');
	if (data.size() >= 5)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		parent(data[2]);
		_name = data[3];
		auto count = std::stoi(data[4]);
		int i = 0, j = 5;
		while (i++ < count)
		{
			Segment s(glm::vec2(std::stof(data[j + 1]), std::stof(data[j + 2])));
			s.type = (SegmentType)std::stoi(data[j]);
			if (s.type != SegmentType::Line)
			{
				s.center.x = std::stof(data[j + 3]);
				s.center.y = std::stof(data[j + 4]);
				s.radius = std::stof(data[j + 5]);
				s.cw = (bool)std::stoi(data[j + 6]);
				j += 4;
			}
			j += 3;
			_curve.push_back(s);
		}
		done(true);
		//compute();
	}
}

/// <summary>
///  only for debug purpose
/// </summary>
/// <param name="o"></param>
void Spline::offset(float o)
{
	_selected = false;

	//if (_current_offset != o)
		_current_offset = o;
	//else
	//	return;

	auto t = std::chrono::high_resolution_clock::now();

	auto c = _curve;
	c.reduce(0.25);
	auto segments = c.untrim(o);

	//auto segments = _curve.untrim(o);

	std::vector<glm::vec3> vertices;
	_indices.clear();

	//////////////////////////// UNTRIM /////////////////////////////

	if (config.field_f3_toggled)
	{
		for (SegmentUntrim& s : segments)
		{
			if (s.type == SegmentType::Line)
			{
				auto dst = geometry::position(s.dst, 1 * geometry::distance(s.point, s.dst), s.point);
				vertices.push_back(glm::vec3(s.point.x, s.point.y, 0));
				vertices.push_back(glm::vec3(s.dst.x, s.dst.y, 0));
				//vertices.push_back(glm::vec3(dst.x, dst.y, 0));
				_indices.push_back((int)vertices.size() - 1);
			}
			else 
			{
				//_indices.push_back((int)vertices.size() - 1);
				std::vector<glm::vec2> coordinates;
				if (s.type == SegmentType::Arc)
					coordinates = geometry::arc(s.point, s.center, s.dst, s.cw, 0.25f);
				else // circle
					coordinates = geometry::circle(s.radius, s.center);
				auto from = coordinates.begin();
				auto it = from + 1;
				while (it != coordinates.end())
				{
					vertices.push_back(glm::vec3((*from).x, (*from).y, 0));
					vertices.push_back(glm::vec3((*it).x, (*it).y, 0));
					from = it;
					it++;
				}
				_indices.push_back((int)vertices.size() - 1);
			}
		}

		// adding original shape

		Segment from = _curve.back();
		auto it = _curve.begin();

		while (it < _curve.end())
		{
			Segment s = (*it);
			if (from.type == SegmentType::Line)
			{
				vertices.push_back(glm::vec3(from.point.x, from.point.y, 0));
				vertices.push_back(glm::vec3(s.point.x, s.point.y, 0));
				if (config.field_f1_toggled)
					_indices.push_back(((int)vertices.size()) - 1);
			}
			else
			{
				std::vector<glm::vec2> coordinates;
				if (from.type == SegmentType::Arc)
					coordinates = geometry::arc(from.point, from.center, s.point, from.cw, 0.25f);
				else // circle
					coordinates = geometry::circle(from.radius, from.center, 0.25f);
				auto from = coordinates.begin();
				auto it = from + 1;
				while (it != coordinates.end())
				{
					vertices.push_back(glm::vec3((*from).x, (*from).y, 0));
					vertices.push_back(glm::vec3((*it).x, (*it).y, 0));
					from = it;
					it++;
				}
				if (config.field_f1_toggled)
					_indices.push_back(((int)vertices.size()) - 1);
			}

			from = s;
			it++;
		}

		for (int i = 0; i < segments.size(); i++)
		{
			vertices.push_back(glm::vec3(segments[i].point, 0));
			vertices.push_back(glm::vec3(_curve[i].point, 0));
			vertices.push_back(glm::vec3(segments[i].dst, 0));
			vertices.push_back(glm::vec3(_curve[i + 1].point, 0));
		}
		_indices.push_back(((int)vertices.size()) - 1);

		//_indices.push_back((int)vertices.size() - 1);
		_primitive = _render->pr_lines();
		if (_untrim_buffer == nullptr)
			_untrim_buffer = _render->create_buffer(vertices);
		else
			_untrim_buffer->flush(vertices);

		return;
	}

	/////////////////////////// CLEAN UNTRIM //////////////////////////////////

	//segments = _curve.clean_untrim(segments, o);


	///////////////////////// TRIM ///////////////////////////////////////////

	auto trim = _curve.trim(segments, o);

	std::vector<Curve> process;
	std::vector<Curve> cleaned;

	int ccc = 1;

	while (ccc--)
	{
		if (config.field_f4_toggled)
			process.push_back(trim);
		else
			process = trim.split_at_intersections();

		if (config.field_f5_toggled)
		{
			if (config.field_f4_toggled)
				trim.split_at_intersections();

			_inter_points.clear();
			_inter_points = trim.intersection_points;

			scaled();
		}

		//////////////////////// CLEAN TRIM /////////////////////////////////////

		Curve::counter[0] = 0;
		Curve::counter[1] = 0;
		for (Curve c : process)
		{
			if (!config.field_f4_toggled && !config.field_f6_toggled)
			{
				if (!c.too_small(o) && !_curve.too_close(c, o))
					cleaned.push_back(c);
			}
			else
				cleaned.push_back(c);
		}
	}

	Logger::log("Process time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));

	Logger::log("clipping tests : " + std::to_string(Curve::counter[1]) + " ||| too close tests : " + std::to_string(Curve::counter[0]) + " ||| intersections : " + std::to_string(Curve::counter[5]));

	/////////////////////////////// DRAW ///////////////////////////////////

	Logger::log("nb=" + std::to_string(cleaned.size()));

	//int i = 0;
	////for (Curve result : process)
	for (Curve result : cleaned)
		{
			//Logger::log("-----------------------------------------------------------------");


			Segment from = result.back();
			auto it = result.begin();

			while (it < result.end())
			{
				Segment s = (*it);
				if (from.type == SegmentType::Line)
				{
					vertices.push_back(glm::vec3(from.point.x, from.point.y, 0));
					vertices.push_back(glm::vec3(s.point.x, s.point.y, 0));
					if (config.field_f1_toggled)
						_indices.push_back(((int)vertices.size()) - 1);
				}
				else
				{
					std::vector<glm::vec2> coordinates;
					if (from.type == SegmentType::Arc)
						coordinates = geometry::arc(from.point, from.center, s.point, from.cw, 0.25f);
					else // circle
						coordinates = geometry::circle(from.radius, from.center, 0.25f);
					auto from = coordinates.begin();
					auto it = from + 1;
					while (it != coordinates.end())
					{
						vertices.push_back(glm::vec3((*from).x, (*from).y, 0));
						vertices.push_back(glm::vec3((*it).x, (*it).y, 0));
						from = it;
						it++;
					}
					if (config.field_f1_toggled)
						_indices.push_back(((int)vertices.size()) - 1);
				}

				from = s;
				it++;
			}
			//int index = _indices.size() == 0 ? 0 : _indices.back() + 1;
			////if (vertices[index] != vertices.back())
			//	vertices.push_back(glm::vec3(result.front().ref_point.x, result.front().ref_point.y, 0));
			if (!config.field_f1_toggled)
				_indices.push_back(((int)vertices.size()) - 1);
	}
	_primitive = _render->pr_lines();
	

	if (_untrim_buffer == nullptr)
		_untrim_buffer = _render->create_buffer(vertices);
	else
		_untrim_buffer->flush(vertices);
}
