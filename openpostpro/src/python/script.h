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

#ifndef _SCRIPT_H
#define _SCRIPT_H
#pragma once
#include <string>
#include <vector>

#ifdef _DEBUG
#       define _NEEDS_REDEFINED_DEBUG
#		undef _DEBUG
#endif

#include <python.h>

#ifdef _NEEDS_REDEFINED_DEBUG
#	define _DEBUG
#endif

//////////////////////////////////////////////////////////////////////////////////
// This is the main class to run python scripts within the main application		//
// First, you need to initialize by calling static initialize_python() method	//
// Then you have to add python directory where are located your files			//
// using the add_module_path() method											//
// You can check for any python error calling the static error_python() method	//
//																				//
// Further in you application, you can instanciate a Script class				//
// Calling initialize(std::string core_path, std::string lang) method will		//
// instanciate the object with the same class name as the module name			//
// get and set methods will operate on this last object							//
//																				//
//////////////////////////////////////////////////////////////////////////////////



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

class Script
{
protected:
	std::string _description;
	std::string _category;
	std::string _module;
	std::string _path;
	std::string _core;
	std::string _lang;
	std::string _error;

	PyObject* _py_core_module = nullptr;
	PyObject* _py_core_class = nullptr;
	PyObject* _py_core_object = nullptr;

protected:
	/// <summary>
	/// Return an error text including method name from _py_core_object object and CPython error desc
	/// </summary>
	/// <returns></returns>
	std::string error(std::string method);

public:
	////////////////////
	// Static methods //
	////////////////////

	/// <summary>
	/// Static method. Call first to setup CPython API
	/// </summary>
	static void initialize_python();

	/// <summary>
	/// Static method. Call to cleanup CPython API
	/// </summary>
	static void finalize_python();

	/// <summary>
	/// Static method. Call to retreive last CPython error string
	/// </summary>
	/// <returns>Python error</returns>
	static std::string python_error();

	/// <summary>
	/// Static method. Add python module search path. You need to add every paths you need before running scripts
	/// </summary>
	/// <param name="path"></param>
	static void add_module_path(std::string path);

	/// <summary>
	/// Static method. Run a python script
	/// </summary>
	/// <param name="code">Python code to run</param>
	static void run_python(std::string code);

	/// <summary>
	/// Static method. Return sys.path variable
	/// </summary>
	static std::vector<std::string> sys_path();

	/////////////////////////////////////
	// Default core_path object access //
	/////////////////////////////////////

	/// <summary>
	/// Initialize a _py_core_object from a class that as the same name as the module name
	/// </summary>
	/// <param name="core_path">Module path</param>
	/// <param name="lang">Language code "eg: en-uk, fr-fr</param>
	/// <returns>True if success</returns>
	virtual bool initialize(std::string core_path, std::string lang);

	/// <summary>
	/// Cleanup python objects instancied in initialize method
	/// </summary>
	virtual void finalize();

	/// <summary>
	/// Return the module math
	/// </summary>
	/// <returns></returns>
	std::string path() { return _path; }

	/// <summary>
	/// Return the module name
	/// </summary>
	/// <returns></returns>
	std::string module() { return _module; }

	/// <summary>
	/// Return the description returned by def description(self) method in _py_core_object object
	/// </summary>
	/// <returns></returns>
	std::string description() { return _description; }

	/// <summary>
	/// Return the category returned by def description(self) method in _py_core_object object
	/// </summary>
	/// <returns></returns>
	std::string category() { return _category; }

	/// <summary>
	/// Return the description returned by def description(self) method in _py_core_object object
	/// </summary>
	/// <returns></returns>
	std::string error() { return _error; }

	/// <summary>
	/// Call a method from _py_core_object object and return the expected string result
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	std::string get_string(std::string method, const char* format = NULL, ...);

	/// <summary>
	/// Call a method from _py_core_object object and return the expected int result
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	int get_int(std::string method, const char* format = NULL, ...);

	/// <summary>
	/// Call a method from _py_core_object object and return the expected float result
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	float get_float(std::string method, const char* format = NULL, ...);

	/// <summary>
	/// Call a method from _py_core_object object and return the expected bool result
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	bool get_bool(std::string method, const char* format = NULL, ...);


	/// <summary>
	/// Call a method from _py_core_object object, result is discard if there is
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	void set(std::string method, const char* format = NULL, ...);

	/// <summary>
	/// Call a method from _py_core_object object and return the expected PyObject* result
	/// You need to PyDecRef the result object after use
	/// </summary>
	/// <param name="method">Method name</param>
	/// <param name="format">Variables description, see Py_BuildValue for more info</param>
	/// <param name="">...</param>
	/// <returns></returns>
	PyObject* get(std::string method, const char* format = NULL, ...);

};

#endif