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
* Extensions for std::string manipulation
*************************************************************************/


#pragma once
#ifndef STRINGS_H
#define STRINGS_H

#include <string>
#include <vector>

namespace stringex
{
//public:
	/// <summary>
	/// Check if a string starts with a substring
	/// </summary>
	/// <param name="source">string to test</param>
	/// <param name="comp">pattern to find</param>
	/// <returns></returns>
	bool start_with(std::string& source, std::string comp);

	/// <summary>
	/// Check if a string ends with a substring
	/// </summary>
	/// <param name="source">string to test</param>
	/// <param name="comp">pattern to find</param>
	/// <returns></returns>
	bool end_with(std::string& source, std::string comp);
	
	/// <summary>
	/// Replace the first substring occurence with another string inside a source string
	/// </summary>
	/// <param name="str">source</param>
	/// <param name="replace">string to look for</param>
	/// <param name="with">string to paste</param>
	/// <returns></returns>
	std::string replace_string(std::string& str, const std::string replace, const std::string with);

	/// <summary>
	/// Replace every substring occurence with another string inside a source string
	/// </summary>
	/// <param name="str">source</param>
	/// <param name="replace">string to look for</param>
	/// <param name="with">string to paste</param>
	/// <returns></returns>
	/// <returns></returns>
	std::string replace_string_all(std::string& str, const std::string replace, const std::string with);
	std::string replace(std::string str, const std::string from, const std::string to);

	/// <summary>
	/// Concat an arry of strings into one string
	/// </summary>
	/// <param name="values">array of strings</param>
	/// <param name="separator">optional : string separator</param>
	/// <returns></returns>
	std::string concat_strings(std::vector<std::string>& values, std::string separator = "");

	/// <summary>
	/// Return left substring before character
	/// </summary>
	/// <param name="source">string to split</param>
	/// <param name="c">character</param>
	/// <returns></returns>
	std::string left(std::string& source, char c);

	/// <summary>
	/// Return left substring after character
	/// </summary>
	/// <param name="source">string to split</param>
	/// <param name="c">character</param>
	/// <returns></returns>
	std::string right(std::string& source, char c);

	/// <summary>
	/// Split a string before and after a character into several strings
	/// </summary>
	/// <param name="source">string to split</param>
	/// <param name="c">character</param>
	/// <returns></returns>
	std::vector<std::string> split(std::string& source, char c);

	/// <summary>
	/// Left space charater trim
	/// </summary>
	/// <param name="s">string to trim</param>
	/// <returns>trimmed string</returns>
	std::string ltrim(std::string& s);

	/// <summary>
	/// Right space charater trim
	/// </summary>
	/// <param name="s">string to trim</param>
	/// <returns>trimmed string</returns>
	std::string rtrim(std::string& s);

	/// <summary>
	/// Left & right space charater trim
	/// </summary>
	/// <param name="s">string to trim</param>
	/// <returns>trimmed string</returns>
	std::string trim(std::string& s);

	std::string to_lower(std::string s);

	/// <summary>
	/// Decode an UTF-8 string into code points (not UTF-16: a character outside of the first plane is one code point). A byte which is
	/// not valid UTF-8 gives U+FFFD. The same on every platform.
	/// </summary>
	std::u32string to_utf32(const std::string& input);

	/// <summary>
	/// Encode code points in UTF-8
	/// </summary>
	std::string from_utf32(const std::u32string& input);

	/// <summary>
	/// The characters of an UTF-8 string in the reverse order (not its bytes)
	/// </summary>
	std::string reverse_utf8(const std::string& input);

	std::string to_upper(std::string s);

	std::string from_escape(std::string s);

	std::string to_escape(std::string s);

	std::string to_string(float value);

	std::string to_string(double value);


#ifdef _WIN32
	/// <summary>
	/// Convert a wide char U16 into std::string
	/// </summary>
	/// <param name="input">wide char string</param>
	/// <returns>converted string</returns>
	std::string to_string(wchar_t* input);

	/// <summary>
	/// Convert an std::string to wide char string
	/// </summary>
	/// <param name="input">string to convert</param>
	/// <returns>converted string</returns>
	std::wstring to_wide_char(std::string input);
#endif
};

#endif