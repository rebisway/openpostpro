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
#include <string>
#include <vector>

enum class CoolingType
{
	None,
	Flood,
	Mist
};

// What the tool is. A torch (plasma, water jet, wire EDM) cuts through the material: it has no spindle, no pass depth and no plunge.
enum class ToolType
{
	Mill,
	Plasma,
	WaterJet,
	Edm
};

inline bool is_torch(ToolType type) { return type != ToolType::Mill; }

// A tool of the tool table. The number is the Tn of the program, and the key of the table.
struct Tool
{
	int number = 1;
	std::string name;
	ToolType type = ToolType::Mill;
	int flutes = 2;				// number of teeth (for the computation of the feed)
	float diameter = 6;			// cutting width for a torch
	float feed = 1000;			// cutting speed
	float plunge_feed = 500;
	float pass = 1;				// depth of one pass
	float spindle_speed = 0;
	bool spindle_cw = true;
	CoolingType cooling = CoolingType::None;

	bool torch() const { return is_torch(type); }
};

// The tools of the user, in a text file (one tool per line, fields separated by ';', so the file can be read again by any version).
class ToolTable
{
private:
	std::vector<Tool> _tools;

public:
	std::vector<Tool>& tools() { return _tools; }
	const std::vector<Tool>& tools() const { return _tools; }

	// the tool with this number, or nullptr
	Tool* find(int number);
	// adds the tool (the number is changed to the next free one if it is already used), returns the tool in the table
	Tool& add(Tool tool);
	// the lowest number which is not used, from 1
	int next_number() const;
	bool remove(int number);
	// changes the number of a tool, refused (false) if another tool has it
	bool renumber(int old_number, int new_number);

	// text of the file
	std::string write() const;
	void read(const std::string& text);

	// file of the user (in the data folder of the application, replaced by OPP_PROFILE for the tests)
	static std::string default_path();
	bool load(const std::string& path = "");
	bool save(const std::string& path = "") const;
};

// the table of the application
ToolTable& tool_table();
