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

#include "moveTo.h"
#include <imgui.h>
#include <strings.h>
#include <logger.h>
#include <lang.h>


MoveTo::MoveTo(Renderer* r) : Toolpath(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f) };
	_data_buffer = r->create_buffer(vertices);
}

MoveTo::~MoveTo()
{
	if (_data_buffer != nullptr)
		delete _data_buffer;
}

std::vector<Curve> MoveTo::coordinates()
{
    return _original;
}

void MoveTo::compute()
{
	auto o1 = _MOVE_TO_SIZE / _render->camera()->scale();
	auto o2 = (_MOVE_TO_SIZE/2) / _render->camera()->scale();
	auto p = _original[0][0].point;
	std::vector<glm::vec3> vertices{
	glm::vec3(p.x - o1, p.y, 0.0f),
	glm::vec3(p.x + o1, p.y, 0.0f),
	glm::vec3(p.x, p.y - o1, 0.0f),
	glm::vec3(p.x, p.y + o1, 0.0f),

	glm::vec3(p.x - o2, p.y - o2, 0.0f),
	glm::vec3(p.x + o2, p.y - o2, 0.0f),
	glm::vec3(p.x + o2, p.y - o2, 0.0f),
	glm::vec3(p.x + o2, p.y + o2, 0.0f),
	glm::vec3(p.x + o2, p.y + o2, 0.0f),
	glm::vec3(p.x - o2, p.y + o2, 0.0f),
	glm::vec3(p.x - o2, p.y + o2, 0.0f),
	glm::vec3(p.x - o2, p.y - o2, 0.0f)
	};
	_data_buffer->flush(vertices);

}

void MoveTo::scaled()
{
	compute();
}

void MoveTo::draw()
{
	_data_buffer->draw(_render->pr_lines());
}

void MoveTo::ui()
{
	char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	sprintf_s(input, "%0.3f;%0.3f", _original[0][0].point.x, _original[0][0].point.y);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text("Point");
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	if (ImGui::InputText("##LP1", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		std::string val = input;
		auto numbers = stringex::split(val, ';');
		auto p = _original[0][0].point;
		if (numbers.size() == 2)
		{
			try
			{
				p.x = std::stof(numbers[0]);
				p.y = std::stof(numbers[1]);
				if (p != _original[0][0].point)
				{
					History::undo(HistoryActionType::Modify, write());
					_original[0][0].point = p;
				}
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}
	}
	
	ImGui::TableNextRow();
	*input = 0;
	if (_pause > 0)
		sprintf_s(input, "%d", _pause);
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("CAM_DRILL_PAUSE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	if (ImGui::InputText("##DRILL_PAUSE", input, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		try
		{
			std::string result = input;
			if (result.size() == 0)
				_pause = 0;
			else
			{
				int t = std::stoi(result);
				if (t > 0)
					_pause = t;
				else
					_pause = 0;
			}
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}
	}

	ImGui::EndTable();
	ImGui::End();
}

std::string MoveTo::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(parent()) +
		WS(_name) +
		WF(_original[0][0].point.x) +
		WF(_original[0][0].point.y) +
		WN(_pause);

	auto ref = references();
	result += WS(std::string("[#REF_IDS#]"));
	result += WN(ref.size());
	for (int i = 0; i < ref.size(); i++)
		result += WN(ref[i]);

	return result;
}

void MoveTo::read(std::string value, float version)
{
	glm::vec2 p = glm::vec2();
	auto data = stringex::split(value, ';');

	RU(_id, 1);
	RT(2);
	RS(_name, 3);
	RF(p.x, 4);
	RF(p.y, 5);
	RI(_pause, 6);

	Curve c;
	c.add(p);
	_original.push_back(c);

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

	compute();
}