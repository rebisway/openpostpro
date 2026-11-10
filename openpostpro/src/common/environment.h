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
		std::string name;				// the name of the family as the font file says it (Arial, Segoe UI)
		std::string regular_path;
		std::string italic_path;
		std::string bold_path;
		std::string bold_italic_path;
		int regular_index = 0;			// the face of the file (a .ttc file holds several)
		int italic_index = 0;
		int bold_index = 0;
		int bold_italic_index = 0;
	};

	// The families of the fonts of the system (TrueType, OpenType and collections), found with FreeType: the name and the styles come from
	// the font files, not from their names. The list is made once, at the first call (the definition is in font.cpp).
	std::map<std::string, FontFamily> fonts();
	// A family by its name: exact, then without the case, then by the name which the first versions of the application gave (a name made of
	// the file name: "arial", "segoeu"), so the documents which were saved before keep their font. An empty family if there is none.
	FontFamily font_family(const std::string& name);
	// The family used when a font is missing and for a new text: Arial if the system has it, else a similar one.
	std::string default_font_name();

	static unsigned int _next_id = 1;
	unsigned int next_id();
	unsigned int next_id(unsigned int id);

	std::tm local_time(time_t timer);

	const std::string current_date_time();
};

#endif