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

#pragma once
#ifndef _GRAPHIC_H
#define _GRAPHIC_H

#include "graphic.h"
#include <imgui.h>
#include <logger.h>
#include <lang.h>
#include <strings.h>



void Graphic::name(std::string n)
{
	//if (!_name.empty() && _name != n) 
	//	History::undo(HistoryActionType::Modify, write()); 
	_name = n;
}

bool Graphic::shape()
{
	return (int)type() >= 100 && (int)type() < 200;
}

bool Graphic::toolpath()
{
	return (int)type() >= 200 && (int)type() < 300;
}

void Graphic::compute()
{
	_needs_update = true;
}

void Graphic::scaled()
{

}

void Graphic::draw()
{
	if (_needs_update)
	{
		_needs_update = false;
		update();
	}
}



#endif

void Graphic::display_properties()
{
	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	properties();

	ImGui::EndTable();
	ImGui::End();
}

void Graphic::property_label(const char* label)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(label);
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
}

bool Graphic::property_point(const char* label, const char* id, glm::vec2& value)
{
	char input[32];
	bool changed = false;

	sprintf_s(input, "%0.3f;%0.3f", value.x, value.y);
	property_label(label);
	if (ImGui::InputText(id, input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string text = input;
		auto numbers = stringex::split(text, ';');
		if (numbers.size() == 2)
		{
			try
			{
				glm::vec2 point(std::stof(numbers[0]), std::stof(numbers[1]));
				if (point != value)
				{
					value = point;
					changed = true;
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}
	return changed;
}

bool Graphic::property_float(const char* label, const char* id, float& value)
{
	char input[32];
	bool changed = false;

	sprintf_s(input, "%.3f", value);
	property_label(label);
	if (ImGui::InputText(id, input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			auto number = std::stof(input);
			if (number != value)
			{
				value = number;
				changed = true;
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}
	return changed;
}

void Graphic::modify(const std::function<void()>& change)
{
	History::undo(HistoryActionType::Modify, write());
	change();
}

bool Graphic::property_bool(const char* label, const char* id, bool& value)
{
	property_label(label);
	bool checked = value;
	if (ImGui::Checkbox(id, &checked) && checked != value)
	{
		value = checked;
		return true;
	}
	return false;
}

bool Graphic::property_string(const char* label, const char* id, std::string& value)
{
	char input[100];
	bool changed = false;

	snprintf(input, sizeof(input), "%s", value.c_str());
	property_label(label);
	if (ImGui::InputText(id, input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (std::string(input) != value)
		{
			value = input;
			changed = true;
		}
	}
	return changed;
}

bool Graphic::property_combo(const char* label, const char* id, int& selected, const std::vector<std::string>& items)
{
	bool changed = false;

	property_label(label);
	const char* current = (selected >= 0 && selected < (int)items.size()) ? items[selected].c_str() : "";
	if (ImGui::BeginCombo(id, current, 0))
	{
		for (int i = 0; i < (int)items.size(); i++)
		{
			const bool is_selected = (i == selected);
			if (ImGui::Selectable(items[i].c_str(), is_selected) && i != selected)
			{
				selected = i;
				changed = true;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}

void Graphic::property_text(const char* label, const std::string& value)
{
	property_label(label);
	ImGui::Text("%s", value.c_str());
}

bool Graphic::property_int(const char* label, const char* id, int& value)
{
	char input[32];
	bool changed = false;

	sprintf_s(input, "%d", value);
	property_label(label);
	if (ImGui::InputText(id, input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			std::string text = input;
			int number = text.empty() ? 0 : std::stoi(text);
			if (number != value)
			{
				value = number;
				changed = true;
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}
	return changed;
}

bool Graphic::property_slider(const char* label, const char* id, float& value, float min, float max)
{
	property_label(label);
	bool changed = ImGui::SliderFloat(id, &value, min, max);

	// one undo step for a whole drag: the state is saved when the drag starts, before the caller applies any change
	if (ImGui::IsItemActivated())
		_slider_snapshot = write();
	if (changed && !_slider_snapshot.empty())
	{
		History::undo(HistoryActionType::Modify, _slider_snapshot);
		_slider_snapshot.clear();
	}
	return changed;
}
