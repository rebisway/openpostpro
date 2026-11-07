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

// The loader: gives the lines of a program to a translator and builds the program of the simulator. The work is done by slices of
// time (step), so the interface goes on during a long program: the caller shows the progress and can cancel.

#pragma once
#ifndef _SIM_LOADER_H
#define _SIM_LOADER_H

#include "sim_translator.h"
#include "sim_program.h"

class SimLoader
{
private:
	SimTranslator* _translator = nullptr;
	SimProgram* _program = nullptr;
	std::vector<std::string> _lines;
	size_t _next = 0;
	bool _active = false;
	bool _cancelled = false;
	std::string _error;

public:
	// Begins the translation of a text. False if the translator did not start (see error()).
	bool begin(SimTranslator* translator, SimProgram* program, const std::string& text, const std::map<std::string, double>& options = std::map<std::string, double>());

	// Translates for at most `budget` seconds. True when the translation is over (finished, cancelled or failed).
	bool step(double budget);

	void cancel();
	bool active() const { return _active; }
	bool cancelled() const { return _cancelled; }
	bool failed() const { return !_error.empty(); }
	const std::string& error() const { return _error; }
	size_t lines() const { return _lines.size(); }
	size_t done() const { return _next; }
	double progress() const { return _lines.empty() ? 1.0 : (double)_next / (double)_lines.size(); }

	// the lines of a text, without the line ends (\n or \r\n)
	static std::vector<std::string> split_lines(const std::string& text);
};

#endif
