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

#include "renderer.h"
#include "logger.h"
#include "environment.h"
#include <map>
#include <filesystem>
#include <exception>
#include "file.h"

Renderer::Renderer()
{
	_width = _height = 0;
}

Renderer::~Renderer()
{
}

void Renderer::initialize()
{
	load_programs();
}

void Renderer::load_programs()
{
	// load shaders from local shaders folder
	// vertex shader has '.vert' extension
	// fragment shadeer has '.frag' extension
	std::filesystem::path path(environment::application_path());
	path /= "shaders";

	std::map<std::string, std::filesystem::path> programs;

	if (std::filesystem::exists(path))
	{
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_regular_file()) {
				int type = -1;
				auto extension = entry.path().extension().string();
				auto name = entry.path().filename().string();
				name = name.substr(0, name.size() - extension.size());

				programs[name] = entry;
			}

		}

		for (auto name : programs)
		{
			load_program(name.first, path.string());
		}
	}
}