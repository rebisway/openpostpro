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

Toolpath::Toolpath(Renderer* r) : Graphic(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f) };
	_data_buffer = r->create_buffer(vertices);
	_deco_buffer = r->create_buffer(vertices);
}

Toolpath::~Toolpath()
{
	if (_data_buffer != nullptr)
	{
		delete _data_buffer;
		_data_buffer = nullptr;
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
			// to move the start point along the curve, we just set le start_point_length
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

			if (segment_pos > 0 && index > 0)
				output.push_back(segments.front());

			output.push_back(segments.back());

			// we have reconstructed a rotated version of _original[0] starting from _start_point_length
			// we just need to add the right segment at the curve begining

			if (_start_point_type != StartPointType::normal)
			{
				auto angle = geometry::oriented_angle(output[1].point, output[0].point);
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

void Toolpath::add(Curve value)
{
	_original.push_back(value);
}

void Toolpath::tree(TreeCurve* value)
{
	_tree = value;
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

