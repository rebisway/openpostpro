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
* methods to manage environment data
* Application path
* User path
*************************************************************************/

#pragma once
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <map>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

namespace environment
{
	std::string current_path();

	std::string application_full_path();

	std::string application_path();

	std::string application_name();

	std::string application_name_without_extension();

	std::string user_path();

	std::string user_documents();

	std::string application_data_path();

	void init();

	std::string combine_path(std::string left, std::string right);


	struct FontFamily
	{
		std::string name;
		std::string regular_path;
		std::string italic_path;
		std::string bold_path;
		std::string bold_italic_path;
	};

	std::map<std::string, FontFamily> fonts();

	static unsigned int _next_id = 1;
	unsigned int next_id();
	unsigned int next_id(unsigned int id);

	std::tm local_time(time_t timer);

	const std::string current_date_time();
};

#endif