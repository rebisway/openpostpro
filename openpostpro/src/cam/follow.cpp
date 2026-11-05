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

#include <follow.h>
#include <strings.h>
#include <imgui.h>
#include <lang.h>


void Follow::update()
{
	for (auto& c : _original)
		c.cw(_cw);

	_computed.clear();
	_computed.insert(_computed.begin(), _original.begin(), _original.end());
	generate_startpoint(_computed);
	generate_data(_computed);
	generate_deco(_computed);
}

void Follow::scaled()
{
	generate_deco(_computed);
}

std::vector<Curve> Follow::coordinates()
{
	std::vector<Curve> result;

	for (Curve c : _computed)
	{
		Curve n = c;
		result.push_back(n);
	}
	return result;
}

void Follow::properties()
{
	bool clockwise = _cw;
	if (property_bool(Lang::l("CAM_FOLLOW_CW"), "##GROUP_CW", clockwise))
		modify([&] { cw(clockwise); });

	if (_original.size() == 1 && _original[0].closed())
	{
		// no undo step: _start_point_inside is not part of write()
		if (_start_point_type != StartPointType::normal)
		{
			bool inside = _start_point_inside;
			if (property_bool(Lang::l("INTERIOR"), "##VISIBLE", inside))
			{
				_start_point_inside = inside;
				compute();
			}
		}

		// the undo step of a slider is pushed by the helper
		float length = start_point_length();
		if (property_slider(Lang::l("TOOLPATH_START_LENGTH"), "##TOOLPATH_START_LENGTH", length, 0, _original[0].length()))
			start_point_length(length);

		tabs_properties(_original[0]);
	}
}


std::string Follow::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(parent()) +
		WS(_name);

	auto ref = references();
	result += WS(std::string("[#REF_IDS#]"));
	result += WN(ref.size());
	for (int i = 0; i < ref.size(); i++)
		result += WN(ref[i]);

	result += WS(std::string("[#COORDINATES#]"));
	result += WN(_original.size());
	for (auto& c : _original)
	{
		result += WN(c.size());
		for(int i=0; i<c.size(); i++)
			result += WSEGMENT(c[i]);
	}

	result += WS(std::string("[#STARTPOINT#]"));
	result += WN((int)_start_point_type);
	result += WN(_start_point_length);
	result += WN(_start_point_offset);
	result += WB(_cw);
	result += WN(_tabs_count);
	result += WN(_tabs_length);
	result += WN(_tabs_height);

	return result;
}

void Follow::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');

	RU(_id, 1);
	RT(2);
	RS(_name, 3);

	int ref = -1;
	for (int i = 0; i < data.size() && ref == -1; i++)
	{
		if (data[i] == "[#REF_IDS#]")
			ref = i + 1;
	}
	if (ref != -1)
	{
		std::vector<int> ids;
		int count = 0, id = 0;
		RI(count, ref);
		ref++;
		for (int i = 0; i < count; i++)
		{
			RI(id, ref + i);
			ids.push_back(id);
		}
		references(ids);
	}

	ref = -1;
	for (int i = 0; i < data.size() && ref == -1; i++)
	{
		if (data[i] == "[#COORDINATES#]")
			ref = i + 1;
	}
	if (ref != -1)
	{
		int count = 0;
		RI(count, ref);
		ref++;
		for (int i = 0; i < count; i++)
		{
			int size = 0;
			RI(size, ref);
			ref++;
			Curve c;
			for (int j = 0; j < size; j++)
			{
				Segment s;
				RSEGMENT(s, ref);
				c.add(s.type, s.point, s.center, s.radius, s.cw);
				ref++;
			}
			_original.push_back(c);
		}
	}

	ref++;
	int stype = 0;
	RI(stype, ref); _start_point_type = (StartPointType)stype;
	RF(_start_point_length, ref++);
	RF(_start_point_offset, ref);

	// older data have no direction
	if (ref + 1 < data.size())
		RB(_cw, ref + 1);

	// older data have no tabs
	RF(_tabs_count, ref + 2);
	RF(_tabs_length, ref + 3);
	RF(_tabs_height, ref + 4);

	update();
}