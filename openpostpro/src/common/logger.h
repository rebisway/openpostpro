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
* Log strings into a text file using a dedicated thread
*************************************************************************/

#pragma once
#ifndef _LOGGER_H
#define _LOGGER_H

#include <string>

class Logger
{
private:
	static std::string _path;
	static std::string _trace;

private:
	/// <summary>
	/// Thread loop
	/// </summary>
	static void loop();

public:
	/// <summary>
	/// Log file path
	/// </summary>
	static std::string path() { return _path; }

	/// <summary>
	/// Start logging
	/// <param name="path">optional log file path, default is log.txt in the executable folder</param>
	/// </summary>
	static void start(std::string path = "", bool always_flush=false);

	/// <summary>
	/// Stop logging
	/// </summary>
	static void stop();

	/// <summary>
	/// Log data string
	/// </summary>
	static void log(std::string data);

	/// <summary>
	/// Log data string
	/// </summary>
	static void error(std::string data);

	/// <summary>
	/// Log data string
	/// </summary>
	static void success(std::string data);

	/// <summary>
	/// Log data string
	/// </summary>
	static void warning(std::string data);

	/// <summary>
	/// Force file write
	/// </summary>
	static void flush();

	/// <summary>
	/// Clear log file
	/// </summary>
	static void clear();

	/// <summary>
	/// Static trace string
	/// </summary>
	static std::string trace() { return _trace; };

	/// <summary>
	/// Set the trace string
	/// </summary>
	static void trace(std::string data) { _trace = data; };
};

#endif