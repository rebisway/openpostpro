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

#include <cad_script.h>

#include <filesystem>

#include <python_api.h>
#include <inifile.h>
#include <strings.h>
#include <cstdarg>
#include <logger.h>
#include <config.h>

void CadScript::set_cadscript()
{
	set("set_script", "O", _py_script_object);

	PyObject* result = PyObject_CallMethod(_py_script_object, "set_core", "O", _py_core_object);

	if (result != NULL)
	{
		Py_DecRef(result);
	}
	else
	{
		auto message = error("set_core");
		throw std::runtime_error(message);
	}
}

CadScript::~CadScript()
{
	finalize();
}

bool CadScript::initialize(std::string module_path, std::string lang)
{
	Script::initialize("cadcore", lang);
	
	if (!module_path.empty())
	{
		if (stringex::end_with(module_path, ".py"))
		{
			_path = module_path;
			auto name = std::filesystem::path(module_path).filename().string();
			_module = name.substr(0, name.size() - 3);
		}
		else
			_module = module_path;

		if (stringex::end_with(module_path, ".py"))
		{
			_path = module_path;
			auto name = std::filesystem::path(module_path).filename().string();
			_module = name.substr(0, name.size() - 3);
		}
		else
			_module = module_path;
	}

	// instanciate class Postpro into _py_post_object
	try
	{
		_py_script_module = PyImport_ImportModule(_module.c_str());

		if (_py_script_module != NULL && !PyErr_Occurred())
		{
			_py_script_class = PyObject_GetAttrString(_py_script_module, "Cadscript");

			if (_py_script_class && PyCallable_Check(_py_script_class) && !PyErr_Occurred())
			{
				_py_script_object = PyObject_CallObject(_py_script_class, NULL);
			}
			else
			{
				_error = "Class \"Cadscript\" is missing [" + Script::python_error() + "]";
				return false;
			}
		}
		else
		{
			_error = "Enable to load script module [" + Script::python_error() + "]";
			return false;
		}

		set_cadscript();

	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}
	// end of instanciation

	set("set_language", "s", _lang.c_str());
	
	try
	{
		load_properties();

		_description = get_string("get_description");
		_category = get_string("get_category");
	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}

	return true;
}


void CadScript::finalize()
{
	save_properties();

	if (_py_script_object != NULL) Py_CLEAR(_py_script_object);
	if (_py_script_class != NULL) Py_CLEAR(_py_script_class);
	if (_py_script_module != NULL) Py_CLEAR(_py_script_module);

	Script::finalize();
}

std::string CadScript::run(glm::vec2 mouse_pos)
{
	initialize();

	std::string result;

	try
	{
		set("set_mouse", "ff", mouse_pos.x, mouse_pos.y);
		result = get_string("run");
	}
	catch (const std::exception& e)
	{
		Logger::error("Running script " + module() + " -> " + std::string(e.what()));
	}

	finalize();

	return result;
}

bool CadScript::begin_dialog(glm::vec2 mouse_pos)
{
	_mouse = mouse_pos;

	if (!initialize())
	{
		Logger::error("Running script " + module() + " -> " + error());
		finalize();
		return false;
	}

	begin_properties();
	return true;
}

MessageBoxResult CadScript::render_GUI(std::string& output)
{
	auto result = draw_properties();

	if (result == MessageBoxResult::Ok)
	{
		// the python object already holds the values chosen in the dialog box
		try
		{
			set("set_mouse", "ff", _mouse.x, _mouse.y);
			output = get_string("run");
		}
		catch (const std::exception& e)
		{
			Logger::error("Running script " + module() + " -> " + std::string(e.what()));
		}
	}

	// finalize saves the properties and frees python memory
	if (result != MessageBoxResult::None)
		finalize();

	return result;
}
