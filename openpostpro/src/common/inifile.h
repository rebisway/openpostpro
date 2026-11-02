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

/************************************************************************
* Read/write into a standard ini file with [SECTIONS] and key=value lines
* Default ini file is located in 
* 'home\.executable_name_folder\executable_name.ini'
*************************************************************************/


#pragma once
#ifndef INIFILE_H
#define INIFILE_H

#include <string>


#include <vector>


class IniFile
{
private:
	std::string _path;
	std::vector<std::string> _lines;

	int findSection(std::string section);
	int findName(std::string name, int offset);

public:
	IniFile(std::string path="");
	~IniFile();

	std::string path() { return _path; }
	
	void write();

	void set(std::string section, std::string name, char const* value);
	void set(std::string section, std::string name, std::string value);
	void set(std::string section, std::string name, int value);
	void set(std::string section, std::string name, float value);
	void set(std::string section, std::string name, double value);
	void set(std::string section, std::string name, bool value);

	std::string get_string(std::string section, std::string name, std::string def = "");
	int get_int(std::string section, std::string name, int def = 0);
	float get_float(std::string section, std::string name, float def = 0);
	double get_double(std::string section, std::string name, double def = 0);
	bool get_bool(std::string section, std::string name, bool def = false);

	void remove(std::string section, std::string name);
	void remove_section(std::string section);

	/// <summary>
	/// Return the whole file content
	/// </summary>
	/// <returns></returns>
	std::string read_data();

	/// <summary>
	/// Return the whole file content
	/// </summary>
	/// <returns></returns>
	std::vector<std::string> get_data();

	/// <summary>
	/// Use data as input
	/// </summary>
	/// <param name="data"></param>
	void load_data(std::string data);
	
	/// <summary>
	/// Use data as input
	/// </summary>
	/// <param name="data"></param>
	void load_data(std::vector<std::string> data);
};

#endif