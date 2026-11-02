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
#include <Python.h>
#include <strings.h>
#include <stdexcept>
#include <filesystem>
#include <config.h>

void Script::initialize_python()
{
	Py_Initialize();
	if (Py_IsInitialized())
	{
		std::string code = "import sys";
		if (!config.python_path.empty())
		{
			auto p = stringex::replace(config.python_path, "\\", "/");
			code += "\nsys.path.append('" + p + "'";
		}

		PyRun_SimpleString(code.c_str());
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
		PyObject* value = PyException_GetArgs(ex);

		if (value != NULL && PyTuple_Check(value))
		{
			for (int i = 0; i < PyTuple_Size(value); i++)
			{
				PyObject* v = PyTuple_GetItem(value, i);
				if (PyUnicode_Check(v))
				{
					if (!error.empty())
						error += ". ";
					error += PyUnicode_AsUTF8(v);
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
	PyRun_SimpleString(py_path.c_str()); // add to python module path the postpro path
}

void Script::run_python(std::string code)
{
	PyRun_SimpleString(code.c_str());
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
				result.push_back(PyUnicode_AsUTF8(v));
			}
		}
	}

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
		answer = PyUnicode_AsUTF8(result);
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
