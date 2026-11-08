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
#include <variant>

// Forward declaration of the CPython object type, see python_api.h to call the CPython API
struct _object;
typedef struct _object PyObject;

enum class MessageBoxResult;

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

	// properties (parameters) of the script, edited with the properties dialog box
	typedef std::variant<bool, int, float, std::string, std::vector<std::string>> PropertyValue;
	int _property_count = 0;
	bool _use_properties = true;					// false if the script manages its own dialog box
	std::vector<PropertyValue> _properties;
	std::vector<PropertyValue> _properties_config;	// working copy edited by the dialog box
	std::vector<std::string> _property_names;		// texts displayed in the dialog box
	std::vector<std::string> _property_keys;		// names used to save the values
	// a text property can be a file name: a button opens the file dialog of the system (see get_property_file of the script)
	struct PropertyFile
	{
		int mode = 0;								// 0: not a file, 1: file to open, 2: file to save
		std::string filter_name;
		std::string filter_pattern;
	};
	std::vector<PropertyFile> _property_files;

	PyObject* _py_core_module = nullptr;
	PyObject* _py_core_class = nullptr;
	PyObject* _py_core_object = nullptr;

protected:
	/// <summary>
	/// Return an error text including method name from _py_core_object object and CPython error desc
	/// </summary>
	/// <returns></returns>
	std::string error(std::string method);

	/// <summary>
	/// Read the properties from the python object, the saved values (ini file) replace the default ones
	/// and are sent back to the python object
	/// </summary>
	void load_properties();

	/// <summary>
	/// Save the properties values into the ini file, each value is saved with the key of its property
	/// </summary>
	void save_properties();

	/// <summary>
	/// Send the property value to the python object
	/// </summary>
	void apply_property(int index);

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

	/// <summary>
	/// Static method. True if python finds a module with this name (script, post-processor or library)
	/// </summary>
	static bool module_exists(std::string name);

	/// <summary>
	/// Static method. Compile a python source without running it
	/// </summary>
	/// <param name="source">Python code</param>
	/// <param name="filename">Name used in the error messages</param>
	/// <param name="line">Line of the error (from 1), 0 if unknown</param>
	/// <param name="column">Column of the error (from 1), 0 if unknown</param>
	/// <returns>The error message, empty if the code is correct</returns>
	static std::string check_syntax(const std::string& source, const std::string& filename, int& line, int& column);

	/// <summary>
	/// Static method. Import again a module already loaded, to use its modified source
	/// </summary>
	/// <returns>The error message, empty if success</returns>
	static std::string reload_module(const std::string& name);

	/// <summary>
	/// Static method. True if the file is one of those delivered with the application
	/// </summary>
	static bool is_delivered(const std::string& path);

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
	/// True if the script has properties and uses the properties dialog box
	/// </summary>
	bool has_properties_dialog() { return _property_count > 0 && _use_properties; }

	/// <summary>
	/// True for the files delivered with the application: they are copied again at each start,
	/// so any modification would be lost
	/// </summary>
	bool read_only();

	/// <summary>
	/// Prepare the dialog box, call it once before calling draw_properties for each frame
	/// </summary>
	void begin_properties();

	/// <summary>
	/// Draw the properties dialog box. When Ok is pressed, values are kept and sent to the python object
	/// </summary>
	/// <returns>None while the dialog box is open</returns>
	MessageBoxResult draw_properties();

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