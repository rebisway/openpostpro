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

#include "group.h"
#include <config.h>
#include <strings.h>
#include <lang.h>
#include <logger.h>

void Group::tool_radius(float value)
{
	if (_tool_radius != value)
	{
		_tool_radius = value;
		for (auto t : _toolpaths)
		{
			t->radius(_tool_radius);
			t->compute();
		}
	}
}

void Group::cw(bool value)
{
	if (value != _cw)
	{
		_cw = value;
		for (auto t : _toolpaths)
		{
			t->cw(!t->cw());
		}
	}
}

void Group::start_point_type(StartPointType value)
{
	if (_start_point_type != value)
	{
		_start_point_type = value;
		for (auto t : _toolpaths)
			t->start_point_type(_start_point_type);

		if (_start_point_offset == 0)
			start_point_offset(_tool_radius * 2);
	}
}

void Group::start_point_offset(float value)
{
	if (_start_point_offset != value)
	{
		_start_point_offset = value;
		for (auto t : _toolpaths)
			t->start_point_offset(_start_point_offset);
	}
}

Group::Group(Renderer* r) : Graphic(r)
{
	_color = config.groupColor;
}

Group::~Group()
{
	for (Toolpath* f : _toolpaths)
	{
		delete f;
	}
	_toolpaths.clear();
}

void Group::add(Toolpath* value)
{
	if (value->name().empty())
	{
		std::string n;
		switch (value->type())
		{
		case GraphicType::CamMoveTo:
			n = Lang::t("CAM_MOVETO");
			break;
		case GraphicType::CamOffset:
			n = Lang::t("CAM_OFFSET");
			break;
		case GraphicType::CamFollow:
			n = Lang::t("CAM_FOLLOW");
			break;
		case GraphicType::CamDrill:
			n = Lang::t("CAM_DRILL");
			break;
		case GraphicType::CamSpiral:
			n = Lang::t("CAM_SPIRAL");
			break;
		case GraphicType::CamPocket:
			n = Lang::t("CAM_POCKET");
			break;
		}

		n += "_" + std::to_string(_toolpaths.size() + 1);
		value->name(n);
	}
	
	// default group params
	value->start_point_offset(_start_point_offset);
	value->start_point_type(_start_point_type);

	_toolpaths.push_back(value);
	value->parent(_name);
}

void Group::remove(Toolpath* value)
{
	if (value != nullptr)
	{
		auto it = std::find(_toolpaths.begin(), _toolpaths.end(), value);
		if (it != _toolpaths.end())
			_toolpaths.erase(it);
	}
}

void Group::remove(std::string name)
{
	Toolpath* toolpath = nullptr;
	for (Toolpath* m : _toolpaths)
	{
		if (m->name() == name)
		{
			toolpath = m;
			break;
		}
	}
	remove(toolpath);
}

void Group::remove(unsigned int id)
{
	Toolpath* toolpath = nullptr;
	for (Toolpath* m : _toolpaths)
	{
		if (m->id() == id)
		{
			toolpath = m;
			break;
		}
	}
	remove(toolpath);
}

Toolpath* Group::toolpath(std::string name)
{
	for (Toolpath* m : _toolpaths)
		if (m->name() == name)
			return m;
	return nullptr;
}

Toolpath* Group::toolpath(unsigned int id)
{
	for (Toolpath* m : _toolpaths)
		if (m->id() == id)
			return m;
	return nullptr;
}

void Group::reset(Renderer* r)
{
	Graphic::reset(r);

	for (Toolpath* f : _toolpaths)
	{
		f->reset(r);
	}
}

void Group::scaled()
{
	for (Toolpath* f : _toolpaths)
	{
		f->scaled();
	}
}

void Group::draw()
{
	// start drawing
	if (_visible)
	{
		_render->set_uniform("inner_color", _color);
		auto cc = _color, lc = _color;
		for (Toolpath* f : _toolpaths)
		{
			if (f->selected())
				cc = config.selectedCamColor;
			else
				cc = _color;
			if (cc != lc)
				_render->set_uniform("inner_color", cc);
			lc = cc;
			f->draw();
		}
	}
}

void Group::ui()
{
	char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	// VISIBILITY
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("VISIBLE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool v = _visible;
	if (ImGui::Checkbox("##GROUP_VISIBLE", &v))
	{
		visible(v);
	}

	// GENERATION
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GENERATE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool g = _generate;
	if (ImGui::Checkbox("##GROUP_GENERATE", &g))
	{
		generate(g);
	}

	// COLOR
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("COLOR"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::ColorEdit4("##COLOR", &_color.r);

	// CW
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_CW"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool cw = _cw;
	if (ImGui::Checkbox("##GROUP_CW", &cw))
	{
		this->cw(cw);
	}

	// TOOL
	ImGui::Spacing();
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
	ImGui::TableNextRow();
	ImGui::Spacing();

	// TOOL NUMBER
	ImGui::TableNextRow();
	if (_tool_number >= 0)
		sprintf_s(input, "%d", _tool_number);
	else
		*input = 0;
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("TOOL"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##TOOL_NUMBER", input, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			std::string result = input;
			if (result.size() == 0)
				_tool_number = _tool_length_compensation = -1;
			else
			{
				int t = std::stoi(result);
				if (_tool_number != t)
				{
					_tool_length_compensation = -1;
					tool_number(t);
				}
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// TOOL RADIUS
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("RADIUS"));
	ImGui::TableSetColumnIndex(1);
	float tr = _tool_radius;
	if (ImGui::InputFloat("##TOOL_RADIUS", &tr, 0.1f, 0.1f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			tool_radius(tr);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// TOOL SPEED
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("TOOL_SPEED"));
	ImGui::TableSetColumnIndex(1);
	int ts = (int)_spindle_speed;
	if (ImGui::InputInt("##TOOL_SPEED", &ts, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			spindle_speed((float)ts);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// TOOL CW
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("TOOL_CW"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	cw = _spindle_cw;
	if (ImGui::Checkbox("##TOOL_CW", &cw))
	{
		spindle_cw(cw);
	}

	// FEED
	ImGui::TableNextRow();
	sprintf_s(input, "%d", (int)_feed);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("TOOL_FEED"));
	ImGui::TableSetColumnIndex(1);
	int fd = (int)_feed;
	if (ImGui::InputInt("##TOOL_FEED", &fd, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			feed((float)fd);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}	

	// PLUNG
	ImGui::TableNextRow();
	sprintf_s(input, "%d", (int)_plung_feed);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("TOOL_PLUNG"));
	ImGui::TableSetColumnIndex(1);
	int pl = (int)_plung_feed;
	if (ImGui::InputInt("##TOOL_PLUNG", &pl, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			plung_feed((float)pl);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	if (_tool_number >= 0)
	{

		// RADIUS COMPENSATION
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("TOOL_RADIUS_COMPENSATION"));
		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-FLT_MIN);
		const char* items[]{ Lang::l("NONE"), Lang::l("TOOL_RADIUS_CMP_LEFT"), Lang::l("TOOL_RADIUS_CMP_RIGHT") };
		int selected_item = (int)_tool_radius_compensation;
		if (ImGui::Combo("##TOOL_RADIUS_COMPENSATION", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			_tool_radius_compensation = (ToolRadiusCompensation)selected_item;
		}

		// LENGTH COMPENSATION
		ImGui::TableNextRow();
		if (_tool_length_compensation >= 0)
			sprintf_s(input, "%d", _tool_length_compensation);
		else
			*input = 0;
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("TOOL_LENGTH_COMPENSATION"));
		ImGui::TableSetColumnIndex(1);
		if (ImGui::InputText("##TOOL_LENGTH_COMPENSATION", input, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			try
			{
				std::string result = input;
				if (result.size() == 0)
					_tool_length_compensation = -1;
				else
					_tool_length_compensation = std::stoi(result);
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}

	// TOOLPATH PARAMS
	ImGui::Spacing();
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
	ImGui::TableNextRow();
	ImGui::Spacing();

	// ORIGIN
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_ORIGIN"));
	ImGui::TableSetColumnIndex(1);
	float o = _origin;
	if (ImGui::InputFloat("##GROUP_ORIGIN", &o, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			origin((float)fd);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}
	
	// DEPTH
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_DEPTH"));
	ImGui::TableSetColumnIndex(1);
	float d = _depth;
	if (ImGui::InputFloat("##GROUP_DEPTH", &d, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			depth(d);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// DEPTH
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_PASS"));
	ImGui::TableSetColumnIndex(1);
	float p = _pass;
	if (ImGui::InputFloat("##GROUP_PASS", &p, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			pass(p);
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// PRE COMMAND
	ImGui::TableNextRow();
	sprintf_s(input, "%s", _pre_command.c_str());
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_PRE_COMMAND"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##GROUP_PRE_COMMAND", input, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			_pre_command = input;
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// POST COMMAND
	ImGui::TableNextRow();
	sprintf_s(input, "%s", _post_command.c_str());
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_POST_COMMAND"));
	ImGui::TableSetColumnIndex(1);
	if (ImGui::InputText("##GROUP_POST_COMMAND", input, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			_post_command = input;
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	// START POINT
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("GROUP_START_POINT_TYPE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	const char* items[]{ Lang::l("NORMAL"), Lang::l("CAD_LINE"), Lang::l("CAD_ARC") };
	int selected_item = (int)_start_point_type;
	if (ImGui::Combo("##GROUP_START_POINT_TYPE", &selected_item, items, IM_ARRAYSIZE(items)))
	{
		start_point_type((StartPointType)selected_item);
	}

	if (_start_point_type != StartPointType::normal)
	{
		// OFFSET
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(Lang::l("GROUP_START_POINT_OFFSET"));
		ImGui::TableSetColumnIndex(1);
		float offset = _start_point_offset;
		if (ImGui::InputFloat("##GROUP_START_POINT_OFFSET", &offset, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
		{
			try
			{
				if (offset != _start_point_offset && offset > 0)
					start_point_offset(offset);
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


std::string Group::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(_name) +
		WC(_color) +
		WN(_visible) +
		WF(_tool_radius) +
		WF(_feed) +
		WF(_plung_feed) +
		WF(_depth) +
		WF(_finition_feed) +
		WN(_tool_number) +
		WF(_finition_offset) +
		WN((int)_tool_radius_compensation) +
		WN(_tool_length_compensation) +
		WS(_pre_command) +
		WS(_post_command) +
		WN((int)_start_point_type) +
		WF(_start_point_offset);

	return result;
}

void Group::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	//if (data.size() == 6)
	{
		// data[0] = type
		RU(_id, 1);
		RS(_name, 2);
		RC(_color, 3);
		RB(_visible, 4);
		RF(_tool_radius, 5);
		RF(_feed, 6);
		RF(_plung_feed, 7);
		RF(_depth, 8);
		RF(_finition_feed, 9);
		RI(_tool_number, 10);
		RF(_finition_offset, 11);
		if (12 < data.size()) _tool_radius_compensation = (ToolRadiusCompensation)std::stoi(data[12]);
		RI(_tool_length_compensation, 13);
		RS(_pre_command, 14);
		RS(_post_command, 15);
		if (16 < data.size()) _start_point_type = (StartPointType)std::stoi(data[16]);
		RF(_start_point_offset, 17);
	}
}
