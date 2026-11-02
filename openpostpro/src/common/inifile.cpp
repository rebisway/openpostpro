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

#include "inifile.h"
#include <fstream>
#include <iostream>


#include "environment.h"
#include "file.h"
#include "strings.h"

IniFile::IniFile(std::string path)
{
	_path = path;

	if (_path == "")
	{
		_path = environment::application_data_path() + environment::application_name_without_extension() + ".ini";
	}

	_lines = file::read_all_lines(_path);
}

IniFile::~IniFile()
{
}

void IniFile::write()
{
	file::write_all_lines(_path, _lines);
}

void IniFile::set(std::string section, std::string name, char const* value)
{
	set(section, name, std::string(value));
}

void IniFile::set(std::string section, std::string name, std::string value)
{
	int s = findSection(section);

	if (s > -1)
	{
		int n = findName(name, s);

		if (n > -1)
		{
			_lines[n] = name + "=" + value;
		}
		else
		{
			// look for next section
			for (int index = s + 1; index < _lines.size(); index++)
			{
				if (stringex::start_with(_lines[index], "["))
				{
					_lines.insert(_lines.begin() + index, name + "=" + value);
					return;
				}
			}
			_lines.push_back(name + "=" + value);
		}
	}
	else
	{
		if (_lines.size() > 0 && _lines.back().size() > 0)
			_lines.push_back("");
		_lines.push_back("[" + section + "]");
		_lines.push_back(name + "=" + value);
	}
}

void IniFile::set(std::string section, std::string name, int value) 
{ 
	set(section, name, std::to_string(value)); 
}

void IniFile::set(std::string section, std::string name, float value) 
{ 
	set(section, name, std::to_string(value)); 
}

void IniFile::set(std::string section, std::string name, double value) 
{ 
	set(section, name, std::to_string(value)); 
}

void IniFile::set(std::string section, std::string name, bool value)
{
	set(section, name, std::string(value ? "true" : "false"));
}


std::string IniFile::get_string(std::string section, std::string name, std::string def)
{
	int s = findSection(section);

	if (s > -1)
	{
		int n = findName(name, s);

		if (n > -1)
		{
			return _lines[n].substr(_lines[n].find_first_of("=") + 1);
		}
	}
	return def;
}

int IniFile::get_int(std::string section, std::string name, int def) 
{ 
	std::string data = get_string(section, name); 
	if (data.size() > 0) 
		return std::stoi(data); 
	else 
		return def; 
}

float IniFile::get_float(std::string section, std::string name, float def) 
{ 
	std::string data = get_string(section, name); 
	if (data.size() > 0) 
		return std::stof(data); 
	else 
		return def; 
}

double IniFile::get_double(std::string section, std::string name, double def)
{
	std::string data = get_string(section, name);
	if (data.size() > 0)
		return std::stod(data);
	else
		return def;
}

bool IniFile::get_bool(std::string section, std::string name, bool def)
{
	std::string data = get_string(section, name);
	if (data.size() > 0)
		return data == "true";
	else
		return def;
}


void IniFile::remove(std::string section, std::string name)
{
	int s = findSection(section);

	if (s > -1)
	{
		int n = findName(name, s);

		if (n > -1)
		{
			_lines.erase(_lines.begin() + n);
		}
	}
}

void IniFile::remove_section(std::string section)
{
	int s = findSection(section);

	if (s > -1)
	{
		int index = s + 1;
		for (int i = index; i < _lines.size(); i++)
		{
			if (_lines[i][0] == '[')
			{
				break;
			}
			index++;
		}

		_lines.erase(_lines.begin() + s, _lines.begin() + index);
	}
}

std::string IniFile::read_data()
{
	return file::read_all_text(_path);
}

std::vector<std::string> IniFile::get_data()
{
	return _lines;
}

void IniFile::load_data(std::string data)
{
	_lines = stringex::split(data, '\n');
}

void IniFile::load_data(std::vector<std::string> data)
{
	_lines.clear();
	_lines.insert(_lines.end(), data.begin(), data.end());
}

int IniFile::findSection(std::string section)
{
	int index = 0;
	for (std::string line : _lines)
	{
		if (stringex::start_with(line, "["))
		{
			if (stringex::start_with(line, "[" + section + "]"))
			{
				return index;
			}
		}
		index++;
	}
	return -1;
}

int IniFile::findName(std::string name, int offset)
{
	for (int index = offset + 1; index < _lines.size(); index++)
	{
		if (stringex::start_with(_lines.at(index), "["))
			break;
		if (stringex::start_with(_lines.at(index), name + "="))
		{
			return index;
		}
	}
	return -1;
}