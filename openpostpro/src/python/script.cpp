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

#include "script.h"
#include <python_api.h>
#include <strings.h>
#include <stdexcept>
#include <filesystem>
#include <config.h>
#include <environment.h>
#include <inifile.h>
#include <lang.h>
#include <imgui.h>
#include <algorithm>

void Script::initialize_python()
{
	Py_Initialize();
	if (Py_IsInitialized())
	{
		std::string code = "import sys";
		if (!config.python_path.empty())
		{
			auto p = stringex::replace(config.python_path, "\\", "/");
			code += "\nsys.path.append('" + p + "')";
		}

		py_run_string(code.c_str());
	}
	else
	{
		throw std::runtime_error("failed to initialize python");
	}
}

void Script::finalize_python()
{
	Py_Finalize();
}

std::string Script::python_error()
{
	std::string error;

	PyObject* ex = PyErr_GetRaisedException();
	if (ex != NULL)
	{
		// the formatted exception gives the file and the line of the error
		PyObject* tb = PyImport_ImportModule("traceback");
		if (tb != NULL)
		{
			PyObject* lines = PyObject_CallMethod(tb, "format_exception", "O", ex);
			if (lines != NULL && PyList_Check(lines))
			{
				for (Py_ssize_t i = 0; i < PyList_Size(lines); i++)
				{
					PyObject* item = PyList_GetItem(lines, i);
					if (PyUnicode_Check(item))
						error += py_utf8(item);
				}
			}
			Py_XDECREF(lines);
			Py_DECREF(tb);
		}
		PyErr_Clear();
		while (!error.empty() && (error.back() == '\n' || error.back() == '\r'))
			error.pop_back();

		PyObject* value = error.empty() ? PyException_GetArgs(ex) : NULL;

		if (value != NULL && PyTuple_Check(value))
		{
			for (int i = 0; i < PyTuple_Size(value); i++)
			{
				PyObject* v = PyTuple_GetItem(value, i);
				if (PyUnicode_Check(v))
				{
					if (!error.empty())
						error += ". ";
					error += py_utf8(v);
				}
			}
		}

		PyErr_SetRaisedException(ex);
	}

	return error;
}

std::string Script::error(std::string method)
{
	return std::string("Method \"" + method + "\" [" + Script::python_error() + "]");
}


void Script::add_module_path(std::string path)
{
	std::string py_path = std::string("sys.path.append('") + stringex::replace_string_all(path, "\\", "/") + "')";
	py_run_string(py_path.c_str()); // add to python module path the postpro path
}

void Script::run_python(std::string code)
{
	py_run_string(code.c_str());
}

std::vector<std::string> Script::sys_path()
{
	std::vector<std::string> result;

	auto p_object = PySys_GetObject("path");
	if (p_object != NULL && PyList_Check(p_object))
	{
		for (int i = 0; i < PyList_Size(p_object); i++)
		{
			PyObject* v = PyList_GetItem(p_object, i);
			if (PyUnicode_Check(v))
			{
				result.push_back(py_utf8(v));
			}
		}
	}

	return result;
}

bool Script::module_exists(std::string name)
{
	bool found = true;

	PyObject* util = PyImport_ImportModule("importlib.util");
	if (util != NULL)
	{
		PyObject* spec = PyObject_CallMethod(util, "find_spec", "s", name.c_str());
		if (spec != NULL)
		{
			found = spec != Py_None;
			Py_DECREF(spec);
		}
		else
			PyErr_Clear();
		Py_DECREF(util);
	}
	else
		PyErr_Clear();

	return found;
}

bool Script::is_delivered(const std::string& path)
{
	if (path.empty())
		return false;

	auto file = std::filesystem::path(path).filename().string();
	for (auto folder : { "postpro", "script" })
	{
		if (std::filesystem::exists(environment::combine_path(environment::combine_path(environment::application_path(), folder), file)))
			return true;
	}
	return false;
}

bool Script::read_only()
{
	return is_delivered(_path);
}

std::string Script::check_syntax(const std::string& source, const std::string& filename, int& line, int& column)
{
	line = column = 0;

	PyObject* code = Py_CompileString(source.c_str(), filename.c_str(), Py_file_input);
	if (code != NULL)
	{
		Py_DECREF(code);
		return "";
	}

	std::string message = "syntax error";
	PyObject* ex = PyErr_GetRaisedException();
	if (ex != NULL)
	{
		PyObject* msg = PyObject_GetAttrString(ex, "msg");
		if (msg != NULL && PyUnicode_Check(msg))
			message = py_utf8(msg);
		else
		{
			PyErr_Clear();
			PyObject* text = PyObject_Str(ex);
			if (text != NULL)
			{
				message = py_utf8(text);
				Py_DECREF(text);
			}
		}
		Py_XDECREF(msg);

		PyObject* value = PyObject_GetAttrString(ex, "lineno");
		if (value != NULL && PyLong_Check(value))
			line = (int)PyLong_AsLong(value);
		PyErr_Clear();
		Py_XDECREF(value);

		value = PyObject_GetAttrString(ex, "offset");
		if (value != NULL && PyLong_Check(value))
			column = (int)PyLong_AsLong(value);
		PyErr_Clear();
		Py_XDECREF(value);

		Py_DECREF(ex);
	}
	PyErr_Clear();

	return message;
}

std::string Script::reload_module(const std::string& name)
{
	std::string error;

	// a module never imported has nothing to reload
	PyObject* module = PyDict_GetItemString(PyImport_GetModuleDict(), name.c_str());
	if (module == NULL)
		return error;

	PyObject* util = PyImport_ImportModule("importlib");
	if (util != NULL)
	{
		PyObject* result = PyObject_CallMethod(util, "reload", "O", module);
		if (result != NULL)
			Py_DECREF(result);
		else
		{
			error = python_error();
			if (error.empty())
				error = "reload failed";
			PyErr_Clear();
		}
		Py_DECREF(util);
	}
	else
		PyErr_Clear();

	return error;
}

void Script::apply_property(int i)
{
	if (std::holds_alternative<bool>(_properties[i]))
		set("set_property", "ii", i, (int)std::get<bool>(_properties[i]));
	else if (std::holds_alternative<int>(_properties[i]))
		set("set_property", "ii", i, std::get<int>(_properties[i]));
	else if (std::holds_alternative<float>(_properties[i]))
		set("set_property", "if", i, std::get<float>(_properties[i]));
	else if (std::holds_alternative<std::string>(_properties[i]))
		set("set_property", "is", i, std::get<std::string>(_properties[i]).c_str());
	else if (std::holds_alternative<std::vector<std::string>>(_properties[i]))
		set("set_property", "is", i, std::get<std::vector<std::string>>(_properties[i])[0].c_str());
}

void Script::load_properties()
{
	IniFile ini;

	_property_count = get_int("get_properties_count");
	_use_properties = get_bool("use_properties");
	_property_names.clear();
	_property_keys.clear();
	_properties.clear();

	for (int i = 0; i < _property_count; i++)
	{
		// description for the dialog box, and key to save the value (the key does not change with the language or the order)
		_property_names.push_back(get_string("get_property_description", "i", i));
		auto key = get_string("get_property_key", "i", i);
		_property_keys.push_back(key);

		// default typed value, replaced by the saved value
		PyObject* p = get("get_property", "i", i);
		if (p != NULL)
		{
			if (PyBool_Check(p))
				_properties.push_back(ini.get_bool(_module, key, (bool)PyObject_IsTrue(p)));
			else if (PyLong_Check(p))
				_properties.push_back(ini.get_int(_module, key, (int)PyLong_AsLong(p)));
			else if (PyFloat_Check(p))
				_properties.push_back(ini.get_float(_module, key, (float)PyFloat_AsDouble(p)));
			else if (PyUnicode_Check(p))
				_properties.push_back(ini.get_string(_module, key, std::string(py_utf8(p))));
			else if (PyList_Check(p))
			{
				// list: first item is the selected index, the others are the choices
				std::vector<std::string> values;
				for (int j = 0; j < PyList_Size(p); j++)
				{
					if (PyUnicode_Check(PyList_GetItem(p, j)))
						values.push_back(py_utf8(PyList_GetItem(p, j)));
				}
				if (values.empty())
					values.push_back("0");
				values[0] = ini.get_string(_module, key, values[0]);
				_properties.push_back(values);
			}
			Py_DECREF(p);
			if (_properties.size() > (size_t)i)
				apply_property(i);
		}
	}
}

void Script::save_properties()
{
	IniFile ini;
	for (size_t i = 0; i < _properties.size() && i < _property_keys.size(); i++)
	{
		if (std::holds_alternative<bool>(_properties[i]))
			ini.set(_module, _property_keys[i], std::get<bool>(_properties[i]));
		else if (std::holds_alternative<int>(_properties[i]))
			ini.set(_module, _property_keys[i], std::get<int>(_properties[i]));
		else if (std::holds_alternative<float>(_properties[i]))
			ini.set(_module, _property_keys[i], std::get<float>(_properties[i]));
		else if (std::holds_alternative<std::string>(_properties[i]))
			ini.set(_module, _property_keys[i], std::get<std::string>(_properties[i]));
		else if (std::holds_alternative<std::vector<std::string>>(_properties[i]))
			ini.set(_module, _property_keys[i], std::get<std::vector<std::string>>(_properties[i])[0]);
	}
	ini.write();
}

void Script::begin_properties()
{
	_properties_config = _properties;
}

MessageBoxResult Script::draw_properties()
{
	MessageBoxResult result = MessageBoxResult::None;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(350.0f, 100.0f + _property_count * 25));
	ImGui::Begin((_description + "###SCRIPT_PROPERTIES").c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	for (int i = 0; i < _property_count && i < (int)_properties_config.size(); i++)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::TextUnformatted(_property_names[i].c_str());
		ImGui::TableSetColumnIndex(1);

		ImGui::SetNextItemWidth(-1);
		auto id = "##PROPERTY" + std::to_string(i);
		if (std::holds_alternative<bool>(_properties_config[i]))
		{
			bool value = std::get<bool>(_properties_config[i]);
			if (ImGui::Checkbox(id.c_str(), &value))
				_properties_config[i] = value;
		}
		else if (std::holds_alternative<int>(_properties_config[i]))
		{
			int value = std::get<int>(_properties_config[i]);
			if (ImGui::InputInt(id.c_str(), &value))
				_properties_config[i] = value;
		}
		else if (std::holds_alternative<float>(_properties_config[i]))
		{
			float value = std::get<float>(_properties_config[i]);
			if (ImGui::InputFloat(id.c_str(), &value))
				_properties_config[i] = value;
		}
		else if (std::holds_alternative<std::string>(_properties_config[i]))
		{
			std::string value = std::get<std::string>(_properties_config[i]);
			char t[200];
			sprintf_s(t, "%s", value.c_str());
			if (ImGui::InputText(id.c_str(), t, 200))
				_properties_config[i] = std::string(t);
		}
		else if (std::holds_alternative<std::vector<std::string>>(_properties_config[i]))
		{
			std::vector<std::string> values = std::get<std::vector<std::string>>(_properties_config[i]);
			std::string choices;
			for (size_t j = 1; j < values.size(); j++)
			{
				choices += values[j];
				choices += '\0';
			}
			int current = std::atoi(values[0].c_str());
			if (ImGui::Combo(id.c_str(), &current, choices.c_str()))
			{
				values[0] = std::to_string(current);
				_properties_config[i] = values;
			}
		}
	}

	ImGui::EndTable();

	ImGui::NewLine();
	ImGuiStyle& style = ImGui::GetStyle();
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = std::max(ImGui::CalcTextSize(Lang::l("OK")).x, ImGui::CalcTextSize(Lang::l("CANCEL")).x);
	ImGui::SetCursorPosX((windowWidth - (2 * textWidth + style.ItemSpacing.x)) * 0.5f);
	if (ImGui::Button(Lang::l("OK")))
	{
		// the edited values are kept and sent to the script
		_properties = _properties_config;
		for (int i = 0; i < (int)_properties.size(); i++)
			apply_property(i);
		result = MessageBoxResult::Ok;
	}
	ImGui::SameLine();
	if (ImGui::Button(Lang::l("CANCEL")))
		result = MessageBoxResult::Cancel;
	ImGui::End();

	return result;
}

bool Script::initialize(std::string core_path, std::string lang)
{
	if (stringex::end_with(core_path, ".py"))
	{
		_path = core_path;
		auto name = std::filesystem::path(core_path).filename().string();
		_core = name.substr(0, name.size() - 3);
	}
	else
		_core = core_path;

	if (!lang.empty())
		_lang = lang;

	// instanciate class loop into _py_core_object
	try
	{
		_py_core_module = PyImport_ImportModule(_core.c_str());

		if (_py_core_module != NULL && !PyErr_Occurred())
		{
			auto class_name = _core;
			class_name[0] = std::toupper(_core[0]);

			_py_core_class = PyObject_GetAttrString(_py_core_module, class_name.c_str());

			if (_py_core_class && PyCallable_Check(_py_core_class) && !PyErr_Occurred())
			{
				_py_core_object = PyObject_CallObject(_py_core_class, NULL);
			}
			else
			{
				_error = "Class \"" + class_name + "\" is missing [" + Script::python_error() + "]";
				return false;
			}
		}
		else
		{
			_error = "Enable to load " + _module + " module [" + Script::python_error() + "]";
			return false;
		}
	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}

	return false;
}

void Script::finalize()
{
	if (_py_core_object != NULL) Py_CLEAR(_py_core_object);
	if (_py_core_class != NULL) Py_CLEAR(_py_core_class);
	if (_py_core_module != NULL) Py_CLEAR(_py_core_module);
}

std::string Script::get_string(std::string method, const char* format, ...)
{
	PyObject* result = nullptr;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);
	std::string answer = "";
	if (result && PyUnicode_Check(result))
	{
		answer = py_utf8(result);
		Py_CLEAR(result);
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
	return answer;
}

int Script::get_int(std::string method, const char* format, ...)
{
	PyObject* result = nullptr;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);
	int answer = 0;
	if (result && PyLong_Check(result))
	{
		answer = PyLong_AsLong(result);
		Py_CLEAR(result);
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
	return answer;
}

float Script::get_float(std::string method, const char* format, ...)
{
	PyObject* result = nullptr;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);
	float answer = 0;
	if (result && PyFloat_Check(result))
	{
		answer = (float)PyFloat_AsDouble(result);
		Py_CLEAR(result);
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
	return answer;
}

bool Script::get_bool(std::string method, const char* format, ...)
{
	PyObject* result = nullptr;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);
	bool answer = false;
	if (result && (PyBool_Check(result) || PyLong_Check(result)))
	{
		answer = PyObject_IsTrue(result);
		Py_CLEAR(result);
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
	return answer;
}

void Script::set(std::string method, const char* format, ...)
{
	PyObject* result = NULL;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);

	if (result)
	{
		Py_CLEAR(result);
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
}

PyObject* Script::get(std::string method, const char* format, ...)
{
	PyObject* result = NULL;
	PY_OBJECT_CALL_METHOD(_py_core_object, method.c_str(), format, result);

	if (result)
	{
		return result;
	}
	else
	{
		auto message = std::string("Method " + method + "[") + Script::python_error() + "]";
		throw std::runtime_error(message);
	}
}
