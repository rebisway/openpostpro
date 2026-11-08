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

#include "sim_translator.h"
#include <python_api.h>
#include <logger.h>
#include <file.h>
#include <filesystem>
#include <algorithm>

namespace
{
	double number(PyObject* dict, const char* key, double fallback = 0.0)
	{
		PyObject* item = PyDict_GetItemString(dict, key);		// borrowed
		if (item == NULL)
			return fallback;
		if (PyFloat_Check(item))
			return PyFloat_AsDouble(item);
		if (PyLong_Check(item) || PyBool_Check(item))
			return PyLong_AsDouble(item);
		return fallback;
	}

	bool flag(PyObject* dict, const char* key, bool fallback = false)
	{
		PyObject* item = PyDict_GetItemString(dict, key);
		if (item == NULL)
			return fallback;
		return PyObject_IsTrue(item) != 0;
	}

	std::string text(PyObject* dict, const char* key)
	{
		PyObject* item = PyDict_GetItemString(dict, key);
		if (item != NULL && PyUnicode_Check(item))
		{
			const char* s = py_utf8(item);
			return s ? s : "";
		}
		return "";
	}

	SimCommand error_message(const std::string& message, int line)
	{
		SimCommand c;
		c.type = SimCommandType::Message;
		c.level = SimLevel::Error;
		c.text = message;
		c.line = line;
		return c;
	}

	// one command of the translator (a dictionary)
	bool parse(PyObject* dict, int line, SimCommand& c)
	{
		if (dict == NULL || !PyDict_Check(dict))
			return false;

		std::string name = text(dict, "cmd");
		c = SimCommand();
		c.line = line;

		if (name == "rapid" || name == "linear" || name == "arc")
		{
			c.type = name == "rapid" ? SimCommandType::Rapid : (name == "linear" ? SimCommandType::Linear : SimCommandType::Arc);
			c.target = glm::dvec3(number(dict, "x"), number(dict, "y"), number(dict, "z"));
			c.feed = number(dict, "f");
			if (c.type == SimCommandType::Arc)
			{
				c.center = glm::dvec3(number(dict, "cx"), number(dict, "cy"), number(dict, "cz"));
				c.cw = flag(dict, "cw");
				std::string plane = text(dict, "plane");
				c.plane = plane == "xz" ? SimPlane::XZ : (plane == "yz" ? SimPlane::YZ : SimPlane::XY);
			}
		}
		else if (name == "dwell")
		{
			c.type = SimCommandType::Dwell;
			c.seconds = number(dict, "seconds");
		}
		else if (name == "tool")
		{
			c.type = SimCommandType::Tool;
			c.number = (int)number(dict, "number");
		}
		else if (name == "spindle")
		{
			c.type = SimCommandType::Spindle;
			std::string state = text(dict, "state");
			c.state = state == "cw" ? 1 : (state == "ccw" ? 2 : 0);
			c.rpm = number(dict, "rpm");
		}
		else if (name == "coolant")
		{
			c.type = SimCommandType::Coolant;
			std::string state = text(dict, "state");
			c.state = state == "flood" ? 1 : (state == "mist" ? 2 : 0);
		}
		else if (name == "torch")
		{
			c.type = SimCommandType::Torch;
			std::string state = text(dict, "state");
			c.state = state == "on" ? 1 : 0;
		}
		else if (name == "stop")
		{
			c.type = SimCommandType::Stop;
			c.optional = flag(dict, "optional");
			c.end = flag(dict, "end");
		}
		else if (name == "message")
		{
			c.type = SimCommandType::Message;
			std::string level = text(dict, "level");
			c.level = level == "error" ? SimLevel::Error : (level == "warning" ? SimLevel::Warning : SimLevel::Info);
			c.text = text(dict, "text");
		}
		else
		{
			c.type = SimCommandType::Custom;
			c.text = name;
		}
		return true;
	}
}

std::string SimTranslator::to_utf8(const std::string& text)
{
	// is the text valid UTF-8?
	size_t i = 0;
	bool valid = true;
	while (i < text.size() && valid)
	{
		unsigned char c = (unsigned char)text[i];
		int extra = c < 0x80 ? 0 : (c >= 0xC2 && c < 0xE0 ? 1 : (c >= 0xE0 && c < 0xF0 ? 2 : (c >= 0xF0 && c < 0xF5 ? 3 : -1)));
		if (extra < 0 || i + extra >= text.size() + (extra == 0 ? 1 : 0))
			valid = extra == 0;
		for (int k = 1; valid && k <= extra; k++)
			valid = i + k < text.size() && ((unsigned char)text[i + k] & 0xC0) == 0x80;
		i += extra + 1;
	}
	if (valid)
		return text;

	// not UTF-8: the file is in an 8 bits encoding (Windows-1252 or Latin-1 for the letters of the Western languages)
	std::string result;
	for (unsigned char c : text)
	{
		if (c < 0x80)
			result += (char)c;
		else
		{
			result += (char)(0xC0 | (c >> 6));
			result += (char)(0x80 | (c & 0x3F));
		}
	}
	return result;
}

namespace
{
	// The module of a file, under a name of its own and read again each time (a new module replaces the old one). A new reference, NULL
	// with the python error pending if it fails.
	PyObject* module_from_file(const std::string& module_name, const std::string& path)
	{
		PyObject* util = PyImport_ImportModule("importlib.util");
		if (util == NULL)
			return NULL;
		PyObject* spec = PyObject_CallMethod(util, "spec_from_file_location", "ss", module_name.c_str(), path.c_str());
		if (spec == NULL || spec == Py_None)
		{
			Py_XDECREF(spec);
			Py_DECREF(util);
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_ImportError, ("file not found: " + path).c_str());
			return NULL;
		}
		PyObject* module = PyObject_CallMethod(util, "module_from_spec", "O", spec);
		Py_DECREF(util);
		if (module == NULL)
		{
			Py_DECREF(spec);
			return NULL;
		}
		PyDict_SetItemString(PyImport_GetModuleDict(), module_name.c_str(), module);
		PyObject* loader = PyObject_GetAttrString(spec, "loader");
		PyObject* result = loader != NULL ? PyObject_CallMethod(loader, "exec_module", "O", module) : NULL;
		Py_XDECREF(loader);
		Py_DECREF(spec);
		if (result == NULL)
		{
			Py_DECREF(module);
			return NULL;
		}
		Py_DECREF(result);
		return module;
	}
}

PythonTranslator::PythonTranslator(const std::string& module, const std::string& folder)
{
	_name = module;
	_folder = folder;
}

// The class Translator of the module and an object of it (the members of Script hold them). False if there is no such class.
bool PythonTranslator::load_class()
{
	if (_folder.empty())
	{
		// the module is read again, so a translator edited since the last time is the one which is used
		Script::reload_module(_name);
		_module = _name;
		_py_core_module = PyImport_ImportModule(_name.c_str());
	}
	else
	{
		_module = _name;
		_py_core_module = module_from_file("opp_translator_" + _name, (std::filesystem::path(_folder) / (_name + ".py")).string());
	}
	if (_py_core_module != NULL)
	{
		_py_core_class = PyObject_GetAttrString(_py_core_module, "Translator");
		if (_py_core_class != NULL && PyCallable_Check(_py_core_class))
			_py_core_object = PyObject_CallObject(_py_core_class, NULL);
	}
	return _py_core_object != nullptr;
}

PythonTranslator::~PythonTranslator()
{
	close();
}

bool PythonTranslator::start(const std::map<std::string, double>& options, std::string& error)
{
	close();

	// the class Translator of the module
	if (!load_class())
	{
		std::string reason = Script::python_error();
		error = "translator " + _name + " not loaded" + (reason.empty() ? " (no class Translator)" : ": " + reason);
		PyErr_Clear();		// python_error() leaves the exception pending
		Script::finalize();
		return false;
	}

	PyObject* dict = PyDict_New();
	for (const auto& option : options)
	{
		PyObject* value = PyFloat_FromDouble(option.second);
		PyDict_SetItemString(dict, option.first.c_str(), value);
		Py_DECREF(value);
	}

	PyObject* info = PyObject_CallMethod(_py_core_object, "start", "O", dict);
	Py_DECREF(dict);
	if (info == NULL)
	{
		error = "translator " + _name + ", start: " + Script::python_error();
		PyErr_Clear();
		Script::finalize();
		return false;
	}

	_unit = PyDict_Check(info) ? text(info, "unit") : "";
	Py_DECREF(info);
	_opened = true;
	return true;
}

void PythonTranslator::translate(const std::string& line, int number, std::vector<SimCommand>& out)
{
	if (!_opened || _py_core_object == nullptr)
	{
		out.push_back(error_message("translator " + _name + " is not started", number));
		return;
	}

	std::string utf8 = to_utf8(line);
	PyObject* result = PyObject_CallMethod(_py_core_object, "next", "s", utf8.c_str());
	if (result == NULL)
	{
		// an exception of the translator: it is shown with its line of Python, and the program goes on
		std::string message = "translator " + _name + ": " + Script::python_error();
		PyErr_Clear();
		out.push_back(error_message(message, number));
		return;
	}

	if (PyList_Check(result))
	{
		Py_ssize_t count = PyList_Size(result);
		for (Py_ssize_t i = 0; i < count; i++)
		{
			SimCommand c;
			if (parse(PyList_GetItem(result, i), number, c))		// borrowed
				out.push_back(c);
			else
				out.push_back(error_message("translator " + _name + " gave something which is not a command", number));
		}
	}
	else if (result != Py_None)
		out.push_back(error_message("translator " + _name + ": next() has to give a list of commands", number));

	Py_DECREF(result);
}

void PythonTranslator::close()
{
	if (_opened && _py_core_object != nullptr)
	{
		PyObject* result = PyObject_CallMethod(_py_core_object, "close", NULL);
		if (result == NULL)
			PyErr_Clear();
		else
			Py_DECREF(result);
	}
	_opened = false;
	Script::finalize();
}

std::vector<std::string> PythonTranslator::extensions()
{
	std::vector<std::string> result;
	if (load_class() && PyObject_HasAttrString(_py_core_object, "extensions"))
	{
		PyObject* items = PyObject_CallMethod(_py_core_object, "extensions", NULL);
		if (items != NULL && (PyList_Check(items) || PyTuple_Check(items)))
		{
			Py_ssize_t count = PySequence_Size(items);
			for (Py_ssize_t i = 0; i < count; i++)
			{
				PyObject* item = PySequence_GetItem(items, i);		// new reference
				if (item != NULL && PyUnicode_Check(item))
				{
					const char* s = py_utf8(item);
					std::string extension = s ? s : "";
					while (!extension.empty() && extension[0] == '.')
						extension.erase(0, 1);
					std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
					if (!extension.empty())
						result.push_back(extension);
				}
				Py_XDECREF(item);
			}
		}
		if (items != NULL)
			Py_DECREF(items);
	}
	PyErr_Clear();
	Script::finalize();
	return result;
}

std::vector<std::string> PythonTranslator::list(const std::string& folder)
{
	std::vector<std::string> result;
	if (!std::filesystem::exists(folder))
		return result;

	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".py")
			continue;
		std::string source = file::read_all_text(entry.path().string());
		if (source.find("class Translator") != std::string::npos && source.find("def next") != std::string::npos)
			result.push_back(entry.path().stem().string());
	}
	std::sort(result.begin(), result.end());
	return result;
}
