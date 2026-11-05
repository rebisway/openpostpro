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
#ifndef POSTPRO_H
#define POSTPRO_H

#include <string>
#include <vector>
#include <variant>
#include <document.h>
#include <script.h>


// The Postpro python object uses a postcore.py intermediate file
// to allow callback from user postpro python object

class Postpro : public Script
{
private:
	//std::string _name;
	//std::string _description;
	//std::string _category;
	//std::string _module;
	//std::string _path;
	//std::string _lang;
	//std::string _error;
	std::string _extention;
	bool _ijk_relative = true;

	glm::vec3 _pos = glm::vec3(0,0,0);

	bool _first_call = true;

	// postpro python class
	PyObject* _py_post_module = nullptr;
	PyObject* _py_post_class = nullptr;
	PyObject* _py_post_object = nullptr;

	// postloop python file
	//PyObject* _py_core_module = nullptr;
	//PyObject* _py_core_class = nullptr;
	//PyObject* _py_core_object = nullptr;

private:
	//std::string error(std::string method);

	// postloop calling
	int get_line_count();
	glm::vec3 get_current_position();
	void set_postpro();
	void set_safe_position(float value);

	bool get_disable_z();

	std::string start_loop();
	std::string stop_loop();
	std::string start_program();
	std::string stop_program();
	std::string start_group(std::string name, int tool_number, int tool_speed);
	std::string stop_group();
	std::string start_tool_radius_compensation(ToolRadiusCompensation cmp);
	std::string stop_tool_radius_compensation(ToolRadiusCompensation cmp);
	std::string start_tool_length_compensation(int number);
	std::string stop_tool_length_compensation(int number);
	std::string start_toolpath();
	std::string stop_toolpath();
	std::string start_single_path();
	std::string stop_single_path();

	// postpro calling
	
	//std::string get_string(std::string method, const char* format = NULL, ...);
	//int get_int(std::string method, const char* format = NULL, ...);
	//float get_float(std::string method, const char* format = NULL, ...);
	//bool get_bool(std::string method, const char* format = NULL, ...);

	//void set(std::string method, const char* format = NULL, ...);
	//PyObject* get(std::string method, const char* format = NULL, ...);

public:
	~Postpro();

	/// <summary>
	/// Initialize the Postpro python object
	/// </summary>
	/// <param name="module"></param>
	/// <param name="lang"></param>
	/// <returns>True if no errors</returns>
	bool initialize(std::string module_path, std::string lang) override;

	/// <summary>
	/// Release memory
	/// </summary>
	void finalize() override;

	/// <summary>
	/// Render the config dialog box
	/// </summary>
	/// <returns></returns>
	bool render_GUI();

	std::string extention();
	bool ijk_relative();

	std::string comment(std::string value);
	std::string rapid(char axe, float value);
	std::string rapid(glm::vec3 value);
	std::string linear(glm::vec3 value, bool rapid = false);
	std::string circular(glm::vec3 value, glm::vec2 center, bool cw);
	std::string feed(float value);
	std::string drilling(glm::vec3 value, float retract, int pause);
	std::string pecking(glm::vec3 value, float retract, float delta);
	std::string tapping(glm::vec3 value, float retract);
	std::string boring(glm::vec3 value, float retract, int pause);
	std::string pause(int value);
	std::string line(std::string value);

	std::string run(Document* doc, std::string path, std::string version);
};


#endif