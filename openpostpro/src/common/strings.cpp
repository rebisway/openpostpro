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

#include "strings.h"
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace stringex
{
	bool start_with(std::string& source, std::string comp)
	{
		if (comp.size() <= source.size())
		{
			for (int i = 0; i < comp.size(); i++)
			{
				if (comp[i] != source[i])
					return false;
			}
			return true;
		}
		return false;
	}

	bool end_with(std::string& source, std::string comp)
	{
		if (comp.size() <= source.size())
		{
			size_t j = comp.size() - 1;
			for (size_t i = source.size() - 1; i >= 0 && (int)j >= 0; i--)
			{
				if (comp[j] != source[i])
					return false;
				j--;
			}
			return true;
		}
		return false;
	}

	std::string replace_string(std::string& str,
		const std::string replace,
		const std::string with) {
		std::size_t pos = str.find(replace);
		if (pos != std::string::npos) {
			str.replace(pos, replace.length(), with);
		}
		return str;
	}

	std::string replace_string_all(std::string& str,
		const std::string replace,
		const std::string with) {
		if (!replace.empty()) {
			std::size_t pos = 0;
			while ((pos = str.find(replace, pos)) != std::string::npos) {
				str.replace(pos, replace.length(), with);
				pos += with.length();
			}
		}
		return str;
	}

	std::string replace(std::string str, const std::string from, const std::string to)
	{
		return replace_string_all(str, from, to);
	}

	std::string concat_strings(std::vector<std::string>& values, std::string separator)
	{
		std::string result = "";
		for (std::string value : values)
		{
			result += value + separator;
		}
		return result.substr(0, result.length() - separator.length());
	}

	std::string left(std::string& source, char c)
	{
		int i = 0;
		for (; i < source.size(); i++)
		{
			if (source[i] == c)
			{
				if (i > 0)
				{
					return source.substr(0, i);
				}
				break;
			}
		}
		return std::string();
	}

	std::string right(std::string& source, char c)
	{
		int i = 0;
		for (; i < source.size(); i++)
		{
			if (source[i] == c)
			{
				if (i < source.size()-1)
				{
					return source.substr(i+1, source.size() - (i + 1));
				}
				break;
			}
		}
		return std::string();
	}

	std::vector<std::string> split(std::string& source, char c)
	{
		std::vector<std::string> ss;
		std::istringstream f(source);
		std::string s;
		while (std::getline(f, s, c)) {
			ss.push_back(s);
		}
		return ss;
	}

	// trim from start (in place)
	std::string ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));

		return s;
	}

	// trim from end (in place)
	std::string rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());

		return s;
	}

	std::string trim(std::string& s) {
		auto l = rtrim(s);
		return ltrim(l);
	}

	std::string to_lower(std::string s)
	{
		std::string result = s;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return result;
	}

	std::string to_upper(std::string s)
	{
		std::string result = s;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::toupper(c); });
		return result;
	}

	std::string from_escape(std::string s)
	{
		if ( s.size() == 0) return s;

		std::string r;

		// (i + 1 < size: s.size() - 1 would be a huge unsigned number for an empty text)
		for (size_t i = 0; i + 1 < s.size(); i++)
		{
			if (s[i] == '\\')
			{
				switch (s[i+1])
				{
				case '\\':
					r += '\\';
					i++;
					break;
				case '\'':
					r += '\'';
					i++;
					break;
				case '?':
					r += '\?';
					i++;
					break;
				case 'a':
					r += '\a';
					i++;
					break;
				case 'b':
					r += '\b';
					i++;
					break;
				case 'n':
					r += '\n';
					i++;
					break;
				case 'r':
					r += '\r';
					i++;
					break;
				case 't':
					r += '\t';
					i++;
					break;
				case 'v':
					r += '\v';
					i++;
					break;
				case 'u':
					if (s.size() - i - 4 > 0)
					{
						auto c = std::stoi(s.substr(i + 1, 4));
						r += (char)c;
						i+=5;
					}
					break;
				default:
					break;
				}
			}
			else
				r += s[i];
		}
		r += s[s.size()-1];

		return r;
	}

	std::string to_escape(std::string s)
	{
		std::string r = s;

		r = replace_string_all(r, "\\", "\\\\");
		r = replace_string_all(r, "\'", "\\\'");
		r = replace_string_all(r, "\"", "\\\"");
		r = replace_string_all(r, "\?", "\\\?");
		r = replace_string_all(r, "\a", "\\\a");
		r = replace_string_all(r, "\b", "\\\b");
		r = replace_string_all(r, "\f", "\\\f");
		r = replace_string_all(r, "\n", "\\\n");
		r = replace_string_all(r, "\r", "\\\r");
		r = replace_string_all(r, "\t", "\\\t");
		r = replace_string_all(r, "\v", "\\\v");

		return r;
	}

	std::string to_string(float value)
	{
		std::ostringstream oss;
		oss << std::setprecision(8) << std::noshowpoint << value;
		return oss.str();
	}

	std::string to_string(double value)
	{
		std::ostringstream oss;
		oss << std::setprecision(16) << std::noshowpoint << value;
		return oss.str();
	}


	std::u32string to_utf32(const std::string& input)
	{
		std::u32string result;
		result.reserve(input.size());
		size_t i = 0;
		while (i < input.size())
		{
			const unsigned char c = (unsigned char)input[i];
			int extra = 0;
			char32_t code = 0;
			if (c < 0x80) { code = c; }
			else if ((c & 0xE0) == 0xC0) { code = c & 0x1F; extra = 1; }
			else if ((c & 0xF0) == 0xE0) { code = c & 0x0F; extra = 2; }
			else if ((c & 0xF8) == 0xF0) { code = c & 0x07; extra = 3; }
			else { result.push_back(0xFFFD); i++; continue; }

			bool valid = i + extra < input.size();
			for (int k = 1; valid && k <= extra; k++)
			{
				const unsigned char next = (unsigned char)input[i + k];
				if ((next & 0xC0) != 0x80)
					valid = false;
				else
					code = (code << 6) | (next & 0x3F);
			}
			// the shortest form only, and no surrogate or code point past U+10FFFF
			if (valid && ((extra == 1 && code < 0x80) || (extra == 2 && code < 0x800) || (extra == 3 && code < 0x10000) || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)))
				valid = false;
			if (!valid)
			{
				result.push_back(0xFFFD);
				i++;
				continue;
			}
			result.push_back(code);
			i += extra + 1;
		}
		return result;
	}

	std::string from_utf32(const std::u32string& input)
	{
		std::string result;
		for (char32_t code : input)
		{
			if (code < 0x80)
				result.push_back((char)code);
			else if (code < 0x800)
			{
				result.push_back((char)(0xC0 | (code >> 6)));
				result.push_back((char)(0x80 | (code & 0x3F)));
			}
			else if (code < 0x10000)
			{
				result.push_back((char)(0xE0 | (code >> 12)));
				result.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
				result.push_back((char)(0x80 | (code & 0x3F)));
			}
			else
			{
				result.push_back((char)(0xF0 | (code >> 18)));
				result.push_back((char)(0x80 | ((code >> 12) & 0x3F)));
				result.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
				result.push_back((char)(0x80 | (code & 0x3F)));
			}
		}
		return result;
	}

	std::string reverse_utf8(const std::string& input)
	{
		std::u32string codes = to_utf32(input);
		std::reverse(codes.begin(), codes.end());
		return from_utf32(codes);
	}

#ifdef _WIN32

	std::string to_string(wchar_t* input)
	{
		int strLength = WideCharToMultiByte(CP_UTF8, 0, input, -1, nullptr, 0, nullptr, nullptr);

		// Create a std::string with the determined length 
		std::string str(strLength, 0);

		// Perform the conversion from LPCWSTR to std::string 
		WideCharToMultiByte(CP_UTF8, 0, input, -1, &str[0], strLength, nullptr, nullptr);

		// ensure there is no '\0' at the end
		std::string result = str.c_str();

		return result;
	}

	std::wstring to_wide_char(std::string input)
	{
		int strLength = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)strlen(input.c_str()), nullptr, 0);

		std::wstring wide;
		wide.resize(strLength + 10);
		strLength = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)strlen(input.c_str()), &wide[0], (int)wide.size());

		return wide;
	}
}
#endif
