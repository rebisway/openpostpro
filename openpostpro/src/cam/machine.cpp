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
#include "machine.h"
#include <algorithm>
#include <filesystem>
#include "strings.h"
#include <file.h>
#include <environment.h>

void Machine::area(glm::vec2& low, glm::vec2& high) const
{
	switch (origin)
	{
	case MachineOrigin::BottomLeft:		low = glm::vec2(0, 0);				break;
	case MachineOrigin::BottomRight:	low = glm::vec2(-width, 0);			break;
	case MachineOrigin::TopLeft:		low = glm::vec2(0, -depth);			break;
	case MachineOrigin::TopRight:		low = glm::vec2(-width, -depth);	break;
	case MachineOrigin::Center:			low = glm::vec2(-width / 2, -depth / 2); break;
	}
	high = low + glm::vec2(width, depth);
}

bool Machine::contains(glm::vec2 point, float tolerance) const
{
	glm::vec2 low, high;
	area(low, high);
	return point.x >= low.x - tolerance && point.x <= high.x + tolerance && point.y >= low.y - tolerance && point.y <= high.y + tolerance;
}

// name;width;depth;height;origin;invert x;invert y;invert z
std::string Machine::write() const
{
	return stringex::replace(name, ";", "U003B") + ';' + stringex::to_string(width) + ';' + stringex::to_string(depth) + ';' +
		stringex::to_string(height) + ';' + std::to_string((int)origin) + ';' + std::to_string((int)invert_x) + ';' +
		std::to_string((int)invert_y) + ';' + std::to_string((int)invert_z);
}

Machine Machine::read(const std::string& text)
{
	Machine m;
	auto d = stringex::split(const_cast<std::string&>(text), ';');
	// a missing or damaged field keeps its default value
	auto number = [&](size_t i, float& value) { if (i < d.size()) { try { value = std::stof(d[i]); } catch (const std::exception&) {} } };
	if (0 < d.size()) m.name = stringex::replace(d[0], "U003B", ";");
	number(1, m.width);
	number(2, m.depth);
	number(3, m.height);
	if (4 < d.size())
	{
		try { int v = std::stoi(d[4]); m.origin = v >= 0 && v <= (int)MachineOrigin::Center ? (MachineOrigin)v : MachineOrigin::BottomLeft; }
		catch (const std::exception&) {}
	}
	auto flag = [&](size_t i, bool& value) { if (i < d.size()) { try { value = std::stoi(d[i]) != 0; } catch (const std::exception&) {} } };
	flag(5, m.invert_x);
	flag(6, m.invert_y);
	flag(7, m.invert_z);
	m.width = (std::max)(m.width, 1.0f);
	m.depth = (std::max)(m.depth, 1.0f);
	return m;
}

Machine* MachineTable::find(const std::string& name)
{
	for (Machine& m : _machines)
		if (m.name == name)
			return &m;
	return nullptr;
}

Machine& MachineTable::add(Machine machine)
{
	std::string base = machine.name.empty() ? "Machine" : machine.name;
	std::string name = base;
	for (int n = 2; find(name) != nullptr; n++)
		name = base + " " + std::to_string(n);
	machine.name = name;
	_machines.push_back(machine);
	return _machines.back();
}

bool MachineTable::remove(const std::string& name)
{
	auto it = std::find_if(_machines.begin(), _machines.end(), [&](const Machine& m) { return m.name == name; });
	if (it == _machines.end())
		return false;
	_machines.erase(it);
	return true;
}

bool MachineTable::rename(const std::string& name, const std::string& to)
{
	Machine* m = find(name);
	if (m == nullptr || to.empty() || (to != name && find(to) != nullptr))
		return false;
	m->name = to;
	return true;
}

std::string MachineTable::write() const
{
	std::string text;
	for (const Machine& m : _machines)
		text += m.write() + '\n';
	return text;
}

void MachineTable::read(const std::string& text)
{
	_machines.clear();
	for (std::string line : stringex::split(const_cast<std::string&>(text), '\n'))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;
		Machine m = Machine::read(line);
		if (!m.name.empty() && find(m.name) == nullptr)
			_machines.push_back(m);
	}
}

std::string MachineTable::default_path()
{
	return environment::combine_path(environment::application_data_path(), "machines.txt");
}

bool MachineTable::load(const std::string& path)
{
	std::string p = path.empty() ? default_path() : path;
	if (!std::filesystem::exists(p))
		return false;
	read(file::read_all_text(p));
	return true;
}

bool MachineTable::save(const std::string& path) const
{
	std::string p = path.empty() ? default_path() : path;
	try
	{
		std::filesystem::create_directories(std::filesystem::path(p).parent_path());
		file::write_all_text(p, write());
	}
	catch (const std::exception&)
	{
		return false;
	}
	return true;
}

MachineTable& machine_table()
{
	static MachineTable table;
	return table;
}
