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

#include <cad_script.h>

#include <filesystem>

#include <script.h>
#include <inifile.h>
#include <strings.h>
#include <cstdarg>
#include <logger.h>

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
		IniFile ini;
		auto property_count = get_int("get_properties_count");
		_property_names.clear();
		_properties.clear();
		for (int i = 0; i < property_count; i++)
		{
			// load desciption of properties for config dialog box
			_property_names.push_back(get_string("get_property_description", "i", i));

			// load default typed properties and saved config from ini file
			PyObject* p = get("get_property", "i", i);
			if (p != NULL)
			{
				if (PyBool_Check(p))
				{
					_properties.push_back((bool)PyObject_IsTrue(p));
					_properties[i] = ini.get_bool(_module, std::to_string(i), std::get<bool>(_properties[i]));
					set("set_property", "ii", i, (long)std::get<bool>(_properties[i]));
				}
				else if (PyLong_Check(p))
				{
					_properties.push_back((int)PyLong_AsLong(p));
					_properties[i] = ini.get_int(_module, std::to_string(i), std::get<int>(_properties[i]));
					set("set_property", "ii", i, std::get<int>(_properties[i]));
				}
				else if (PyFloat_Check(p))
				{
					_properties.push_back((float)PyFloat_AsDouble(p));
					_properties[i] = ini.get_float(_module, std::to_string(i), std::get<float>(_properties[i]));
					set("set_property", "if", i, std::get<float>(_properties[i]));
				}
				else if (PyUnicode_Check(p))
				{
					_properties.push_back(std::string(PyUnicode_AsUTF8(p)));
					_properties[i] = ini.get_string(_module, std::to_string(i), std::get<std::string>(_properties[i]));
					set("set_property", "is", i, (std::get<std::string>(_properties[i])).c_str());
				}
				else if (PyList_Check(p))
				{
					std::vector<std::string> values;
					for (int i = 0; i < PyList_Size(p); i++)
					{
						if (PyUnicode_Check(PyList_GetItem(p, i)))
							values.push_back(PyUnicode_AsUTF8(PyList_GetItem(p, i)));
					}

					values[0] = ini.get_string(_module, std::to_string(i), values[0]);
					_properties.push_back(values);
					set("set_property", "is", i, (std::get<std::vector<std::string>>(_properties[i])[0]).c_str());
				}
				Py_DECREF(p);
			}
		}

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
	IniFile ini;
	for (int i = 0; i < _properties.size(); i++)
	{
		if (std::holds_alternative<bool>(_properties[i]))
		{
			ini.set(_module, std::to_string(i), std::get<bool>(_properties[i]));
		}
		else if (std::holds_alternative<int>(_properties[i]))
		{
			ini.set(_module, std::to_string(i), std::get<int>(_properties[i]));
		}
		else if (std::holds_alternative<float>(_properties[i]))
		{
			ini.set(_module, std::to_string(i), std::get<float>(_properties[i]));
		}
		else if (std::holds_alternative<std::string>(_properties[i]))
		{
			ini.set(_module, std::to_string(i), std::get<std::string>(_properties[i]));
		}
		else if (std::holds_alternative<std::vector<std::string>>(_properties[i]))
		{
			ini.set(_module, std::to_string(i), std::get<std::vector<std::string>>(_properties[i])[0]);
		}
	}
	ini.write();

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