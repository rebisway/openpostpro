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

#include "file.h"

#include <iostream>
#include <fstream>
#include <filesystem>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace file
{
	void write_all_text(std::string path, std::string text)
	{
		std::filebuf file;

		file.open(path, std::ios::out);
		std::ostream os(&file);
		os << text;
		file.close();
	}

	void write_all_lines(std::string path, std::vector<std::string> lines)
	{
		if (lines.size() == 0) return;

		std::string buffer("");
		std::string last = lines.back();
		lines.pop_back();

		for (std::string line : lines)
		{
			buffer += line + "\n";
		}

		buffer += last;

		write_all_text(path, buffer);
	}

	std::vector<std::string> read_all_lines(std::string path)
	{
		std::vector<std::string> result;
		if (std::filesystem::exists(path))
		{
			std::string line;
			std::ifstream input(path);
			if (std::getline(input, line))
			{
				// check if utf8
				if (line.size() > 3 && line[0] == '\xef' && line[1] == '\xbb' && line[2] == '\xbf')
					result.push_back(line.substr(3));
				else
					result.push_back(line);
			}
			while (std::getline(input, line))
			{
				result.push_back(line);
			}

			// a file made on Windows read on another system: the text mode does not remove the CR of the line ends there
			for (std::string& l : result)
				if (!l.empty() && l.back() == '\r')
					l.pop_back();
		}

		return result;
	}

	std::string read_all_text(std::string path)
	{
		if (std::filesystem::exists(path))
		{
			auto size = std::filesystem::file_size(path);

			std::ifstream ifs(path);
			std::string content;
			content.reserve(size);
			content.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

			// a file made on Windows read on another system: the text mode does not turn CR LF into LF there
			if (content.find('\r') != std::string::npos)
			{
				std::string clean;
				clean.reserve(content.size());
				for (size_t i = 0; i < content.size(); i++)
					if (!(content[i] == '\r' && i + 1 < content.size() && content[i + 1] == '\n'))
						clean += content[i];
				content.swap(clean);
			}

			if (content.size() > 3 && content[0] == '\xef' && content[1] == '\xbb' && content[2] == '\xbf')
				return content.substr(3);

			return content;
		}
		else
			return "";
	}
}