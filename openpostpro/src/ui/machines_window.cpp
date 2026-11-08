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
#include "machines_window.h"
#include <imgui.h>
#include <document.h>
#include <lang.h>
#include <logger.h>
#include <cstdio>

void MachinesWindow::draw(Document* document)
{
	if (!_open)
		return;

	MachineTable& table = machine_table();
	bool changed = false;

	ImGui::SetNextWindowSize(ImVec2(620, 420), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin((std::string(Lang::l("MACHINES_WINDOW")) + "###MACHINES_WINDOW").c_str(), &_open))
	{
		ImGui::End();
		return;
	}

	// list
	ImGui::BeginChild("##MACHINES_LIST", ImVec2(220, 0), true);
	for (const Machine& m : table.machines())
	{
		bool used = document != nullptr && document->has_machine() && document->machine().name == m.name;
		std::string label = m.name + (used ? "  (" + Lang::t("MACHINE_IN_USE") + ")" : "") + "##machine_" + m.name;
		if (ImGui::Selectable(label.c_str(), m.name == _selected))
			_selected = m.name;
	}
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginGroup();
	if (ImGui::Button(Lang::l("BTN_ADD")))
	{
		Machine m;
		m.name = Lang::t("MACHINES_NEW_NAME");
		_selected = table.add(m).name;
		changed = true;
	}
	ImGui::SameLine();
	Machine* machine = table.find(_selected);
	ImGui::BeginDisabled(machine == nullptr);
	if (ImGui::Button(Lang::l("BTN_REMOVE")) && machine != nullptr)
	{
		// the next machine is selected, or the previous one when it was the last
		size_t position = 0;
		for (size_t i = 0; i < table.machines().size(); i++)
			if (table.machines()[i].name == _selected)
				position = i;
		table.remove(_selected);
		const auto& rest = table.machines();
		_selected = rest.empty() ? "" : rest[(std::min)(position, rest.size() - 1)].name;
		machine = table.find(_selected);
		changed = true;
	}
	ImGui::EndDisabled();

	// the machine of the document: a copy of the one of the table
	{
		const char* use = Lang::l("MACHINE_USE");
		const char* none = Lang::l("MACHINE_NONE");
		float width = ImGui::CalcTextSize(use).x + ImGui::CalcTextSize(none).x + ImGui::GetStyle().FramePadding.x * 4 + ImGui::GetStyle().ItemSpacing.x;
		ImGui::SameLine();
		ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width));
		ImGui::BeginDisabled(machine == nullptr || document == nullptr);
		if (ImGui::Button(use) && machine != nullptr && document != nullptr)
			document->machine(*machine);
		ImGui::EndDisabled();
		// the label is short: the tooltip says what it does
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip("%s", Lang::l("MACHINE_USE_HINT"));
		ImGui::SameLine();
		ImGui::BeginDisabled(document == nullptr || !document->has_machine());
		if (ImGui::Button(none) && document != nullptr)
			document->clear_machine();
		ImGui::EndDisabled();
	}
	ImGui::Separator();

	if (machine != nullptr)
	{
		ImGui::PushItemWidth(-FLT_MIN);

		char name[64];
		sprintf_s(name, "%s", machine->name.c_str());
		ImGui::TextUnformatted(Lang::l("TOOLS_NAME"));
		ImGui::InputText("##M_NAME", name, sizeof(name));
		// the name is applied when the field is left (Enter, Tab or a click elsewhere), not only with Enter
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			// refused when another machine has this name
			std::string old = machine->name;
			if (table.rename(old, name))
			{
				_selected = name;
				changed = true;
			}
			machine = table.find(_selected);
		}

		ImGui::TextUnformatted(Lang::l("MACHINE_WIDTH"));
		changed |= ImGui::InputFloat("##M_WIDTH", &machine->width, 0, 0, "%.1f");
		ImGui::TextUnformatted(Lang::l("MACHINE_DEPTH"));
		changed |= ImGui::InputFloat("##M_DEPTH", &machine->depth, 0, 0, "%.1f");
		ImGui::TextUnformatted(Lang::l("MACHINE_HEIGHT"));
		changed |= ImGui::InputFloat("##M_HEIGHT", &machine->height, 0, 0, "%.1f");
		machine->width = (std::max)(machine->width, 1.0f);
		machine->depth = (std::max)(machine->depth, 1.0f);

		const char* origins[]{ Lang::l("MACHINE_ORIGIN_BL"), Lang::l("MACHINE_ORIGIN_BR"), Lang::l("MACHINE_ORIGIN_TL"), Lang::l("MACHINE_ORIGIN_TR"), Lang::l("MACHINE_ORIGIN_C") };
		int origin = (int)machine->origin;
		ImGui::TextUnformatted(Lang::l("MACHINE_ORIGIN"));
		if (ImGui::Combo("##M_ORIGIN", &origin, origins, IM_ARRAYSIZE(origins)))
		{
			machine->origin = (MachineOrigin)origin;
			changed = true;
		}

		ImGui::PopItemWidth();
		ImGui::Spacing();
		changed |= ImGui::Checkbox(Lang::l("MACHINE_INVERT_X"), &machine->invert_x);
		changed |= ImGui::Checkbox(Lang::l("MACHINE_INVERT_Y"), &machine->invert_y);
		changed |= ImGui::Checkbox(Lang::l("MACHINE_INVERT_Z"), &machine->invert_z);
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::TextWrapped("%s", Lang::l("MACHINE_AXES_HINT"));
		ImGui::PopStyleColor();
	}
	ImGui::EndGroup();
	ImGui::End();

	if (changed && !table.save())
		Logger::error("Machines: the table can not be saved");
}
