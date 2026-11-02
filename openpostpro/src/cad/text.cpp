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

#include "text.h"
#include "geometry.h"
#include "imgui.h"
#include <lang.h>
#include <strings.h>
#include <logger.h>
#include <font.h>
#include <environment.h>
#include <filesystem>
#include <history.h>

Text::Text(Renderer* r) : Shape(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) };
	_buffer = r->create_buffer(vertices);
	_a1 = new Anchor(r);

	_family = "arial";
	_height = 50.0f;
	_text = "no_text";
	_mirror = false;
}

Text::~Text()
{
	delete _a1;
	_render->delete_buffer(_buffer);
}

Shape* Text::clone()
{
	Text* t = new Text(_render);
	t->read(write());
	t->parent("");
	t->id(-1);
	return t;
}

std::vector<std::vector<glm::vec2>> Text::coordinates()
{
	return _transf;
}

void Text::text(std::string value) 
{ 
	if (_text != value)
	{
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());

		_text = value;
		_update_origin = true;
		compute();
	}
}

void Text::p1(glm::vec2 value)
{
	if (_p1 != value)
	{
		_p1 = value;
		compute();
	}
}

void Text::fontFamily(std::string value)
{
	if (_family != value)
	{
		_family = value;
		_update_origin = true;
		compute();
	}
}

void Text::italic(bool value)
{
	if (_italic != value)
	{
		_italic = value;
		_update_origin = true;
		compute();
	}
}

void Text::bold(bool value)
{
	if (_bold != value)
	{
		_bold = value;
		_update_origin = true;
		compute();
	}
}

void Text::inter(float value)
{
	if (_inter != value)
	{
		_inter = value;
		_update_origin = true;
		compute();
	}
}

void Text::height(float value) 
{
	if (_height != value) 
	{ 
		_height = value;
		_update_origin = true;
		compute();
	}
}

void Text::position(TextPos value)
{
	if (_position != value)
	{
		_position = value;
		_update_origin = true;
		compute();
	}
}

void Text::mirror(bool value)
{
	if (_mirror != value)
	{
		_mirror = value;
		_update_origin = true;
		compute();
	}
}

void Text::reference(int value)
{
	if (_reference != value)
	{
		_reference = value;
		if (_reference_path.size() > 0 && _reference != -1)
			compute();
	}
}

void Text::reference_path(std::vector<glm::vec2> path, bool closed)
{
	_reference_path.clear();
	_reference_length = 0;
	_reference_pos = 0;
	_reference_closed = closed;
	if (path.size() > 0)
	{
		_reference_path.insert(_reference_path.end(), path.begin(), path.end());
		for (int i = 0; i < _reference_path.size() - 1; i++)
			_reference_length += geometry::distance(_reference_path[i], _reference_path[i + 1]);
		if (_reference_path.size() > 0 && _reference != -1)
			compute();
	}
}

void Text::reference_pos(float value)
{
	if (_reference_pos != value)
	{
		_reference_pos = value;
		if (_reference_path.size() > 0 && _reference != -1)
			compute();
	}
}

void Text::set(std::string text, std::string family, bool italic, bool bold, float height) 
{ 
	if (_text != text || _family != family || _italic != italic || _bold != bold || _height != height)
	{
		_text = text;
		_family = family;
		_italic = italic;
		_bold = bold;
		_height = height;
		_update_origin = true;
		compute();
	}
}

void Text::transform(glm::tmat4x4<float> mat)
{
	if (_reference == -1)
	{
		_p1 = mat * glm::vec4(_p1.x, _p1.y, 1, 1);
		compute();
	}
}

void Text::move(glm::vec2 point)
{
	if (done())
	{
		if (_reference == -1)
			transform(glm::translate(glm::vec3(point - _p1, 0.0f)));
		else
		{
			auto p1 = _reference_point, p2 = point;
			auto l1 = std::numeric_limits<float>::max(), l2 = std::numeric_limits<float>::max();
			int i1 = 0, i2 = 0;
			for (int i = 0; i < _reference_path.size() - 1; i++)
			{
				auto p = geometry::projection(_reference_point, _reference_path[i], _reference_path[i + 1]);
				if (geometry::colinear_segment_point(_reference_path[i], _reference_path[i + 1], p, geometry::ERR_FLOAT3))
				{
					auto l = geometry::distance2(p, point);
					if (l < l1)
					{
						l1 = l;
						i1 = i;
						p1 = p;
					}
				}
				p = geometry::projection(point, _reference_path[i], _reference_path[i + 1]);
				if (geometry::colinear_segment_point(_reference_path[i], _reference_path[i + 1], p, geometry::ERR_FLOAT3))
				{
					auto l = geometry::distance2(p, point);
					if (l < l2)
					{
						l2 = l;
						i2 = i;
						p2 = p;
					}
				}
			}

			l1 = l2 = 0;
			// compute length on reference_path
			for (int i = 0; i < i1 && i < _reference_path.size() - 1; i++)
				l1 += geometry::distance(_reference_path[i], _reference_path[i + 1]);
			l1 += geometry::distance(_reference_path[i1], p1);

			for (int i = 0; i < i2 && i < _reference_path.size() - 1; i++)
				l2 += geometry::distance(_reference_path[i], _reference_path[i + 1]);
			l2 += geometry::distance(_reference_path[i1], p2);

			// compute difference
			float diff = 0.0f;
			int sign = 1;
			if (i1 < i2)
			{
				diff += geometry::distance(p1, _reference_path[i1 + 1]);
				for (int i = i1 + 1; i < i2 && i < _reference_path.size() - 2; i++)
					diff += geometry::distance(_reference_path[i], _reference_path[i + 1]);
				diff += geometry::distance(_reference_path[i2], p2);
			}
			else if (i1 > i2)
			{
				diff += geometry::distance(p2, _reference_path[i2 + 1]);
				for (int i = i2 + 1; i < i1 && i < _reference_path.size() - 2; i++)
					diff += geometry::distance(_reference_path[i], _reference_path[i + 1]);
				diff += geometry::distance(_reference_path[i1], p1);
				sign = -1;
			}
			else
			{
				diff += geometry::distance(p1, p2);
				sign = geometry::distance(_reference_path[i1], p1) < geometry::distance(_reference_path[i1], p2) ? 1 : -1;
			}

			_reference_pos += sign * diff;
			compute();
		}
	}
}

bool Text::point(glm::vec2 point)
{
	_p1 = point;
	_update_origin = true;
	done(true);
	compute();

	return true;
}

bool Text::is_over(glm::vec2 point)
{
	// check if mouse inside clip and if distance "mouse to the line" is less than precision
	if (_selected && _a1->is_over(point))
		return true;

	if (_bounds.contains(point)) {
		auto last = glm::vec2(_vertices.front().x, _vertices.front().y);
		for (int i = 1; i < _vertices.size(); i++)
		{
			auto next = glm::vec2(_vertices[i].x, _vertices[i].y);
			if (geometry::foretriangle(last, next, point) < PRECISION)
			{
				if (glm::distance(last, point) + glm::distance(next, point) - glm::distance(last, next) < PRECISION)
					return true;
			}
			last = next;
		}
	}
	return false;
}

Anchor* Text::anchor(glm::vec2 point)
{
	if (_a1->is_over(point))
	{
		_reference_point = point;
		return _a1;
	}
	return nullptr;
}

std::vector<Anchor*> Text::anchors()
{
	return std::vector<Anchor*>({_a1});
}

void Text::compute()
{
	if (!done())
		return;

	if (_update_origin)
	{
		if (_font != nullptr)
		{
			delete _font;
			_font = nullptr;
		}

		auto family = environment::fonts()[_family];
		if (_bold && _italic)
		{
			if (std::filesystem::exists(family.bold_italic_path))
				_font = new Font(_family, family.bold_italic_path, _bold, _italic);
		}
		else if (_bold)
		{
			if (std::filesystem::exists(family.bold_path))
				_font = new Font(_family, family.bold_path, _bold, _italic);
		}
		else if (_italic)
		{
			if (std::filesystem::exists(family.italic_path))
				_font = new Font(_family, family.italic_path, _bold, _italic);
		}
		if (_font == nullptr)
		{
			if (std::filesystem::exists(family.regular_path))
				_font = new Font(_family, family.regular_path, _bold, _italic);
			else
				Logger::log("ERROR: font " + family.name + "missing");
		}

	}

	if (_font != nullptr)
	{
		auto t = _text;
		if (_mirror)
			std::reverse(t.begin(), t.end());

		// bounds is an array of characheters bounding box
		// we use the vertical middle axis as reference for matrix operation to orientate the letter with the shape reference position
		auto bounds = _font->get_text_bounds(t, _height, _inter);
		auto length = bounds.back().right();

		std::string test = "é";

		if (_update_origin)
			_origin = _font->get_text_coordinates(t, _chars, _height, _inter);

		_update_origin = false;

		// this is the computation for a text sticked to a shape
		// it needs to be upgraded for allowing closed shapes and full rotation
		if (_reference != -1 && _reference_path.size() > 1)
		{
			_transf.clear();

			float y = 0;
			float h = (float)_font->get_glyph('o')->height();
			if (_position == TextPos::Middle)
				y = h / 2; // _font->height() / 2.0f;
			else if (_position == TextPos::Down)
				y = h; // _font->height() * 2.0f / 3.0f;

			// _origin reference is (0;0) coordinates
			// so we compute letter vertical axis coordinates for each letter
			std::vector<glm::vec2> axis;
			int from = 0;
			for (int to : _chars)
			{
				if (to != -1) // -1 is for non graphical characters such as space char.
				{
					axis.push_back(glm::vec2(bounds[axis.size()].left() + (bounds[axis.size()].right() - bounds[axis.size()].left()) / 2.0f, y));
					from = to + 1;
				}
				else
					axis.push_back(geometry::vec2_empty);
			}

			// we will follow reference_path and apply appropriate translation/rotation/translation
			std::vector<glm::mat4> matrices;
			for (int a = 0; a < axis.size(); a++)
			{
				float pos = glm::max<float>(0, glm::min(_reference_pos, _reference_length) - length);
				float dst_len = 0, cur_len = 0;
				for (int i = 0; i < _reference_path.size()-1; i++)
				{
					auto l = geometry::distance(_reference_path[i], _reference_path[i + 1]);
					if (dst_len + l < axis[a].x + pos) // if we are still less than the length where the letter should be positionned, we carry on
					{
						dst_len += l;
					}
					else
					{
						// so we compute final coordinates where letter axis will be moved, 
						// and the rotation angle which is pi/2 + angle(i, i+1)
						auto p = axis[a].x + pos - dst_len;
						auto position = geometry::position(_reference_path[i + 1], p, _reference_path[i]);
						auto angle = geometry::oriented_angle(_reference_path[i+1], _reference_path[i]);

						if (_mirror)
							angle = geometry::oriented_angle(angle + glm::pi<float>());

						// now we can compute the matrix
						auto m = glm::translate(glm::vec3(-axis[a].x, -axis[a].y, 1));
						m = glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f)) * m;
						m = glm::translate(glm::vec3(position.x, position.y, 1)) * m;

						matrices.push_back(m);

						if (a == 0)
							_reference_point = m * glm::vec4(0, 0, 1, 1);

						break;
					}
				}
			}

			int index = 0;
			from = 0;
			for (int to : _chars)
			{
				if (to != -1) // -1 is for non graphical characters such as space char.
				{
					for (int i = from; i <= to; i++)
					{
						std::vector<glm::vec2> c;
						for (auto v : _origin[i])
						{
							auto p = matrices[glm::min<int>(index, (int)matrices.size()-1)] * glm::vec4(v.x, v.y, 1, 1);
							c.push_back(glm::vec2(p.x, p.y));
						}
						_transf.push_back(c);
					}
					from = to + 1;
				}
				index++;
			}
		}
		else
		{
			// if there is no reference, then the _origin coordinates are just 
			// translated to _p1 coordinates, as original coordinates uses (0;0) as origin
			_pt = _p1;
			_transf.clear();
			for (std::vector<glm::vec2> p : _origin)
			{
				_transf.push_back(std::vector<glm::vec2>());
				for (glm::vec2 v : p)
					_transf.back().push_back(v + _pt);
			}
		}

		_indices.clear();
		_vertices.clear();
		for (std::vector<glm::vec2> poly : _transf)
		{
			std::vector<glm::vec3> vertices;
			vertices.reserve(poly.size());
			for (glm::vec2 v : poly)
				_vertices.push_back(glm::vec3(v.x, v.y, 0.0f));
			_indices.push_back((int)poly.size());
		}

	}

	// update clip rect
	_bounds.max();

	for (glm::vec2 p : _vertices)
	{
		_bounds.top_left.x = glm::min(_bounds.top_left.x, p.x);
		_bounds.top_left.y = glm::max(_bounds.top_left.y, p.y);
		_bounds.bottom_right.x = glm::max(_bounds.bottom_right.x, p.x);
		_bounds.bottom_right.y = glm::min(_bounds.bottom_right.y, p.y);
	}

	_bounds.top_left.x = glm::min(_bounds.top_left.x, _p1.x);
	_bounds.top_left.y = glm::max(_bounds.top_left.y, _p1.y);
	_bounds.bottom_right.x = glm::max(_bounds.bottom_right.x, _p1.x);
	_bounds.bottom_right.y = glm::min(_bounds.bottom_right.y, _p1.y);

	_bounds = _bounds.offset(PRECISION);

	Graphic::compute();
}

void Text::update()
{
	if (!done())
		return;

	if (_font != nullptr)
	{
		// update vertices
		_buffer->flush(_vertices);
	}

	// update anchors
	if (_reference != -1)
		_a1->point(_reference_point);
	else
		_a1->point(_pt);
}

void Text::draw()
{
	Graphic::draw();

	int previous = 0;
	for (int next : _indices)
	{
		_buffer->draw(_render->pr_line_strip(), previous, next);
		previous += next;
	}
}

void Text::draw_anchors()
{
	// draw anchors
	if (_selected /*&& _reference == -1*/)
		_a1->draw();
}

void Text::ui()
{
	auto families = environment::fonts();
	auto cur_fam = families[_family];

	char input[100];

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
		if (_reference == -1)
		{
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
							point(p);
						}
					}
					catch (const std::exception& e)
					{
						Logger::log(e.what());
					}
				}
			}
		}
		else
			ImGui::Text(input);

	ImGui::TableNextRow();
	if (_text.size() < 100)
		sprintf_s(input, "%s", _text.c_str());
	else
		sprintf_s(input, "%s", _text.substr(0, 99).c_str());
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("FONT_TEXT"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##FT_TEXT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
		{
		try
		{
			if (std::string(input) != _text)
			{
				History::undo(HistoryActionType::Modify, write());
				text(input);
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("FONT_FAMILY"));
	ImGui::TableSetColumnIndex(1);
	int cur_font_index = 0, n = 0;
	if (ImGui::BeginCombo("##FAMILY", _family.c_str(), 0))
	{
		std::map<std::string, environment::FontFamily>::iterator it;
		for (it = families.begin(); it != families.end(); it++)
		{
			const bool is_selected = (it->first == _family);
			if (ImGui::Selectable(it->first.c_str(), is_selected))
			{
				cur_font_index = n;
				if (it->first != _family)
				{
					History::undo(HistoryActionType::Modify, write());
					fontFamily(it->first);
				}
			}
			n++;

			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::TableNextRow();
	sprintf_s(input, "%.3f", _height);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("FONT_HEIGHT"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##FT_HEIGHT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto h = std::stof(input);
			if (h != _height)
			{
				History::undo(HistoryActionType::Modify, write());
				height(h);
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	if (!cur_fam.bold_path.empty() || !cur_fam.bold_italic_path.empty())
	{
		bool bd = _bold;
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("FONT_BOLD"));
		ImGui::TableSetColumnIndex(1);
		if (ImGui::Checkbox("##FT_BOLD", &bd))
		{
			if (bd != _bold)
			{
				History::undo(HistoryActionType::Modify, write());
				bold(bd);
			}
		}
	}

	if (!cur_fam.italic_path.empty() || !cur_fam.bold_italic_path.empty())
	{
		bool it = _italic;
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("FONT_ITALIC"));
		ImGui::TableSetColumnIndex(1);
		if (ImGui::Checkbox("##FT_ITALIC", &it))
		{
			if (it != _italic)
			{
				History::undo(HistoryActionType::Modify, write());
				italic(it);
			}
		}
	}

	ImGui::TableNextRow();
	sprintf_s(input, "%.3f", _inter);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("FONT_INTER"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##FT_INTER", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto i = std::stof(input);
			if (i != _inter)
			{
				History::undo(HistoryActionType::Modify, write());
				inter(i);
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	bool mr = _mirror;
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("FONT_MIRROR"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::Checkbox("##FT_MIRROR", &mr))
	{
		if (mr != _mirror)
		{
			History::undo(HistoryActionType::Modify, write());
			mirror(mr);
		}
	}

	if (_reference != -1)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("FONT_POSITION"));
		ImGui::TableSetColumnIndex(1);
		if (ImGui::BeginCombo("##FT_POSITION", Lang::l("FONT_POS" + std::to_string((int)_position)), 0))
		{
			for (int i = 0; i < 3; i++)
			{
				const bool is_selected = (i == (int)_position);
				if (ImGui::Selectable(Lang::l("FONT_POS" + std::to_string(i)), is_selected))
				{
					if (i != (int)_position)
					{
						History::undo(HistoryActionType::Modify, write());
						_position = (TextPos)i;
						compute();
					}
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextRow();
		sprintf_s(input, "%.3f", _reference_pos);
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("FONT_LENGTH"));
		ImGui::TableSetColumnIndex(1);
		if (ImGui::InputText("##REF_LENGTH", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			try
			{
				auto h = std::stof(input);
				if (h != _reference_pos)
				{
					History::undo(HistoryActionType::Modify, write());
					reference_pos(h);
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

std::string Text::write()
{
	std::string result =
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		parent() + ';' +
		_name + ';' +
		_family + ';' +
		stringex::to_escape(_text) + ';' +
		std::to_string(_italic) + ';' +
		std::to_string(_bold) + ';' +
		std::to_string(_height) + ';' +
		stringex::to_string(_p1.x) + ';' +
		stringex::to_string(_p1.y) + ';' +
		std::to_string(_reference) + ';' +
		stringex::to_string(_reference_pos) + ';' +
		std::to_string(_reference_closed) + ';' +
		std::to_string((int)_position) + ';' +
		std::to_string(_mirror) + ';' +
		stringex::to_string(_inter) + ';' +
		std::to_string(_reference_path.size()) + ';';

	for (int i = 0; i < _reference_path.size(); i++)
		result += std::to_string(_reference_path[i].x) + ';' + std::to_string(_reference_path[i].y) + ';';

	return result;
}

void Text::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');

	// data[0] = type
	_id = (unsigned int)std::stoi(data[1]);
	parent(data[2]);
	_name = data[3];
	_family = data[4];
	_text = stringex::from_escape(data[5]);
	_italic = (bool)std::stoi(data[6]);
	_bold = (bool)std::stoi(data[7]);
	_height = std::stof(data[8]);
	_p1.x = std::stof(data[9]);
	_p1.y = std::stof(data[10]);
	_reference = std::stoi(data[11]);
	_reference_pos = std::stof(data[12]);
	_reference_closed = (bool)std::stoi(data[13]);
	_position = (TextPos)std::stoi(data[14]);
	_mirror = (bool)std::stoi(data[15]);
	_inter = std::stof(data[16]);

	int count = std::stoi(data[17]) * 2;
	
	_reference_path.clear();
	for (int i = 0; i < count; i+=2)
	{
		float x = std::stof(data[18 + i]);
		float y = std::stof(data[18 + i + 1]);
		_reference_path.push_back(glm::vec2(x, y));
	}
	_update_origin = true;

	_reference_length = 0;
	if (count)
	{
		for (int i = 0; i < _reference_path.size() - 1; i++)
			_reference_length += geometry::distance(_reference_path[i], _reference_path[i + 1]);
	}

	done(true);
	compute();
}