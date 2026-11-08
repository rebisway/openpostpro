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

std::function<void(Text*)> Text::on_link_request;
std::function<void(Text*)> Text::on_refresh_request;
std::function<bool(const std::string&)> Text::on_button;

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
		for (int i = 0; i + 1 < _reference_path.size(); i++)
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
			for (int i = 0; i + 1 < _reference_path.size(); i++)
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
			for (int i = 0; i < i1 && i + 1 < _reference_path.size(); i++)
				l1 += geometry::distance(_reference_path[i], _reference_path[i + 1]);
			l1 += geometry::distance(_reference_path[i1], p1);

			for (int i = 0; i < i2 && i + 1 < _reference_path.size(); i++)
				l2 += geometry::distance(_reference_path[i], _reference_path[i + 1]);
			l2 += geometry::distance(_reference_path[i1], p2);

			// compute difference
			float diff = 0.0f;
			int sign = 1;
			if (i1 < i2)
			{
				diff += geometry::distance(p1, _reference_path[i1 + 1]);
				for (int i = i1 + 1; i < i2 && i + 2 < _reference_path.size(); i++)
					diff += geometry::distance(_reference_path[i], _reference_path[i + 1]);
				diff += geometry::distance(_reference_path[i2], p2);
			}
			else if (i1 > i2)
			{
				diff += geometry::distance(p2, _reference_path[i2 + 1]);
				for (int i = i2 + 1; i < i1 && i + 2 < _reference_path.size(); i++)
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

		auto family = environment::font_family(_family);
		// the file of the style which is asked, else the regular one, else any file of the family
		struct Candidate { const std::string* path; int index; };
		std::vector<Candidate> candidates;
		if (_bold && _italic)
			candidates.push_back({ &family.bold_italic_path, family.bold_italic_index });
		else if (_bold)
			candidates.push_back({ &family.bold_path, family.bold_index });
		else if (_italic)
			candidates.push_back({ &family.italic_path, family.italic_index });
		candidates.push_back({ &family.regular_path, family.regular_index });
		candidates.push_back({ &family.bold_path, family.bold_index });
		candidates.push_back({ &family.italic_path, family.italic_index });
		candidates.push_back({ &family.bold_italic_path, family.bold_italic_index });
		for (const Candidate& c : candidates)
		{
			if (_font != nullptr || c.path->empty() || !std::filesystem::exists(*c.path))
				continue;
			try
			{
				_font = new Font(_family, *c.path, _bold, _italic, c.index);
			}
			catch (...)
			{
				_font = nullptr;
			}
		}
		if (_font == nullptr)
			Logger::log("ERROR: font " + _family + " missing or not readable");

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

		// the text follows the path: each letter is moved to its place on the path and turned along it
		if (_reference != -1 && _reference_path.size() > 1)
		{
			_transf.clear();

			// a position read from an old file was the end of the text: it is now the start
			if (_reference_legacy)
			{
				_reference_pos = glm::max(0.0f, glm::min(_reference_pos, _reference_length) - length);
				_reference_legacy = false;
			}

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

			// position and direction of the path at a distance from its start. On a closed path the distance goes round; on an open
			// path the first and the last segment go on in a straight line, so a letter is never out of place when the text is longer than the path
			bool closed = _reference_closed || geometry::distance(_reference_path.front(), _reference_path.back()) < 0.001f;
			auto at = [&](float s, glm::vec2& position, float& angle)
			{
				if (closed && _reference_length > 0)
				{
					s = std::fmod(s, _reference_length);
					if (s < 0)
						s += _reference_length;
				}
				int segment = 0;
				float done = 0;
				int last = (int)_reference_path.size() - 2;
				for (int i = 0; i <= last; i++)
				{
					auto l = geometry::distance(_reference_path[i], _reference_path[i + 1]);
					if (l < 0.00001f && i != last)
						continue;		// a segment of length zero has no direction
					segment = i;
					if (i == last || done + l >= s)
						break;
					done += l;
				}
				position = geometry::position(_reference_path[segment + 1], s - done, _reference_path[segment]);
				angle = geometry::oriented_angle(_reference_path[segment + 1], _reference_path[segment]);
			};

			// we will follow reference_path and apply appropriate translation/rotation/translation
			std::vector<glm::mat4> matrices;
			for (int a = 0; a < axis.size(); a++)
			{
				glm::vec2 position;
				float angle = 0;
				at(_reference_pos + axis[a].x, position, angle);

				if (_mirror)
					angle = geometry::oriented_angle(angle + glm::pi<float>());

				// now we can compute the matrix
				auto m = glm::translate(glm::vec3(-axis[a].x, -axis[a].y, 1));
				m = glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f)) * m;
				m = glm::translate(glm::vec3(position.x, position.y, 1)) * m;

				matrices.push_back(m);

				if (a == 0)
					_reference_point = m * glm::vec4(0, 0, 1, 1);
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

// the actions on the link with a curve: icons above the table of the properties (a tooltip tells what each one does)
void Text::properties_toolbar()
{
	if (!on_button)
		return;
	if (_reference == -1)
	{
		if (on_button("TEXT_LINK_PICK") && on_link_request)
			on_link_request(this);
	}
	else
	{
		if (on_button("TEXT_REFRESH") && on_refresh_request)
			on_refresh_request(this);
		ImGui::SameLine();
		if (on_button("DRAWING_REMOVE_REFERENCE_TO_TEXT"))
			modify([&] { _reference = -1; compute(); });
	}
}

void Text::properties()
{
	auto families = environment::fonts();
	auto cur_fam = environment::font_family(_family);

	if (_reference == -1)
	{
		auto position = _p1;
		if (property_point("Point", "##LP1", position))
			modify([&] { point(position); });
	}
	else
	{
		char t[40];
		sprintf_s(t, "%0.3f;%0.3f", _p1.x, _p1.y);
		property_text("Point", t);
	}

	auto content = _text;
	if (property_string(Lang::l("FONT_TEXT"), "##FT_TEXT", content))
		modify([&] { text(content); });

	std::vector<std::string> names;
	int family = 0;
	for (auto& f : families)
	{
		if (f.first == cur_fam.name)
			family = (int)names.size();
		names.push_back(f.first);
	}
	if (property_combo(Lang::l("FONT_FAMILY"), "##FAMILY", family, names))
		modify([&] { fontFamily(names[family]); });

	auto height = _height;
	if (property_float(Lang::l("FONT_HEIGHT"), "##FT_HEIGHT", height))
		modify([&] { this->height(height); });

	if (!cur_fam.bold_path.empty() || !cur_fam.bold_italic_path.empty())
	{
		bool bd = _bold;
		if (property_bool(Lang::l("FONT_BOLD"), "##FT_BOLD", bd))
			modify([&] { bold(bd); });
	}

	if (!cur_fam.italic_path.empty() || !cur_fam.bold_italic_path.empty())
	{
		bool it = _italic;
		if (property_bool(Lang::l("FONT_ITALIC"), "##FT_ITALIC", it))
			modify([&] { italic(it); });
	}

	auto inter = _inter;
	if (property_float(Lang::l("FONT_INTER"), "##FT_INTER", inter))
		modify([&] { this->inter(inter); });

	bool mr = _mirror;
	if (property_bool(Lang::l("FONT_MIRROR"), "##FT_MIRROR", mr))
		modify([&] { mirror(mr); });

	if (_reference != -1)
	{
		int position = (int)_position;
		std::vector<std::string> positions;
		for (int i = 0; i < 3; i++)
			positions.push_back(Lang::l("FONT_POS" + std::to_string(i)));
		if (property_combo(Lang::l("FONT_POSITION"), "##FT_POSITION", position, positions))
			modify([&] { _position = (TextPos)position; compute(); });

		// the start of the text along the path; the undo step of a slider is pushed by the helper
		auto reference = _reference_pos;
		if (property_slider(Lang::l("FONT_LENGTH"), "##REF_LENGTH", reference, 0.0f, _reference_length))
			reference_pos(reference);
	}
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

	// 1: the position is the start of the text (older files have no marker: the position was its end)
	result += "1;";

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

	// an older file has no marker after the path: the position was the end of the text
	size_t marker = 18 + (size_t)count;
	_reference_legacy = _reference != -1 && !(marker < data.size() && data[marker] == "1");

	_reference_length = 0;
	if (count)
	{
		for (int i = 0; i + 1 < _reference_path.size(); i++)
			_reference_length += geometry::distance(_reference_path[i], _reference_path[i + 1]);
	}

	done(true);
	compute();
}