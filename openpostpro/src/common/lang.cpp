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

#include "lang.h"
#include "environment.h"
#include <filesystem>
#include <fstream>
#include <strings.h>

#ifdef _WIN32
#define _NEW_LINE "\r\n"
#define _NL_LENGTH 2
#else
#define _NEW_LINE "\n"
#define _NL_LENGTH 1
#endif

std::map<std::string, std::string> Lang::m_lang;
std::string Lang::m_name;

std::vector<std::string> Lang::get_langs_files(std::string path)
{
	std::filesystem::path folder(environment::application_path());
	folder /= "lang";

	if (std::filesystem::exists(path))
	{
		folder = path;
	}

	std::vector<std::string> result;

	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (entry.is_regular_file())
		{
			auto extension = entry.path().extension().string();
			if (extension == ".txt")
			{
				result.push_back(entry.path().string());
			}
		}
	}

	return result;
}

std::vector<std::string> Lang::get_langs(std::string path)
{
	std::filesystem::path folder(environment::application_path());
	folder /= "lang";

	if (std::filesystem::exists(path))
	{
		folder = path;
	}

	std::vector<std::string> result;

	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (entry.is_regular_file())
		{
			auto extension = entry.path().extension().string();
			if (extension == ".txt")
			{
				auto filename = entry.path().filename().string();
				result.push_back(filename.substr(0, filename.length()-4));
			}
		}
	}

	return result;
}

std::map<std::string, std::string> Lang::get_lang(std::string name, std::string path)
{
	std::map<std::string, std::string> result;

	std::string filename = path;

	if (!std::filesystem::exists(path))
	{
		filename = environment::combine_path(environment::combine_path(environment::application_path(),"lang"), name) + ".txt";
	}

	std::ifstream input(filename);
	for (std::string line; std::getline(input, line); ) {
		auto index = line.find_first_of('=');
		// a line without '=' (blank line, text which goes on) is not a key
		if (index != std::string::npos && index > 0)
		{
			auto code = line.substr(0, index);
			auto content = line.substr(index + 1);

			// replace "{\n}" with new line
			std::size_t pos = 0;
			while ((pos = content.find("{\n}", pos)) != std::string::npos) {
				content.replace(pos, 4, _NEW_LINE);
				pos += _NL_LENGTH;
			}

			result[code] = stringex::from_escape(content);
		}
	}

	return result;
}

void Lang::load(std::string name, std::string path)
{
	m_name = name;
	Lang::m_lang = get_lang(name, path);
}

const char* Lang::l(std::string code)
{
	if (Lang::m_lang.find(code) == m_lang.end())
	{
		Lang::m_lang[code] = code;
	}
	return Lang::m_lang[code].c_str();
}

std::string Lang::t(std::string code)
{
	if (Lang::m_lang.find(code) == m_lang.end())
	{
		Lang::m_lang[code] = code;
	}
	return Lang::m_lang[code];
}