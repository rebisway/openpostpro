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

/************************************************************************
* Manage string resources for language. Default language files are located
* into a lang sub-folder. 
* A language file contains :
* a pair of KEY=value on a single line
* Use escape string {\n} for new line
*************************************************************************/

#pragma once
#ifndef LANG_H
#define LANG_H

#include <map>
#include <string>
#include <vector>

class Lang {
private:
	/// <summary>
	/// Store dictonnary
	/// </summary>
	static std::map<std::string, std::string> m_lang;

	/// <summary>
	/// Read input language file
	/// </summary>
	/// <param name="name">Language code</param>
	/// <param name="path">Optional : input file path</param>
	/// <returns>std::map<std::string, std::string> containing key/value pairs</returns>
	static std::map<std::string, std::string> get_lang(std::string name, std::string path = "");

	static std::string m_name;

public:
	static std::string getName() { return m_name; };

	/// <summary>
	/// Return array of languages available
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	static std::vector<std::string> get_langs_files(std::string path = "");

	/// <summary>
	/// Return array of languages available
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	static std::vector<std::string> get_langs(std::string path = "");

	/// <summary>
	/// Load selected current language
	/// </summary>
	/// <param name="name">Language code</param>
	/// <param name="path">Optional : input file path</param>
	static void load(std::string name, std::string path = "");;

	/// <summary>
	/// Return associated value of key
	/// </summary>
	/// <param name="code">Key code</param>
	/// <returns>const char* "value"</returns>
	static const char* l(std::string code);;

	/// <summary>
	/// Return associated value of key
	/// </summary>
	/// <param name="code">Key code</param>
	/// <returns>std::string "value"</returns>
	static std::string t(std::string code);;
};
#endif