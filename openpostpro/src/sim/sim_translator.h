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

// The translators of the simulator. A translator reads a program line by line and gives the simple commands of the simulator
// (see docs/SIMULATOR.fr.md). The ones of the application are Python modules of the `simulator` folder with the methods
// start(options), next(line) and close(); the loader only knows the interface, so a translator can also be written in C++.

#pragma once
#ifndef _SIM_TRANSLATOR_H
#define _SIM_TRANSLATOR_H

#include "sim_types.h"
#include "script.h"
#include <map>
#include <vector>

class SimTranslator
{
public:
	virtual ~SimTranslator() {}

	// Begins a program: the options are numbers (a flag is 0 or 1). False and an error text if the translator can not work.
	virtual bool start(const std::map<std::string, double>& options, std::string& error) = 0;

	// Translates one line of the program (number from 1). The commands are added to `out`.
	virtual void translate(const std::string& text, int number, std::vector<SimCommand>& out) = 0;

	virtual void close() = 0;

	// the unit of the program as the translator says it ("mm" or "inch"), empty if it does not
	virtual std::string unit() const { return ""; }

	// A line of a program as UTF-8: a line which already is valid UTF-8 is not changed, any other one is taken as Windows-1252 or
	// Latin-1 (the programs of many CAM softwares and machines are in these encodings)
	static std::string to_utf8(const std::string& text);
};

// A translator written in Python: the module defines a class named Translator (as a post-processor defines a class named Postpro).
// The python interpreter has to be initialized (Script::initialize_python) and the folder of the module in its path.
class PythonTranslator : public SimTranslator, private Script
{
private:
	std::string _name;
	std::string _unit;
	bool _opened = false;

public:
	// module: the name of the python module, for example "gcode"
	PythonTranslator(const std::string& module);
	~PythonTranslator() override;

	bool start(const std::map<std::string, double>& options, std::string& error) override;
	void translate(const std::string& text, int number, std::vector<SimCommand>& out) override;
	void close() override;
	std::string unit() const override { return _unit; }

	std::string name() const { return _name; }

	// The translators of a folder: the modules (without .py) which define a class Translator with a method next. A module without
	// them is a library which other files use. Sorted by name.
	static std::vector<std::string> list(const std::string& folder);
};

#endif
