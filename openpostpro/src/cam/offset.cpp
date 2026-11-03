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

#include <offset.h>
#include <imgui.h>
#include <lang.h>
#include <strings.h>

void Offset::update()
{
	_computed.clear();

	auto sign = _interior ? -1 : 1;

	for (Curve& c : _original)
	{
		c.cw(_cw);
		auto curves = c.offset(radius() * sign);
		_computed.insert(_computed.end(), curves.begin(), curves.end());
	}
	
	_start_point_inside = _interior;

	generate_startpoint(_computed);
	generate_data(_computed);
	generate_deco(_computed);
}

void Offset::scaled()
{
	generate_deco(_computed);
}

std::vector<Curve> Offset::coordinates()
{
	std::vector<Curve> result;

	for (Curve c : _computed)
	{
		Curve n = c;
		result.push_back(n);
	}
	return result;
}

void Offset::ui()
{
	//char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());
	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("INTERIOR"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool v = _interior;
	if (ImGui::Checkbox("##VISIBLE", &v))
	{
		if (_interior != v)
		{
			_start_point_inside = _interior = v;
			compute();
		}
	}


	if (_original.size() == 1 && _original[0].closed())
	{
		auto c = _original[0];
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("TOOLPATH_START_LENGTH"));
		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-FLT_MIN);
		float l = start_point_length();
		if (ImGui::SliderFloat("##TOOLPATH_START_LENGTH", &l, 0, c.length()))
		{
			if (l != start_point_length())
				start_point_length(l);
		}
	}

	ImGui::EndTable();
	ImGui::End();
}

std::string Offset::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(parent()) +
		WS(_name) +
		WN(_radius) +
		WB(_cw) +
		WB(_interior);

	//auto ref = references();
	//result += WS(std::string("[#REF_IDS#]"));
	//result += WN(ref.size());
	//for (int i = 0; i < ref.size(); i++)
	//	result += WN(ref[i]);

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
		for (int i = 0; i < c.size(); i++)
			result += WSEGMENT(c[i]);
	}

	result += WS(std::string("[#STARTPOINT#]"));
	result += WN((int)_start_point_type);
	result += WN(_start_point_length);
	result += WN(_start_point_offset);

	return result;
}

void Offset::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');

	RU(_id, 1);
	RT(2);
	RS(_name, 3);
	RF(_radius, 4);
	RB(_cw, 5);
	RB(_interior, 6);

	//int ref = -1;
	//for (int i = 0; i < data.size() && ref == -1; i++)
	//{
	//	if (data[i] == "[#REF_IDS#]")
	//		ref = i + 1;
	//}
	//if (ref != -1)
	//{
	//	std::vector<int> ids;
	//	int count=0, id=0;
	//	RI(count, ref);
	//	ref++;
	//	for (int i = 0; i < count; i++)
	//	{
	//		RI(id, ref + i);
	//		ids.push_back(id);
	//	}
	//	references(ids);
	//}

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

	update();
}

