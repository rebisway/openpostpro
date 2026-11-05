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

#pragma once
#ifndef CAD_SCRIPT_H
#define CAD_SCRIPT_H

#include <string>
#include <vector>
#include <variant>
#include <document.h>
#include <script.h>

class CadScript : public Script
{
private:
	glm::vec2 _mouse = glm::vec2(0, 0);		// mouse position kept while the dialog box is open

	// postpro python class
	PyObject* _py_script_module = nullptr;
	PyObject* _py_script_class = nullptr;
	PyObject* _py_script_object = nullptr;

private:
	void set_cadscript();

public:
	std::string category() { return _category; }
	std::string description() { return _description; }

	~CadScript();
	bool initialize(std::string module_path = "", std::string lang = "");
	void finalize();

	/// <summary>
	/// Run the script with the saved properties
	/// </summary>
	std::string run(glm::vec2 mouse_pos = glm::vec2(0,0));

	/// <summary>
	/// Load the script and its properties before displaying the properties dialog box
	/// </summary>
	/// <returns>False if the script cannot be loaded</returns>
	bool begin_dialog(glm::vec2 mouse_pos);

	/// <summary>
	/// Draw the properties dialog box for the current frame, the script is run when Ok is pressed
	/// </summary>
	/// <param name="output">Script output, filled when the result is Ok</param>
	/// <returns>None while the dialog box is open</returns>
	MessageBoxResult render_GUI(std::string& output);
};

#endif
