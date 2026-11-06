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

// Version of the application, shown in the About window. This is not the version of the document file format.
// To publish a new version: change the numbers below, and increase the build number.
// The build date is the date of the compilation of the file which includes this header (application.cpp).

#pragma once
#ifndef _VERSION_H
#define _VERSION_H

#include <string>

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 5
#define APP_VERSION_PATCH 0
// increased at each version published
#define APP_BUILD 1

namespace appinfo
{
	// "0.5.0"
	inline std::string version()
	{
		return std::to_string(APP_VERSION_MAJOR) + "." + std::to_string(APP_VERSION_MINOR) + "." + std::to_string(APP_VERSION_PATCH);
	}

	inline int build() { return APP_BUILD; }

	// date of the compilation, "2026-11-06" (the preprocessor gives "Nov  6 2026")
	inline std::string build_date()
	{
		const char* compiled = __DATE__;
		const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
		int month = 0;
		for (int i = 0; i < 12; i++)
		{
			if (compiled[0] == months[i * 3] && compiled[1] == months[i * 3 + 1] && compiled[2] == months[i * 3 + 2])
				month = i + 1;
		}
		int day = (compiled[4] == ' ' ? 0 : compiled[4] - '0') * 10 + (compiled[5] - '0');
		std::string year(compiled + 7, 4);
		char text[16];
		snprintf(text, sizeof(text), "%s-%02d-%02d", year.c_str(), month, day);
		return text;
	}

	// time of the compilation, "14:42:59"
	inline std::string build_time() { return __TIME__; }
}

#endif
