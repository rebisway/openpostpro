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

#include "toolpath.h"
#include <config.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <lang.h>


std::vector<int> Toolpath::references()
{
	return _references_id;
}

void Toolpath::references(std::vector<int> value)
{
	_references_id.clear();
	_references_id.insert(_references_id.end(), value.begin(), value.end());
}

void Toolpath::reference(int value)
{
	_references_id.push_back(value);
}

void Toolpath::start_point_type(StartPointType value)
{
	if (_start_point_type != value)
	{
		_start_point_type = value;
		compute();
	}
}

void Toolpath::start_point_length(float value)
{
	if (_start_point_length != value)
	{
		_start_point_length = value;
		compute();
	}
}

void Toolpath::start_point_offset(float value)
{
	if (_start_point_offset != value)
	{
		_start_point_offset = value;
		compute();
	}
}

void Toolpath::start_point_inside(bool value)
{
	if (_start_point_inside != value)
	{
		_start_point_inside = value;
		compute();
	}
}

void Toolpath::tabs_count(float value)
{
	if (_tabs_count != value)
	{
		_tabs_count = value;
		_tabs_positions.clear();		// evenly spread again
		_tab_selected = 0;
		compute();
	}
}

void Toolpath::tabs_length(float value)
{
	if (_tabs_length != value)
	{
		_tabs_length = value;
		compute();
	}
}

void Toolpath::tabs_height(float value)
{
	if (_tabs_height != value)
	{
		_tabs_height = value;
		compute();
	}
}

void Toolpath::tab_position(int index, float value)
{
	if (index >= 0 && index < (int)_tabs_positions.size() && _tabs_positions[index] != value)
	{
		_tabs_positions[index] = value;
		compute();
	}
}

std::string Toolpath::tabs_write()
{
	std::string result = WN(_tabs_positions.size());
	for (float p : _tabs_positions)
		result += WN(p);
	return result;
}

void Toolpath::tabs_read(const std::vector<std::string>& data, int index)
{
	_tabs_positions.clear();
	int count = 0;
	// older data have no positions: the tabs are evenly spread
	RI(count, index);
	for (int i = 0; i < count && index + 1 + i < (int)data.size(); i++)
		_tabs_positions.push_back(std::stof(data[index + 1 + i]));
}

Toolpath::Toolpath(Renderer* r) : Graphic(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f) };
	_data_buffer = r->create_buffer(vertices);
	_deco_buffer = r->create_buffer(vertices);
	_tab_buffer = r->create_buffer(vertices);
}

Toolpath::~Toolpath()
{
	if (_data_buffer != nullptr)
	{
		delete _data_buffer;
		_data_buffer = nullptr;
	}
	if (_tab_buffer != nullptr)
	{
		delete _tab_buffer;
		_tab_buffer = nullptr;
	}
	if (_tree != nullptr)
	{
		delete _tree;
		_tree = nullptr;
	}
}

void Toolpath::generate_startpoint(std::vector<Curve>& curves)
{
	if (start_point_allowed() && curves.size() == 1)
	{
		Curve& c = curves[0];
		if (c.closed())
		{
			// the original closed curve starts at length 0 and end at max_length which the curve length
			// start point is starting by default at 0
			// to move the start point along the curve, we just set the start_point_length
			// from this value, we look for the segment along the required length
			// we can split the targeted segment and reconstruct the new computed curve
			// it is a simple "rotation" along segments
			// if an line or arc start is requested, we add it at the begining of the computed curve
			
			// ensure that we do not loop several times the whole curve
			while (_start_point_length > c.length())
				_start_point_length -= c.length();
			while (_start_point_length < 0)
				_start_point_length += c.length();

			float pos = 0;
			int index = 0;
			auto segment = c.begin();
			auto next = c.begin() + 1;

			while (next != c.end() && (*segment).length + pos < _start_point_length)
			{
				pos += (*segment).length;
				segment = next;
				next = next + 1;

				index++;
			}

			Curve output;
			// segment_pos is the position where to split the segment
			// if segment_pos == 0, segment[index] is the new start segment
			// else, we split segment[index] at segment_pos, push the right part and start computed output
			// at the end, we push the left part
			float segment_pos = _start_point_length - pos;
			std::vector<Segment> segments;
			if (segment_pos > 0)
				segments = (*segment).split_at(segment_pos, (*next).point);
			else
				segments.push_back(*segment);
				
			output.push_back(segments.back());

			while (next != c.end())
			{
				output.push_back(*next);
				next = next + 1;
			}

			next = c.begin();
			while(next != segment)
			{
				output.push_back(*next);
				next = next + 1;
			}

			// the left part of the split segment closes the curve, whatever the index of the segment
			if (segment_pos > 0)
				output.push_back(segments.front());

			output.push_back(segments.back());

			// we have reconstructed a rotated version of _original[0] starting from _start_point_length
			// we just need to add the right segment at the curve begining

			output = generate_tabs(output, _start_point_length, true);

			if (_start_point_type != StartPointType::normal)
			{
				// direction of the toolpath at its first point, the tangent when the first segment is an arc
				float angle;
				if (output[0].type == SegmentType::Arc)
					angle = geometry::oriented_angle(output[0].point, output[0].center) + (output[0].cw ? -glm::half_pi<float>() : glm::half_pi<float>());
				else
					angle = geometry::oriented_angle(output[1].point, output[0].point);
				auto source = geometry::position(geometry::oriented_angle(angle + glm::half_pi<float>()), _start_point_offset, output[0].point);

				if (_start_point_inside != output.inside(source))
					source = geometry::position(geometry::oriented_angle(angle - glm::half_pi<float>()), _start_point_offset, output[0].point);

				if (_start_point_type == StartPointType::line)
				{
					output.insert(output.begin(), Segment(source));
				}
				else if (_start_point_type == StartPointType::arc)
				{
					bool clock = _start_point_inside ? output.cw() : !output.cw();
					auto centre = geometry::middle(source, output[0].point);
					output.insert(output.begin(), Segment(source, centre, _start_point_offset / 2.0f, clock));
				}
			}
			
			auto check = output.length();

			_computed.clear();
			_computed.push_back(output);
		}
	}
	else if (start_point_allowed())
	{
		// several curves : no start point, but the closed ones keep their tabs
		for (Curve& c : curves)
			if (c.closed())
				c = generate_tabs(c);
	}
}

void Toolpath::tabs_init_positions(float total)
{
	int count = (int)_tabs_count;
	if ((int)_tabs_positions.size() == count)
		return;
	_tabs_positions.clear();
	for (int i = 0; i < count; i++)
		_tabs_positions.push_back((i + 0.5f) * total / count);
}

Curve Toolpath::generate_tabs(Curve& curve, float offset, bool positions)
{
	int count = (int)_tabs_count;
	if (!tabs_allowed() || count <= 0 || _tabs_length <= 0 || _tabs_height <= 0 || curve.size() < 2 || !curve.closed())
		return curve;

	float total = curve.length();
	float spacing = total / count;
	float tab = glm::min(_tabs_length, spacing * 0.9f);
	if (tab <= 0)
		return curve;

	if (positions)
	{
		_tabs_total = total;
		tabs_init_positions(total);
	}

	// ranges along the curve (start, end, index of the tab). Without positions, each tab is centred in its own part of the curve.
	// A tab which crosses the start or the end of the curve is cut in two ranges.
	struct Range { float first; float second; int index; };
	std::vector<Range> ranges;
	for (int i = 0; i < count; i++)
	{
		float center = positions ? _tabs_positions[i] - offset : (i + 0.5f) * spacing;
		center = std::fmod(center, total);
		if (center < 0)
			center += total;
		float start = center - tab / 2.0f;
		float end = center + tab / 2.0f;
		if (start < 0)
		{
			ranges.push_back({ start + total, total, i });
			ranges.push_back({ 0, end, i });
		}
		else if (end > total)
		{
			ranges.push_back({ start, total, i });
			ranges.push_back({ 0, end - total, i });
		}
		else
			ranges.push_back({ start, end, i });
	}
	// index of the tab at the position p, -1 if none
	auto in_tab = [&](float p)
	{
		for (auto& r : ranges)
			if (p >= r.first && p <= r.second)
				return r.index;
		return -1;
	};

	const float eps = 0.0001f;
	Curve result;
	float pos = 0;
	for (size_t i = 0; i + 1 < curve.size(); i++)
	{
		Segment current = curve[i];
		glm::vec2 end = curve[i + 1].point;
		float length = current.length;
		float from = pos;
		float to = pos + length;

		while (true)
		{
			// next boundary strictly inside the remaining part of the segment
			float boundary = to + 1;
			for (auto& r : ranges)
			{
				if (r.first > from + eps && r.first < to - eps) boundary = glm::min(boundary, r.first);
				if (r.second > from + eps && r.second < to - eps) boundary = glm::min(boundary, r.second);
			}

			if (boundary > to)
			{
				current.tab_index = in_tab((from + to) / 2.0f);
				current.tab = current.tab_index >= 0;
				result.push_back(current);
				break;
			}

			auto parts = current.split_at(boundary - from, end);
			parts[0].tab_index = in_tab((from + boundary) / 2.0f);
			parts[0].tab = parts[0].tab_index >= 0;
			result.push_back(parts[0]);
			current = parts[1];
			from = boundary;
		}
		pos = to;
	}
	result.push_back(curve.back());
	result.length();
	return result;
}

void Toolpath::tabs_properties(const Curve& curve)
{
	int count = (int)_tabs_count;
	if (property_int(Lang::l("TABS_COUNT"), "##TABS_COUNT", count))
		modify([&] { tabs_count((float)glm::max(count, 0)); });

	float length = _tabs_length;
	if (property_float(Lang::l("TABS_LENGTH"), "##TABS_LENGTH", length))
		modify([&] { tabs_length(glm::max(length, 0.0f)); });

	float height = _tabs_height;
	if (property_float(Lang::l("TABS_HEIGHT"), "##TABS_HEIGHT", height))
		modify([&] { tabs_height(glm::max(height, 0.0f)); });

	// each tab can be moved on its own: choose it, then slide it along the curve
	if (count > 0)
	{
		float total = _tabs_total > 0 ? _tabs_total : const_cast<Curve&>(curve).length();
		tabs_init_positions(total);
		_tab_selected = glm::clamp(_tab_selected, 0, (int)_tabs_positions.size() - 1);

		int selected = _tab_selected + 1;
		if (property_int(Lang::l("TABS_SELECTED"), "##TABS_SELECTED", selected))
		{
			_tab_selected = glm::clamp(selected, 1, (int)_tabs_positions.size()) - 1;
			compute();
		}

		// the undo step of a slider is pushed by the helper
		float position = _tabs_positions[_tab_selected];
		if (property_slider(Lang::l("TABS_POSITION"), "##TABS_POSITION", position, 0, total))
			tab_position(_tab_selected, position);
	}
}

void Toolpath::generate_data(std::vector<Curve>& curves)
{
	_data_indices.clear();
	std::vector<glm::vec3> vertices;

	int count = 0;

	// we compute coordinates for each curve
	for (auto c : curves)
	{
		if (c.size() > 1)
		{
			count++;

			auto front = geometry::v3(c.front().point);
			auto from = c.begin();
			auto to = from + 1;

			if ((*from).type != SegmentType::Circle)
				vertices.push_back(geometry::v3((*from).point));

			while (to != c.end())
			{
				if ((*from).type == SegmentType::Line)
					vertices.push_back(geometry::v3((*to).point));
				else if ((*from).type == SegmentType::Arc)
				{
					auto v = geometry::arc((*from).point, (*from).center, (*to).point, (*from).cw, 1/_render->camera()->scale());
					auto p = v.begin() + 1;
					while (p != v.end())
					{
						vertices.push_back(geometry::v3((*p)));
						p = std::next(p);
					}
				}
				else if ((*from).type == SegmentType::Circle)
				{
					auto v = geometry::circle((*from).radius, (*from).center);
					auto p = v.begin();
					while (p != v.end())
					{
						vertices.push_back(geometry::v3((*p)));
						p = std::next(p);
					}
				}
				from = to;
				to = std::next(to);
			}

			_data_indices.push_back((int)vertices.size());

			// DEBUG
			//if (count > 500)
			//	break;
		}
	}

	_data_buffer->flush(vertices);
}

void Toolpath::generate_deco(std::vector<Curve>& curves)
{
	_deco_indices.clear();
	std::vector<glm::vec3> vertices;

	if (config.show_cam_arrow)
	{
		auto o = 10 / _render->camera()->scale();

		// we add the arrow to display toolpath direction
		for (auto c : curves)
		{
			float length = 25 / _render->camera()->scale();
			float pos = 0;
			int index = 0;
			auto segment = c.begin();
			auto next = c.begin() + 1;

			// if _start_point_type != normal, we create arrow on second segment for better lisibility
			if (_start_point_type != StartPointType::normal)
			{
				segment = next;
				next = next + 1;
			}

			while (next != c.end() && (*segment).length + pos < length)
			{
				pos += (*segment).length;
				segment = next;
				next = next + 1;

				index++;
			}
			glm::vec2 position = geometry::vec2_empty;
			float angle = 0;

			auto s1 = (*segment), s2 = (segment == c.end()-1 ? c.front() : (*(segment + 1)));
			float segment_pos = length - pos;
			auto segments = s1.split_at(segment_pos, s2.point);

			if (s1.type == SegmentType::Line)
			{
				position = segments.back().point;
				angle = geometry::oriented_angle(s2.point, s1.point);
			}
			else if (s1.type == SegmentType::Circle)
			{
				position = glm::vec2(s1.center.x - s1.radius, s1.point.y);
				angle = s1.cw ? glm::half_pi<float>() : -glm::half_pi<float>();
			}
			else// arc
			{
				position = geometry::arc_middle(s1.point, s1.center, s2.point, s1.cw);
				angle = geometry::oriented_angle(position, s1.center) + (s1.cw ? -glm::half_pi<float>() : glm::half_pi<float>());
			}

			// we compute the arrow
			auto p1 = geometry::position(angle + glm::quarter_pi<float>()/2, -o, position);
			auto p2 = geometry::position(angle + -glm::quarter_pi<float>()/2, -o, position);
			
			vertices.push_back(glm::vec3(p1.x, p1.y, 0));
			vertices.push_back(glm::vec3(position.x, position.y, 0));
			vertices.push_back(glm::vec3(p2.x, p2.y, 0));
			_deco_indices.push_back((int)vertices.size());
		}

		if (config.show_cam_start)
		{
			auto o = 3.0f / _render->camera()->scale();
			for (auto c : curves)
			{
				auto p = c.front().point;
				vertices.push_back(glm::vec3(p.x - o, p.y - o, 0));
				vertices.push_back(glm::vec3(p.x + o, p.y + o, 0));
				_deco_indices.push_back((int)vertices.size());
				vertices.push_back(glm::vec3(p.x - o, p.y + o, 0));
				vertices.push_back(glm::vec3(p.x + o, p.y - o, 0));
				_deco_indices.push_back((int)vertices.size());
			}
		}
		
		_deco_buffer->flush(vertices);
	}
}

void Toolpath::generate_tabs_deco(std::vector<Curve>& curves)
{
	_tab_indices.clear();
	std::vector<glm::vec3> vertices;

	for (auto& c : curves)
	{
		int open = -1;		// index of the tab being drawn, -1 if none
		for (size_t i = 0; i + 1 < c.size(); i++)
		{
			const Segment& from = c[i];
			const Segment& to = c[i + 1];
			int index = from.tab ? from.tab_index : -1;
			if (index != open)
			{
				if (open != -1)
					_tab_indices.push_back(std::make_pair((int)vertices.size(), open));
				open = index;
				if (open != -1)
					vertices.push_back(geometry::v3(from.point));
			}
			if (open == -1 || from.type == SegmentType::Circle)
				continue;
			if (from.type == SegmentType::Line)
				vertices.push_back(geometry::v3(to.point));
			else
			{
				auto v = geometry::arc(from.point, from.center, to.point, from.cw, 1 / _render->camera()->scale());
				for (size_t k = 1; k < v.size(); k++)
					vertices.push_back(geometry::v3(v[k]));
			}
		}
		if (open != -1)
			_tab_indices.push_back(std::make_pair((int)vertices.size(), open));
	}
	if (!vertices.empty())
		_tab_buffer->flush(vertices);
}

void Toolpath::draw_tabs(glm::vec4 color, glm::vec4 selected_color)
{
	int from = 0;
	for (auto& part : _tab_indices)
	{
		_render->set_uniform("inner_color", part.second == _tab_selected && selected() ? selected_color : color);
		_tab_buffer->draw(_render->pr_line_strip(), from, part.first - from);
		from = part.first;
	}
}

void Toolpath::add(Curve value)
{
	_original.push_back(value);
}

void Toolpath::tree(TreeCurve* value)
{
	_tree = value;

	// the shapes which made the tree, to be able to refresh the toolpath when they are modified
	if (_references_id.empty() && value != nullptr)
	{
		for (Curve& c : value->curves())
		{
			int r = c.reference();
			if (r != -1 && std::find(_references_id.begin(), _references_id.end(), r) == _references_id.end())
				_references_id.push_back(r);
		}
	}
}

void Toolpath::refresh(TreeCurve* value)
{
	// one curve: the one of the first reference (a toolpath made from several curves has one toolpath for each)
	std::vector<Curve> curves = value->curves();
	delete value;
	if (curves.empty())
		return;

	Curve chosen = curves[0];
	if (!_references_id.empty())
	{
		for (Curve& c : curves)
			if (c.reference() == _references_id[0])
			{
				chosen = c;
				break;
			}
	}

	_original.clear();
	_original.push_back(chosen);
	compute();
}

void Toolpath::draw()
{
	Graphic::draw();

	int from = 0;
	for (auto to : _data_indices)
	{
		_data_buffer->draw(_render->pr_line_strip(), from, to - from);
		from = to;
	}
	
	from = 0;
	for (auto to : _deco_indices)
	{
		_deco_buffer->draw(_render->pr_line_strip(), from, to - from);
		from = to;
	}
}

bool Toolpath::cw()
{
	return _cw;
}

void Toolpath::cw(bool value)
{
	if (_cw != value)
	{
		_cw = value;
		compute();
	}
}

