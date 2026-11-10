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

#include "drill.h"
#include <imgui.h>
#include "strings.h"
#include <logger.h>
#include <lang.h>


Drill::Drill(Renderer* r) : Toolpath(r)
{
	std::vector<glm::vec3> vertices{ glm::vec3(0.0f, 0.0f, 0.0f) };
	_data_buffer = r->create_buffer(vertices);
}

Drill::~Drill()
{
	if (_data_buffer != nullptr)
		delete _data_buffer;
}

std::vector<Curve> Drill::coordinates()
{
	return _original;
}

void Drill::compute()
{
	auto o1 = _DRILL_SIZE / _render->camera()->scale();
	auto o2 = (_DRILL_SIZE / 2) / _render->camera()->scale();
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

void Drill::scaled()
{
	compute();
}

void Drill::draw()
{
	_data_buffer->draw(_render->pr_lines());
}

void Drill::properties()
{
	auto position = _original[0][0].point;
	if (property_point("Point", "##LP1", position))
		modify([&] { _original[0][0].point = position; });

	std::vector<std::string> modes = { Lang::l("CAM_DRILL_DRILLING"), Lang::l("CAM_DRILL_PECKING"), Lang::l("CAM_DRILL_TAPPING"), Lang::l("CAM_DRILL_BORING") };
	int selected = ((int)_mode) - 1;
	if (property_combo(Lang::l("MODE"), "##MODE", selected, modes))
		modify([&] { mode((DrillMode)(selected + 1)); compute(); update(); });

	auto retract = _retract;
	if (property_float(Lang::l("CAM_DRILL_RETRACT"), "##DRILL_RETRACT", retract))
		modify([&] { _retract = retract; });

	if (_mode == DrillMode::Pecking)
	{
		auto delta = _delta;
		if (property_float(Lang::l("CAM_DRILL_DELTA"), "##DRILL_DELTA", delta))
			modify([&] { _delta = delta; });
	}

	if (_mode == DrillMode::Drilling || _mode == DrillMode::Boring)
	{
		int pause = _pause;
		if (property_int(Lang::l("CAM_DRILL_PAUSE"), "##DRILL_PAUSE", pause))
			modify([&] { _pause = pause > 0 ? pause : 0; });
	}
}

std::string Drill::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(parent()) +
		WS(_name) +
		WN((int)_mode) +
		WF(_original[0][0].point.x) + 
		WF(_original[0][0].point.y) +
		WN(_delta) +
		WN(_retract) +
		WN(_pause);

	auto ref = references();
	result += WS(std::string("[#REF_IDS#]"));
	result += WN(ref.size());
	for (int i = 0; i < ref.size(); i++)
		result += WN(ref[i]);

	return result;
}

void Drill::read(std::string value, float version)
{
	_original.clear();
	glm::vec2 p = glm::vec2();
	auto data = stringex::split(value, ';');

	RU(_id, 1);
	RT(2);
	RS(_name, 3);
	if (4 < data.size()) _mode = (DrillMode)std::stoi(data[4]);
	RF(p.x, 5);
	RF(p.y, 6);
	RF(_delta, 7);
	RF(_retract, 8);
	RI(_pause, 9);

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
	update();
}