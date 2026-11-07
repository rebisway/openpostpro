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

PythonTranslator::PythonTranslator(const std::string& module)
{
	_name = module;
}

PythonTranslator::~PythonTranslator()
{
	close();
}

bool PythonTranslator::start(const std::map<std::string, double>& options, std::string& error)
{
	close();

	// the module is read again, so a translator edited since the last time is the one which is used
	Script::reload_module(_name);
	_module = _name;
	Script::initialize(_name, "");
	if (_py_core_object == nullptr)
	{
		error = _error.empty() ? "translator " + _name + " not loaded" : _error;
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

	PyObject* result = PyObject_CallMethod(_py_core_object, "next", "s", line.c_str());
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
