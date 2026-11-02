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

#include "environment.h"
#include <stdio.h>  /* defines FILENAME_MAX */
#include <iostream>
#include "strings.h"

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#include <atlstr.h>
#include <Shlobj.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#include <limits.h>
#define GetCurrentDir getcwd
#include <sys/types.h>
#include <pwd.h>
#endif
#include <filesystem>

#include "file.h"
#include <sstream>

namespace environment
{
	std::string current_path()
	{
		char buff[FILENAME_MAX];
		auto size = GetCurrentDir(buff, FILENAME_MAX);
		return std::string(buff);
	}

	std::string application_full_path()
	{
#ifdef _WIN32
		WCHAR result[MAX_PATH];
		GetModuleFileName(NULL, (LPWSTR)result, MAX_PATH);
		return std::string(CW2A(result));
#else
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		return std::string(result, (count > 0) ? count : 0);
#endif
	}

	std::string application_path()
	{
		std::string path = application_full_path();
		return path.substr(0, path.find_last_of(PATH_SEPARATOR) + 1);
	}

	std::string application_name()
	{
		std::string path = application_full_path();
		return path.substr(path.find_last_of(PATH_SEPARATOR) + 1);
	}

	std::string application_name_without_extension()
	{
		std::string path = application_full_path();
		path = path.substr(path.find_last_of(PATH_SEPARATOR) + 1);
		return path.substr(0, path.find_last_of("."));
	}

	std::string user_path()
	{
#ifdef _WIN32
		WCHAR path[MAX_PATH];
		SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path);
		return std::string(CW2A(path)) + PATH_SEPARATOR;
#else
		const char* path;

		if ((path = getenv("HOME")) == NULL) {
			path = getpwuid(getuid())->pw_dir;
		}
		return std::string(path) + PATH_SEPARATOR;
#endif
	}

	std::string user_documents()
	{
		return environment::combine_path(user_path(), "Documents");
	}

	std::string application_data_path()
	{
		return user_path()
			+ "."
			+ application_name_without_extension() + PATH_SEPARATOR;
	}

	void init()
	{
		std::filesystem::create_directory(application_data_path());
	}

	std::string combine_path(std::string left, std::string right)
	{
		auto result = left;

		char sep[] = { PATH_SEPARATOR , 0 };

		if (!stringex::end_with(result, sep))
			result += PATH_SEPARATOR;

		if (!stringex::start_with(right, sep))
			result += right;
		else
			result += right.substr(1, right.size() - 1);

		return result;
	}

	std::map<std::string, FontFamily> fonts(std::string path)
	{
		std::map<std::string, FontFamily> list;
		std::vector<std::string> l;
		FontFamily previous;
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			auto extention = stringex::to_lower(entry.path().extension().string());
			auto path = entry.path().string();
			auto filename = stringex::to_lower(entry.path().filename().string());
			if ( extention == ".ttf")
			{
				auto name = stringex::replace_string(filename, ".ttf", "");

				bool italic = false;
				bool bold = false;

				if (name.find("italic") != std::string::npos)
				{
					italic = true;
					name = stringex::replace_string(name, "italic", "");
				}
				if (name.find("bold") != std::string::npos)
				{
					bold = true;
					name = stringex::replace_string(name, "bold", "");
				}
				if (name.find("bd") != std::string::npos)
				{
					bold = true;
					name = stringex::replace_string(name, "bd", "");
				}
				if (name.find("regular") != std::string::npos)
				{
					name = stringex::replace_string(name, "regular", "");
				}
				if (stringex::end_with(name, "i"))
				{
					italic = true;
					name = name.substr(0, name.size() - 1);
				}
				if (stringex::end_with(name, "b"))
				{
					bold = true;
					name = name.substr(0, name.size() - 1);
				}
				if (stringex::end_with(name, "r"))
				{
					name = name.substr(0, name.size() - 1);
				}
				while (stringex::end_with(name, "_"))
				{
					name = name.substr(0, name.size() - 1);
				}
				while (stringex::end_with(name, "-"))
				{
					name = name.substr(0, name.size() - 1);
				}


				if (name != "")
				{
					FontFamily family = list[name];
					family.name = name;
					if (bold && italic)
						family.bold_italic_path = path;
					else if (bold)
						family.bold_path = path;
					else if (italic)
						family.italic_path = path;
					else
						family.regular_path = path;

					list[name] = family;
				}

				//l.push_back(filename);
			}
		}

		//file::write_all_lines("c:\\dev\\fonts.txt", l);

		return list;
	}
#ifdef _WIN32
	std::map<std::string, FontFamily> fonts()
	{
		return fonts("c:\\windows\\fonts");
	}
#elif defined(linux) || defined(__linux)
	std::map<std::string, FontFamily> fonts()
	{
		std::map<std::string, FontFamily> result = fonts("/usr/share/fonts");
		result.merge(fonts("/usr/local/share/fonts"));
		return result;
	}
#elif defined(__MACH__)
	std::map<std::string, FontFamily> fonts()
	{
		return fonts("/Library/Fonts/");
	}
#endif

	unsigned int next_id()
	{
		return _next_id++;
	}
	unsigned int next_id(unsigned int id)
	{
		return _next_id = id;
	}

	// source from Galik
	// https://stackoverflow.com/questions/38034033/c-localtime-this-function-or-variable-may-be-unsafe
	std::tm local_time(time_t timer)
	{
		std::tm bt{};
#if defined(__unix__)
		localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
		localtime_s(&bt, &timer);
#else
		static std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);
		bt = *std::localtime(&timer);
#endif
		return bt;
	}

	const std::string current_date_time()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto t = local_time(in_time_t);
		std::stringstream result;
		result << std::put_time(&t, "%Y-%m-%d %X");

		return result.str();
	}

}