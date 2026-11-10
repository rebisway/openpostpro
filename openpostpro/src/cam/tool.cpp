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

#include "tool.h"
#include <algorithm>
#include <filesystem>
#include "strings.h"
#include <file.h>
#include <environment.h>

Tool* ToolTable::find(int number)
{
	for (Tool& t : _tools)
		if (t.number == number)
			return &t;
	return nullptr;
}

int ToolTable::next_number() const
{
	int n = 1;
	while (std::any_of(_tools.begin(), _tools.end(), [n](const Tool& t) { return t.number == n; }))
		n++;
	return n;
}

Tool& ToolTable::add(Tool tool)
{
	if (tool.number < 0 || find(tool.number) != nullptr)
		tool.number = next_number();
	_tools.push_back(tool);
	std::sort(_tools.begin(), _tools.end(), [](const Tool& a, const Tool& b) { return a.number < b.number; });
	return *find(tool.number);
}

bool ToolTable::remove(int number)
{
	auto it = std::find_if(_tools.begin(), _tools.end(), [number](const Tool& t) { return t.number == number; });
	if (it == _tools.end())
		return false;
	_tools.erase(it);
	return true;
}

bool ToolTable::renumber(int old_number, int new_number)
{
	Tool* t = find(old_number);
	if (t == nullptr || new_number < 0 || (new_number != old_number && find(new_number) != nullptr))
		return false;
	t->number = new_number;
	std::sort(_tools.begin(), _tools.end(), [](const Tool& a, const Tool& b) { return a.number < b.number; });
	return true;
}

// number;name;type;diameter;feed;plunge;pass;spindle;cw;cooling;flutes
std::string ToolTable::write() const
{
	std::string text;
	for (const Tool& t : _tools)
	{
		text += std::to_string(t.number) + ';' + stringex::replace(t.name, ";", "U003B") + ';' + std::to_string((int)t.type) + ';' +
			stringex::to_string(t.diameter) + ';' + stringex::to_string(t.feed) + ';' + stringex::to_string(t.plunge_feed) + ';' +
			stringex::to_string(t.pass) + ';' + stringex::to_string(t.spindle_speed) + ';' + std::to_string((int)t.spindle_cw) + ';' +
			std::to_string((int)t.cooling) + ';' + std::to_string(t.flutes) + '\n';
	}
	return text;
}

void ToolTable::read(const std::string& text)
{
	_tools.clear();
	for (std::string line : stringex::split(const_cast<std::string&>(text), '\n'))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;
		auto d = stringex::split(line, ';');
		try
		{
			Tool t;
			t.number = std::stoi(d[0]);
			if (find(t.number) != nullptr)
				continue;
			// a missing or unknown field keeps its default value
			if (1 < d.size()) t.name = stringex::replace(d[1], "U003B", ";");
			if (2 < d.size()) { int v = std::stoi(d[2]); t.type = v >= 0 && v <= (int)ToolType::Edm ? (ToolType)v : ToolType::Mill; }
			if (3 < d.size()) t.diameter = std::stof(d[3]);
			if (4 < d.size()) t.feed = std::stof(d[4]);
			if (5 < d.size()) t.plunge_feed = std::stof(d[5]);
			if (6 < d.size()) t.pass = std::stof(d[6]);
			if (7 < d.size()) t.spindle_speed = std::stof(d[7]);
			if (8 < d.size()) t.spindle_cw = std::stoi(d[8]) != 0;
			if (10 < d.size()) t.flutes = std::max(1, std::stoi(d[10]));
			if (9 < d.size()) { int v = std::stoi(d[9]); t.cooling = v >= 0 && v <= (int)CoolingType::Mist ? (CoolingType)v : CoolingType::None; }
			_tools.push_back(t);
		}
		catch (const std::exception&)
		{
			// a damaged line is ignored
		}
	}
	std::sort(_tools.begin(), _tools.end(), [](const Tool& a, const Tool& b) { return a.number < b.number; });
}

std::string ToolTable::default_path()
{
	return environment::combine_path(environment::application_data_path(), "tools.txt");
}

bool ToolTable::load(const std::string& path)
{
	std::string p = path.empty() ? default_path() : path;
	if (!std::filesystem::exists(p))
		return false;
	read(file::read_all_text(p));
	return true;
}

bool ToolTable::save(const std::string& path) const
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

ToolTable& tool_table()
{
	static ToolTable table;
	return table;
}
