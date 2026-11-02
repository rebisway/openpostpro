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

#include <postpro.h>
#include <stdexcept>
#include <filesystem>
#include <algorithm>

#include <python_api.h>

#include <script.h>
#include <strings.h>
#include <message_box.h>
#include <lang.h>
#include <inifile.h>
#include <directory.h>
#include <file.h>
#include <logger.h>
#include <drill.h>
#include <moveTo.h>


int Postpro::get_line_count()
{
	return get_int("get_line_count");
}

glm::vec3 Postpro::get_current_position()
{
	PyObject* result = get("get_current_position");
	glm::vec3 answer = glm::vec3(0,0,0);

	if (result != NULL)
	{
		if (PyList_Check(result) && PyList_Size(result) == 3)
		{
			if (PyFloat_Check(PyList_GetItem(result, 0)) || PyLong_Check(PyList_GetItem(result, 0)))
				answer.x = (float)PyFloat_AsDouble(PyList_GetItem(result, 0));
			if (PyFloat_Check(PyList_GetItem(result, 1)) || PyLong_Check(PyList_GetItem(result, 1)))
				answer.y = (float)PyFloat_AsDouble(PyList_GetItem(result, 1));
			if (PyFloat_Check(PyList_GetItem(result, 2)) || PyLong_Check(PyList_GetItem(result, 2)))
				answer.z = (float)PyFloat_AsDouble(PyList_GetItem(result, 2));
		}
		Py_DecRef(result);
	}
	else
	{
		auto message = error("get_current_position");
		throw std::runtime_error(message);
	}
	return answer;
}

void Postpro::set_postpro()
{
	set("set_postpro", "O", _py_post_object);
	
	PyObject* result = PyObject_CallMethod(_py_post_object, "set_core", "O", _py_core_object);

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

void Postpro::set_safe_position(float value)
{
	set("set_safe_position", "f", value);
}

bool Postpro::get_disable_z()
{
	return get_bool("get_disable_z");
}

std::string Postpro::start_loop()
{
	return get_string("start_loop");
}

std::string Postpro::stop_loop()
{
	return get_string("stop_loop");
}

std::string Postpro::start_program()
{
	return get_string("start_program");
}

std::string Postpro::stop_program()
{
	return get_string("stop_program");
}

std::string Postpro::start_group(std::string name, int tool_number, int tool_speed)
{
	return get_string("start_group", "sii", name.c_str(), tool_number, tool_speed);
}

std::string Postpro::stop_group()
{
	return get_string("stop_group");
}

std::string Postpro::start_tool_radius_compensation(ToolRadiusCompensation cmp)
{
	switch (cmp)
	{
	case ToolRadiusCompensation::None:
		return "";
	case ToolRadiusCompensation::Left:
		return get_string("tool_left_radius_compensation");
	case ToolRadiusCompensation::Right:
		return get_string("tool_right_radius_compensation");
	}
	return std::string();
}

std::string Postpro::stop_tool_radius_compensation(ToolRadiusCompensation cmp)
{
	if (cmp == ToolRadiusCompensation::None)
		return "";
	else
		return get_string("tool_cancel_radius_compensation");
}

std::string Postpro::start_tool_length_compensation(int number)
{
	if (number >= 0)
	{
		return get_string("tool_length_compensation", "i", number);
	}
	else
		return std::string();
}

std::string Postpro::stop_tool_length_compensation(int value)
{
	if (value >= 0)
		return get_string("tool_cancel_length_compensation");
	else
		return std::string();
}

std::string Postpro::start_toolpath()
{
	return get_string("start_toolpath");
}

std::string Postpro::stop_toolpath()
{
	return get_string("stop_toolpath");
}

std::string Postpro::start_single_path()
{
	return get_string("start_single_path");
}

std::string Postpro::stop_single_path()
{
	return get_string("stop_single_path");
}

Postpro::~Postpro()
{
	finalize();
}

bool Postpro::initialize(std::string module_path, std::string lang)
{
	Script::initialize("postcore", lang);

	if (stringex::end_with(module_path, ".py"))
	{
		_path = module_path;
		auto name = std::filesystem::path(module_path).filename().string();
		_module = name.substr(0, name.size() - 3);
	}
	else
		_module = module_path;

	// instanciate class Postpro into _py_post_object
	try
	{
		_py_post_module = PyImport_ImportModule(_module.c_str());

		if (_py_post_module != NULL && !PyErr_Occurred())
		{
			_py_post_class = PyObject_GetAttrString(_py_post_module, "Postpro");

			if (_py_post_class && PyCallable_Check(_py_post_class) && !PyErr_Occurred())
			{
				_py_post_object = PyObject_CallObject(_py_post_class, NULL);
			}
			else
			{
				_error = "Class \"Postpro\" is missing [" + Script::python_error() + "]";
				return false;
			}
		}
		else
		{
			_error = "Enable to load post-processor module [" + Script::python_error() + "]";
			return false;
		}

		set_postpro();

	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}
	// end of instanciation
	
	set("set_language", "s", _lang.c_str());

	IniFile ini;

	try 
	{
		_property_count = get_int("get_properties_count");
		_property_names.clear();
		_properties.clear();
		for (int i = 0; i < _property_count; i++)
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
					_properties.push_back(std::string(py_utf8(p)));
					_properties[i] = ini.get_string(_module, std::to_string(i), std::get<std::string>(_properties[i]));
					set("set_property", "is", i, (std::get<std::string>(_properties[i])).c_str());
				}
				else if (PyList_Check(p))
				{
					std::vector<std::string> values;
					for (int i = 0; i < PyList_Size(p); i++)
					{
						if (PyUnicode_Check(PyList_GetItem(p, i)))
							values.push_back(py_utf8(PyList_GetItem(p, i)));
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
		_extention = get_string("get_extention");
		_ijk_relative = get_bool("ijk_relative");
	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}

	return true;
}


void Postpro::finalize()
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

	if (_py_post_object != NULL ) Py_CLEAR(_py_post_object);
	if (_py_post_class != NULL) Py_CLEAR(_py_post_class);
	if (_py_post_module != NULL) Py_CLEAR(_py_post_module);

	Script::finalize();
}


bool Postpro::render_GUI()
{
	// must cleanup and load module because of different threading
	if (_first_call)
	{
		// as ImGui menu is called for each frame, we have to load properties at the first call
		// and we copy the properties into the properties_temp vector
		// if ok button is pressed, properties_temp is then copied back to properties
		initialize(_module, _lang);
		_first_call = false;
		_properties_config.clear();
		_properties_config.insert(_properties_config.begin(), _properties.begin(), _properties.end());
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(350.0f, 100.0f + _property_count * 25));
	ImGui::Begin((_description + "###POSTPRO").c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	for (int i = 0; i < _property_count; i++)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Spacing();
		ImGui::Text(_property_names[i].c_str());
		ImGui::TableSetColumnIndex(1);

		ImGui::SetNextItemWidth(-1);
		if (std::holds_alternative<bool>(_properties_config[i]))
		{
			bool value = std::get<bool>(_properties_config[i]);
			if (ImGui::Checkbox((std::string("##PROPERTY_BOOL") + std::to_string(i)).c_str(), &value))
			{
				_properties_config[i] = value;
			}
		}
		else if (std::holds_alternative<int>(_properties_config[i]))
		{
			int value = std::get<int>(_properties_config[i]);
			if (ImGui::InputInt((std::string("##PROPERTY_INT") + std::to_string(i)).c_str(), &value))
			{
				_properties_config[i] = value;
			}
		}
		else if (std::holds_alternative<float>(_properties_config[i]))
		{
			float value = std::get<float>(_properties_config[i]);
			if (ImGui::InputFloat((std::string("##PROPERTY_FLOAT") + std::to_string(i)).c_str(), &value))
			{
				_properties_config[i] = value;
			}
		}
		else if (std::holds_alternative<std::string>(_properties_config[i]))
		{
			std::string value = std::get<std::string>(_properties_config[i]);
			char t[200];
			sprintf_s(t, "%s", value.c_str());
			if (ImGui::InputText((std::string("##PROPERTY_STRING") + std::to_string(i)).c_str(), t, 200))
			{
				_properties_config[i] = std::string(t);
			}
		}
		else if (std::holds_alternative<std::vector<std::string>>(_properties_config[i]))
		{
			std::vector<std::string> values = std::get<std::vector<std::string>>(_properties_config[i]);
			char t[200];
			int index = 0, current=std::atoi(values[0].c_str());
			for (int j = 1; j < values.size(); j++)
			{
				for (int k = 0; k < values[j].size(); k++)
					t[index++] = values[j][k];
				t[index++] = 0;
			}
			t[index++] = 0;
			if (ImGui::Combo("##PROPERTY_COMBO", &current, t))
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
		_first_call = true;
		// ok is pressed so we copy temp config back to main config
		_properties.clear();
		_properties.insert(_properties.begin(), _properties_config.begin(), _properties_config.end());
	}
	ImGui::SameLine();
	if (ImGui::Button(Lang::l("CANCEL")))
		_first_call = true;
	ImGui::End();

	// finalize_python frees python memory and saves properties into ini file
	if (_first_call)
		finalize();

	return _first_call;
}

std::string Postpro::extention()
{
	return _extention;
}

bool Postpro::ijk_relative()
{
	return _ijk_relative;
}

std::string Postpro::comment(std::string value)
{
	return get_string("comment", "s", value.c_str());
}

std::string Postpro::rapid(char axe, float value)
{
	glm::vec3 values = _pos;
	if (axe == 'X')
		values.x = value;
	else if (axe == 'Y')
		values.y = value;
	else if (axe == 'Z')
		values.z = value;

	return linear(values, true);
}

std::string Postpro::rapid(glm::vec3 value)
{
	return linear(value, true);
}

std::string Postpro::linear(glm::vec3 value, bool rapid)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z && get_disable_z() == false)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}
	
	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, (rapid ? "rapid" : "linear"), "OO", axes, values);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method ") + (rapid ? "rapid" : "linear") + " is missing from Postpro";
			throw std::runtime_error(message);
		}

		_pos = value;
	}

	return answer;
}

std::string Postpro::circular(glm::vec3 value, glm::vec2 center, bool cw)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}
	
	float i = center.x, j = center.y;
	if (_ijk_relative)
	{
		i = center.x - _pos.x;
		j = center.y - _pos.y;
	}
	if (!_ijk_relative || _ijk_relative && i != 0)
	{
		a_axes.push_back("I");
		a_values.push_back(i);
	}
	if (!_ijk_relative || _ijk_relative && j != 0)
	{
		a_axes.push_back("J");
		a_values.push_back(j);
	}


	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, (cw ? "clockwise" : "counter_clockwise"), "OO", axes, values);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method ") + (cw ? "clockwise" : "counter_clockwise") + " is missing from Postpro";
			throw std::runtime_error(message);
		}

		_pos = value;
	}
	return answer;
}

std::string Postpro::feed(float value)
{
	return get_string("feed", "f", value);
}

std::string Postpro::drilling(glm::vec3 value, float retract, int pause)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}

	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, "drilling", "OOfi", axes, values, retract, pause);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method drilling is missing from Postpro");
			throw std::runtime_error(message);
		}

		_pos = value;
	}

	return answer;
}

std::string Postpro::pecking(glm::vec3 value, float retract, float delta)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}

	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, "pecking", "OOff", axes, values, retract, delta);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method pecking is missing from Postpro");
			throw std::runtime_error(message);
		}

		_pos = value;
	}

	return answer;
}

std::string Postpro::tapping(glm::vec3 value, float retract)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}

	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, "tapping", "OOf", axes, values, retract);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method tapping is missing from Postpro");
			throw std::runtime_error(message);
		}

		_pos = value;
	}

	return answer;
}

std::string Postpro::boring(glm::vec3 value, float retract, int pause)
{
	std::vector<std::string> a_axes;
	std::vector<float> a_values;

	if (value.x != _pos.x)
	{
		a_axes.push_back("X");
		a_values.push_back(value.x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(value.y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(value.z);
	}

	std::string answer;

	if (a_axes.size() > 0)
	{
		PyObject* axes = PyTuple_New(a_axes.size());
		PyObject* values = PyTuple_New(a_axes.size());
		for (int i = 0; i < a_axes.size(); i++)
		{
			PyTuple_SetItem(axes, i, PyUnicode_FromString(a_axes[i].c_str()));
			PyTuple_SetItem(values, i, PyFloat_FromDouble(a_values[i]));
		}
		PyObject* result = PyObject_CallMethod(_py_core_object, "boring", "OOfi", axes, values, retract, pause);

		Py_DECREF(axes);
		Py_DECREF(values);

		if (result != NULL)
		{
			if (PyUnicode_Check(result))
				answer = py_utf8(result);
			Py_DecRef(result);
		}
		else
		{
			auto message = std::string("Method boring is missing from Postpro");
			throw std::runtime_error(message);
		}

		_pos = value;
	}

	return answer;
}

std::string Postpro::pause(int value)
{
	return get_string("pause", "i", value);
}

std::string Postpro::line(std::string value)
{
	std::string result = "";
	value.erase(std::remove(value.begin(), value.end(), 'r'), value.end());
	auto lines = stringex::split(value, '\n');
	for (auto l : lines)
	{
		result += get_string("line", "s", l.c_str());
	}

	return result;
}

std::string Postpro::run(Document* doc, std::string path, std::string version)
{
	auto t = std::chrono::high_resolution_clock::now();

	initialize(_module, _lang);

	// ensure existence of path
	Directory::create(path);

	std::string output_path = environment::combine_path(path, std::filesystem::path(doc->path()).stem().string() + "." + extention());
	std::string output = "";


	output += start_loop();

	output += comment("Created with OpenPostPro " + version);
	output += comment("File : " + doc->path());
	output += comment("Date : " + environment::current_date_time());

	output += start_program();

	// we loop into groups
	for (Group* g : doc->groups())
	{
		if (g->generate())
		{
			output += comment("GROUP [" + g->name() + "]");

			if (!g->pre_command().empty())
				output += line(g->pre_command());

			set_safe_position(g->safe());

			output += start_group(g->name(), g->tool_number(), (int)g->spindle_speed());

			output += start_tool_radius_compensation(g->tool_radius_compensation());
			output += start_tool_length_compensation(g->tool_length_compensation());

			// we loop into toolpaths
			for (Toolpath* t : g->toolpaths())
			{
				output += comment("TOOLPATH [" + t->name() + "]");
				output += start_toolpath();
				for (auto c : t->coordinates())
				{
					_pos = get_current_position();

					// the tool is left to safe z
					output += rapid('Z', g->safe());

					// we rapid move to first coordinates
					output += rapid(glm::vec3(c[0].point.x, c[0].point.y, g->safe()));


					if (t->type() == GraphicType::CamDrill) // special case drilling
					{
						Drill* d = (Drill*)t;
						switch (d->mode())
						{
						case DrillMode::Drilling:
							output += drilling(glm::vec3(d->point().x, d->point().y, g->depth()), d->retract(), d->pause());
							break;
						case DrillMode::Pecking:
							output += pecking(glm::vec3(d->point().x, d->point().y, g->depth()), d->retract(), d->delta());
							break;
						case DrillMode::Tapping:
							output += tapping(glm::vec3(d->point().x, d->point().y, g->depth()), d->retract());
							break;
						case DrillMode::Boring:
							output += boring(glm::vec3(d->point().x, d->point().y, g->depth()), d->retract(), d->pause());
							break;
						}
					}
					else if (t->type() == GraphicType::CamMoveTo) // special case move to
					{
						MoveTo* d = (MoveTo*)t;
						output += rapid(glm::vec3(d->point().x, d->point().y, g->safe()));
						if (d->pause() > 0)
							output += pause(d->pause());
					}
					else
					{
						// we plung
						if (get_disable_z() == false)
						{
							float z = std::max(g->origin() - g->pass(), g->depth());
							bool first_z = true;

							while (z >= g->depth())
							{
								// go down to Z working position
								output += feed(g->plung_feed());
								output += linear(glm::vec3(c[0].point.x, c[0].point.y, z));
								output += feed(g->feed());

								// if first pass, call start_single_path
								if (first_z)
								{
									output += start_single_path();
									first_z = false;
								}

								auto from = c.front();
								auto next = c.begin() + 1;

								while (next != c.end())
								{
									if (from.type == SegmentType::Line)
										output += linear(glm::vec3((*next).point, z));
									else
										output += circular(glm::vec3((*next).point, z), from.center, from.cw);
									from = (*next);
									next = std::next(next);
								}

								if (z == g->depth())
									break;
								else
									z = std::max(z - g->pass(), g->depth());
							}
						}
						else
						{
							output += start_single_path();

							auto from = c.front();
							auto next = c.begin() + 1;
							while (next != c.end())
							{
								if (from.type == SegmentType::Line)
									output += linear(glm::vec3((*next).point, 0));
								else
									output += circular(glm::vec3((*next).point, 0), from.center, from.cw);
								from = (*next);
								next = std::next(next);
							}
						}

						output += stop_single_path();
					}

					output += stop_toolpath();
				}
			}

			output += stop_tool_length_compensation(g->tool_length_compensation());
			output += stop_tool_radius_compensation(g->tool_radius_compensation());

			output += stop_group();

			if (!g->post_command().empty())
				output += line(g->post_command());
		}
	}

	output += stop_program();

	output += stop_loop();
	
	auto line_count = get_line_count();

	try {
		file::write_all_text(output_path, output);
	}
	catch (const std::runtime_error& e)
	{
		Logger::error(std::string("Writing file (") + output_path + ") [" + e.what() + "]");
	}


	finalize();

	doc->output(output_path);

	Logger::log("Output time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
	Logger::log("Output path : " + output_path);
	Logger::log("Output lines : " + std::to_string(line_count));

	return std::string();
}