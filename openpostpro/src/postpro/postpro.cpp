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
#include <config.h>
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

	try 
	{
		load_properties();

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
	save_properties();

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
		// the dialog box edits a copy, copied back to the properties if the ok button is pressed
		initialize(_module, _lang);
		_first_call = false;
		begin_properties();
	}

	auto result = draw_properties();

	if (result != MessageBoxResult::None)
	{
		// finalize frees python memory and saves properties into ini file
		_first_call = true;
		finalize();
	}

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

	// the post-processor works in the unit of the document
	set("set_unit", "s", doc->unit() == Unit::Inch ? "inch" : "mm");

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

			output += start_tool_length_compensation(g->tool_length_compensation());

			// the depths of the passes: the tool goes down by one pass, all the toolpaths of the group are machined at this depth, then the
			// tool goes down again... (without Z, a single pass)
			std::vector<float> levels;
			if (get_disable_z() == false)
			{
				float z = std::max(g->origin() - g->pass(), g->depth());
				levels.push_back(z);
				while (z > g->depth() && g->pass() > 0)
				{
					z = std::max(z - g->pass(), g->depth());
					levels.push_back(z);
				}
			}
			else
				levels.push_back(0);

			// we loop into the passes, then into toolpaths
			for (size_t level = 0; level < levels.size(); level++)
			{
				const float z = levels[level];

				for (Toolpath* t : g->toolpaths())
				{
					// drillings and moves are done once, with the first pass
					if (level > 0 && (t->type() == GraphicType::CamDrill || t->type() == GraphicType::CamMoveTo))
						continue;

					output += comment("TOOLPATH [" + t->name() + "]");
					output += start_toolpath();
					// radius compensation (G41 / G42) of the toolpath: only a compensated "follow" has one
					ToolRadiusCompensation compensation = t->radius_compensation();

					for (auto c : t->coordinates())
					{
						_pos = get_current_position();

						// the tool is left to safe z
						output += rapid('Z', g->safe());

						// we rapid move to first coordinates
						output += rapid(glm::vec3(c[0].point.x, c[0].point.y, g->safe()));
						output += start_tool_radius_compensation(compensation);


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
							if (get_disable_z() == false)
							{
								// go down to Z working position: the plunge is vertical, the tool comes from the safe height
								output += feed(g->plung_feed());
								output += linear(glm::vec3(c[0].point.x, c[0].point.y, z));
								output += feed(g->feed());

								output += start_single_path();

								auto from = c.front();
								auto next = c.begin() + 1;
								float last_z = z;
								// tabs keep the tool at the tab height, with a vertical move at their boundaries
								const float tab_z = g->depth() + t->tabs_height();

								while (next != c.end())
								{
									float seg_z = from.tab ? std::max(z, tab_z) : z;
									if (seg_z != last_z)
									{
										output += linear(glm::vec3(from.point, seg_z));
										last_z = seg_z;
									}
									if (from.type == SegmentType::Line)
										output += linear(glm::vec3((*next).point, seg_z));
									else
										output += circular(glm::vec3((*next).point, seg_z), from.center, from.cw);
									from = (*next);
									next = std::next(next);
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
							output += stop_tool_radius_compensation(compensation);
						}

						output += stop_toolpath();
					}
				}
			}

			output += stop_tool_length_compensation(g->tool_length_compensation());

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