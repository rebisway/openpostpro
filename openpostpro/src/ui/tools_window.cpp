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

#include "tools_window.h"
#include <imgui.h>
#include <lang.h>
#include <logger.h>
#include <feeds.h>
#include <algorithm>
#include <cstdio>

void ToolsWindow::draw()
{
	if (!_open)
		return;

	ToolTable& table = tool_table();
	bool changed = false;

	ImGui::SetNextWindowSize(ImVec2(620, 380), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin((std::string(Lang::l("TOOLS_WINDOW")) + "###TOOLS_WINDOW").c_str(), &_open))
	{
		ImGui::End();
		return;
	}

	// list
	ImGui::BeginChild("##TOOLS_LIST", ImVec2(220, 0), true);
	for (const Tool& t : table.tools())
	{
		char label[160];
		sprintf_s(label, "T%d  %s##tool%d", t.number, t.name.c_str(), t.number);
		if (ImGui::Selectable(label, t.number == _selected))
			_selected = t.number;
	}
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginGroup();
	if (ImGui::Button(Lang::l("BTN_ADD")))
	{
		Tool t;
		t.number = table.next_number();
		t.name = Lang::t("TOOLS_NEW_NAME");
		_selected = table.add(t).number;
		changed = true;
	}
	ImGui::SameLine();
	Tool* tool = table.find(_selected);
	ImGui::BeginDisabled(tool == nullptr);
	if (ImGui::Button(Lang::l("BTN_REMOVE")) && tool != nullptr)
	{
		// the next tool is selected, or the previous one when it was the last
		size_t position = 0;
		for (size_t i = 0; i < table.tools().size(); i++)
			if (table.tools()[i].number == _selected)
				position = i;
		table.remove(_selected);
		const auto& rest = table.tools();
		_selected = rest.empty() ? -1 : rest[(std::min)(position, rest.size() - 1)].number;
		_edited = -2;
		tool = table.find(_selected);
		changed = true;
	}
	ImGui::EndDisabled();

	// computation of the feeds from the table, at the right of the line (a mill only; the material and the teeth are below)
	{
		const char* label = Lang::l("TOOLS_COMPUTE");
		float width = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2;
		// (SameLine(offset) would add the offset of the group again: the position is set after the line is continued)
		ImGui::SameLine();
		ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width));
		ImGui::BeginDisabled(tool == nullptr || tool->torch());
		if (ImGui::Button(label) && tool != nullptr)
		{
			feeds::Result result = feeds::compute(feeds::materials()[_material], tool->diameter, tool->flutes, _max_rpm);
			feeds::apply(result, *tool);
			_rpm_limited = result.limited;
			changed = true;
		}
		ImGui::EndDisabled();
	}
	ImGui::Separator();

	if (tool != nullptr)
	{
		if (_edited != _selected)
		{
			_new_number = tool->number;
			_edited = _selected;
		}
		ImGui::PushItemWidth(-FLT_MIN);
		const int step_none = 0;

		ImGui::TextUnformatted(Lang::l("TOOL_NUMBER"));
		ImGui::InputInt("##T_NUMBER", &_new_number, step_none, step_none);
		// the number is applied when the field is left (Enter, Tab or a click elsewhere), not only with Enter
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			// refused when another tool has this number
			if (table.renumber(_selected, _new_number))
			{
				_selected = _new_number;
				changed = true;
			}
			_edited = -2;
			tool = table.find(_selected);
		}

		char name[64];
		sprintf_s(name, "%s", tool->name.c_str());
		ImGui::TextUnformatted(Lang::l("TOOLS_NAME"));
		if (ImGui::InputText("##T_NAME", name, sizeof(name)))
		{
			tool->name = name;
			changed = true;
		}

		const char* types[]{ Lang::l("TOOL_TYPE_MILL"), Lang::l("TOOL_TYPE_PLASMA"), Lang::l("TOOL_TYPE_WATERJET"), Lang::l("TOOL_TYPE_EDM") };
		int type = (int)tool->type;
		ImGui::TextUnformatted(Lang::l("TOOL_TYPE"));
		if (ImGui::Combo("##T_TYPE", &type, types, IM_ARRAYSIZE(types)))
		{
			tool->type = (ToolType)type;
			changed = true;
		}

		ImGui::TextUnformatted(tool->torch() ? Lang::l("TOOL_KERF") : Lang::l("TOOL_DIAMETER"));
		changed |= ImGui::InputFloat("##T_DIAMETER", &tool->diameter, 0, 0, "%.3f");
		ImGui::TextUnformatted(Lang::l("TOOL_FEED"));
		changed |= ImGui::InputFloat("##T_FEED", &tool->feed, 0, 0, "%.0f");

		// a torch has no plunge, no pass, no spindle and no cooling
		if (!tool->torch())
		{
			ImGui::TextUnformatted(Lang::l("TOOL_PLUNG"));
			changed |= ImGui::InputFloat("##T_PLUNGE", &tool->plunge_feed, 0, 0, "%.0f");
			ImGui::TextUnformatted(Lang::l("GROUP_PASS"));
			changed |= ImGui::InputFloat("##T_PASS", &tool->pass, 0, 0, "%.3f");
			ImGui::TextUnformatted(Lang::l("TOOL_SPEED"));
			changed |= ImGui::InputFloat("##T_SPINDLE", &tool->spindle_speed, 0, 0, "%.0f");
			changed |= ImGui::Checkbox(Lang::l("TOOL_CW"), &tool->spindle_cw);
			const char* cooling[]{ Lang::l("COOLING_NONE"), Lang::l("COOLING_FLOOD"), Lang::l("COOLING_MIST") };
			int c = (int)tool->cooling;
			ImGui::TextUnformatted(Lang::l("TOOL_COOLING"));
			if (ImGui::Combo("##T_COOLING", &c, cooling, IM_ARRAYSIZE(cooling)))
			{
				tool->cooling = (CoolingType)c;
				changed = true;
			}

			// the table of the feeds: speed, feed, plunge, pass and cooling from the material, the diameter and the number of teeth
			ImGui::Separator();
			ImGui::TextUnformatted(Lang::l("TOOLS_FLUTES"));
			if (ImGui::InputInt("##T_FLUTES", &tool->flutes, 0, 0))
			{
				tool->flutes = (std::max)(1, tool->flutes);
				changed = true;
			}
			const auto& materials = feeds::materials();
			ImGui::TextUnformatted(Lang::l("TOOLS_MATERIAL"));
			std::string preview = Lang::t(std::string("MATERIAL_") + materials[_material].key);
			if (ImGui::BeginCombo("##T_MATERIAL", preview.c_str()))
			{
				for (int i = 0; i < (int)materials.size(); i++)
				{
					std::string name = Lang::t(std::string("MATERIAL_") + materials[i].key);
					if (ImGui::Selectable(name.c_str(), i == _material))
						_material = i;
				}
				ImGui::EndCombo();
			}
			ImGui::TextUnformatted(Lang::l("TOOLS_MAX_RPM"));
			ImGui::InputFloat("##T_MAX_RPM", &_max_rpm, 0, 0, "%.0f");
			if (_rpm_limited)
				ImGui::TextWrapped("%s", Lang::l("TOOLS_RPM_LIMITED"));
			// the hint is long: it goes to the next line when the window is narrow
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::TextWrapped("%s", Lang::l("TOOLS_COMPUTE_HINT"));
			ImGui::PopStyleColor();
		}
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();
	ImGui::End();

	if (changed && !table.save())
		Logger::error("Tools: the table can not be saved");
}
