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
#include "travel_sort.h"
#include "cut_path.h"
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
	// the core knows the values of the program: the drawing has the opposite ones when the axes are reversed
	return _machine ? _machine->from_machine(answer) : answer;
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
	set("set_safe_position", "f", to_machine(glm::vec3(0, 0, value)).z);
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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z && get_disable_z() == false)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
	}
	
	float i = center.x, j = center.y;
	if (_ijk_relative)
	{
		i = center.x - _pos.x;
		j = center.y - _pos.y;
	}

	// reversed axes: the center and the direction of the arc in the program
	if (_machine)
		_machine->arc_to_machine(i, j, cw);
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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
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
		PyObject* result = PyObject_CallMethod(_py_core_object, "drilling", "OOfi", axes, values, to_machine(glm::vec3(0, 0, retract)).z, pause);

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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
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
		PyObject* result = PyObject_CallMethod(_py_core_object, "pecking", "OOff", axes, values, to_machine(glm::vec3(0, 0, retract)).z, delta);

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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
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
		PyObject* result = PyObject_CallMethod(_py_core_object, "tapping", "OOf", axes, values, to_machine(glm::vec3(0, 0, retract)).z);

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
		a_values.push_back(to_machine(value).x);
	}
	if (value.y != _pos.y)
	{
		a_axes.push_back("Y");
		a_values.push_back(to_machine(value).y);
	}
	if (value.z != _pos.z)
	{
		a_axes.push_back("Z");
		a_values.push_back(to_machine(value).z);
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
		PyObject* result = PyObject_CallMethod(_py_core_object, "boring", "OOfi", axes, values, to_machine(glm::vec3(0, 0, retract)).z, pause);

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
	value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
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

	// a machine with reversed axes: the values of the program are the opposite ones of the drawing
	_machine = doc->has_machine() && doc->machine().reversed() ? &doc->machine() : nullptr;

	initialize(_module, _lang);

	// the post-processor works in the unit of the document
	set("set_unit", "s", doc->unit() == Unit::Inch ? "inch" : "mm");

	// the profile lifts the tool to its own safe height (5) before the first group gives its one: with a reversed Z that height is below
	if (_machine)
		set_safe_position(5.0f);

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
			output += comment("OPERATION [" + g->name() + "]");

			if (!g->pre_command().empty())
				output += line(g->pre_command());

			set_safe_position(g->safe());

			output += start_group(g->name(), g->tool_number(), (int)g->spindle_speed());

			output += start_tool_length_compensation(g->tool_length_compensation());

			// the depths of the passes: the tool goes down by one pass, all the toolpaths of the group are machined at this depth, then the
			// tool goes down again... (without Z, a single pass)
			std::vector<float> levels;
			if (g->is_torch() && get_disable_z() == false)
				// a torch cuts through the material in one pass, at the cutting height
				levels.push_back(g->depth());
			else if (get_disable_z() == false)
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

			// The independent contours (follow, offset, drilling) which follow each other are sorted to shorten the rapid moves of the tool.
			// A pocket is sorted by itself (its finishing pass stays at the end). The other toolpaths (spiral, move) keep their order and the order of
			// their curves, and are the limit of a group of sorted ones.
			struct Item
			{
				Toolpath* toolpath;
				Curve curve;
				bool lead_in = false;	// the first segment of the curve is the entry of the offset start
			};
			std::vector<std::vector<Item>> runs;
			std::vector<bool> run_sorted;
			// a run of independent contours takes the next independent toolpath
			std::vector<bool> run_open;
			for (Toolpath* t : g->toolpaths())
			{
				std::vector<Curve> curves = t->coordinates();

				// a pocket: all the curves are sorted together (the parts of the pocket are machined one after the other, not the rings of
				// a part, then the rings of the next part...) but the finishing pass stays at the end
				if (t->type() == GraphicType::CamPocket)
				{
					const size_t split = curves.size() - std::min(t->final_count(), curves.size());
					for (int part = 0; part < 2; part++)
					{
						runs.push_back(std::vector<Item>());
						run_sorted.push_back(true);
						run_open.push_back(false);
						for (size_t i = part == 0 ? 0 : split; i < (part == 0 ? split : curves.size()); i++)
							runs.back().push_back({ t, curves[i], false });
					}
					continue;
				}

				const bool independent = t->type() == GraphicType::CamFollow || t->type() == GraphicType::CamOffset || t->type() == GraphicType::CamDrill;
				if (runs.empty() || !independent || !run_open.back())
				{
					runs.push_back(std::vector<Item>());
					run_sorted.push_back(independent);
					run_open.push_back(independent);
				}
				// an entry exists when the toolpath has a single curve (the offset start is computed for it only)
				const bool lead_in = t->start_point_allowed() && t->start_point_type() != StartPointType::normal && curves.size() == 1;
				for (Curve& c : curves)
					runs.back().push_back({ t, c, lead_in });
			}
			// The contours inside of another one are machined before it, at any depth: the holes of a part, then its outline (and an island
			// which is inside of a hole before the hole). before[r][i]: the items of the run r which must be done before the item i.
			std::vector<std::vector<std::vector<int>>> before(runs.size());
			for (size_t r = 0; r < runs.size(); r++)
			{
				if (!run_sorted[r])
					continue;
				std::vector<Item>& items = runs[r];
				std::vector<Curve*> contours;
				for (Item& item : items)
					contours.push_back(item.toolpath->contour());
				bool any = false;
				std::vector<std::vector<int>> list(items.size());
				for (size_t i = 0; i < items.size(); i++)
				{
					if (contours[i] == nullptr)
						continue;
					geometry::rectangle& outer = contours[i]->bounds();
					for (size_t j = 0; j < items.size(); j++)
					{
						if (i == j || contours[j] == nullptr || items[i].toolpath == items[j].toolpath)
							continue;
						// the bounds first: a contour which is inside of another one has its bounds inside of the bounds of the other one
						geometry::rectangle& inner = contours[j]->bounds();
						if (inner.left() < outer.left() || inner.right() > outer.right() || inner.top() > outer.top() || inner.bottom() < outer.bottom())
							continue;
						if (contours[j]->inside(*contours[i]))
						{
							list[i].push_back((int)j);
							any = true;
						}
					}
				}
				if (any)
					before[r] = list;
			}

			// a toolpath out of the area of the machine can not be machined: the file is written anyway, with a warning
			if (doc->has_machine())
			{
				bool outside = false;
				for (auto& items : runs)
					for (Item& item : items)
						for (const Segment& segment : item.curve)
							if (!doc->machine().contains(segment.point))
								outside = true;
				if (outside)
				{
					Logger::log("Warning: the group [" + g->name() + "] goes out of the area of the machine [" + doc->machine().name + "]");
					output += comment("WARNING: out of the area of the machine " + doc->machine().name);
				}
			}
			glm::vec2 cursor(0.0f);
			// the items of the runs with nested contours as they are before the sort: before[r] gives indices in this list
			std::vector<std::vector<Item>> unsorted(runs.size());
			for (size_t r = 0; r < runs.size(); r++)
			{
				if (!run_sorted[r])
					continue;
				if (!before[r].empty())
					unsorted[r] = runs[r];
				std::vector<glm::vec2> starts, ends;
				for (Item& item : runs[r])
				{
					starts.push_back(item.curve.front().point);
					ends.push_back(item.curve.back().point);
				}
				auto order = sort_by_travel(starts, ends, cursor, before[r]);
				std::vector<Item> sorted;
				for (int i : order)
					sorted.push_back(runs[r][i]);
				runs[r] = sorted;
			}

			// we loop into the passes, then into the curves
			for (size_t level = 0; level < levels.size(); level++)
			{
				const float z = levels[level];

				// the sorted ones are done backward every other pass: the tool starts where the previous pass ended
				std::vector<const Item*> sequence;
				for (size_t r = 0; r < runs.size(); r++)
				{
					if (run_sorted[r] && level % 2 == 1 && !before[r].empty())
					{
						// nested contours: the order is made again from where the previous pass ended, the inner ones first
						std::vector<glm::vec2> starts, ends;
						for (Item& item : unsorted[r])
						{
							starts.push_back(item.curve.front().point);
							ends.push_back(item.curve.back().point);
						}
						glm::vec2 here = sequence.empty() ? glm::vec2(0.0f) : sequence.back()->curve.back().point;
						for (int i : sort_by_travel(starts, ends, here, before[r]))
							sequence.push_back(&unsorted[r][i]);
					}
					else if (run_sorted[r] && level % 2 == 1)
						for (size_t i = runs[r].size(); i > 0; i--)
							sequence.push_back(&runs[r][i - 1]);
					else
						for (Item& item : runs[r])
							sequence.push_back(&item);
				}

				Toolpath* current = nullptr;
				ToolRadiusCompensation compensation = ToolRadiusCompensation::None;
				for (const Item* item : sequence)
				{
					Toolpath* t = item->toolpath;
					Curve c = item->curve;

					// drillings and moves are done once, with the first pass
					if (level > 0 && (t->type() == GraphicType::CamDrill || t->type() == GraphicType::CamMoveTo))
						continue;

					if (t != current)
					{
						if (current != nullptr)
							output += stop_toolpath();
						current = t;
						output += comment("TOOLPATH [" + t->name() + "]");
						output += start_toolpath();
						// radius compensation (G41 / G42) of the toolpath: only a compensated "follow" has one
						compensation = t->radius_compensation();
					}

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
								// go down to Z working position: vertical, or along the entry of the curve (oblique), from the safe height
								CutPathParams params;
								params.z = z;
								params.z_top = level == 0 ? g->origin() : levels[level - 1];
								params.safe = g->safe();
								params.plunge_feed = g->plung_feed();
								params.feed = g->feed();
								// tabs keep the tool at the tab height, with a vertical move at their boundaries
								params.tab_z = g->depth() + t->tabs_height();
								params.oblique = g->oblique() && !g->is_torch() && item->lead_in;
								output += cut_path(*this, c, params);
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

					}
				}
				if (current != nullptr)
					output += stop_toolpath();
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
	_machine = nullptr;

	Logger::log("Output time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
	Logger::log("Output path : " + output_path);
	Logger::log("Output lines : " + std::to_string(line_count));

	return std::string();
}