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
#ifndef _PYTHON_API_H
#define _PYTHON_API_H

// Include this header, instead of script.h alone, in the source files that call the CPython API.
// script.h only forward declares PyObject to keep Python.h out of the other translation units.

#include <string>
#include <cstdarg>
#include <exception>

#ifdef _DEBUG
#       define _NEEDS_REDEFINED_DEBUG
#		undef _DEBUG
#endif

// Use the stable ABI so one build works with any Python >= 3.12
#ifndef Py_LIMITED_API
#	define Py_LIMITED_API 0x030C0000
#endif

#include <Python.h>

#ifdef _NEEDS_REDEFINED_DEBUG
#	define _DEBUG
#endif

// Stable ABI replacements of PyUnicode_AsUTF8 and PyRun_SimpleString
inline const char* py_utf8(PyObject* object)
{
	return PyUnicode_AsUTF8AndSize(object, NULL);
}

// Run python code in the __main__ module. Returns 0 on success, -1 on error (printed like PyRun_SimpleString)
inline int py_run_string(const char* code)
{
	PyObject* main_module = PyImport_AddModule("__main__"); // borrowed reference
	PyObject* builtins = PyImport_ImportModule("builtins");
	PyObject* result = NULL;

	if (main_module != NULL && builtins != NULL)
		result = PyObject_CallMethod(builtins, "exec", "sO", code, PyModule_GetDict(main_module));

	Py_XDECREF(builtins);

	if (result == NULL)
	{
		PyErr_Print();
		return -1;
	}

	Py_DECREF(result);
	return 0;
}

// This marco forwards a call to PyObject_CallMethod with variadic args
// inputs are : 
//		object_source : PyObject* that as the method to call
//		method_name : method name to call
//		format_string : string describing args, see https://docs.python.org/3/c-api/arg.html#c.PyArg_Parse
//		result_object : NULL PyObject* that will receive PyObject_CallMethod return value. Needs to be Py_DecRef after
#define PY_OBJECT_CALL_METHOD(object_source, method_name, format_string, result_object) {								\
							if (format_string != NULL)																	\
							{																							\
								try {																					\
								PyObject* tuple = nullptr;																\
								va_list args;																			\
								va_start(args, format_string);															\
								tuple = Py_VaBuildValue(format_string, args);											\
								va_end(args);																			\
								result_object = PyObject_CallMethod(object_source, method_name, "O", tuple);			\
								Py_DecRef(tuple);																		\
								} catch(const std::exception& e) {_error = e.what();}									\
							}																							\
							else																						\
								result_object = PyObject_CallMethod(object_source, method_name, NULL);					\
						}																								\

#endif
