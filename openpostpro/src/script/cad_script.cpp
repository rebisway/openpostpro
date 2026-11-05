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

#include <cad_script.h>

#include <filesystem>

#include <python_api.h>
#include <inifile.h>
#include <strings.h>
#include <cstdarg>
#include <logger.h>
#include <config.h>
#include <line.h>
#include <circle.h>
#include <arc.h>
#include <point.h>
#include <ellipse.h>
#include <polyline.h>
#include <spline.h>
#include <text.h>

// helpers building the python description of a shape: a dictionary with the id, the type and the parameters
static PyObject* py_vec(glm::vec2 v)
{
	return Py_BuildValue("(dd)", (double)v.x, (double)v.y);
}

static void py_put(PyObject* dict, const char* key, PyObject* value)
{
	if (value != NULL)
	{
		PyDict_SetItemString(dict, key, value);
		Py_DECREF(value);
	}
}

static PyObject* py_text(const std::string& value)
{
	return PyUnicode_FromString(value.c_str());
}

static PyObject* shape_to_dict(Shape* shape)
{
	PyObject* d = PyDict_New();
	py_put(d, "id", PyLong_FromUnsignedLong(shape->id()));
	py_put(d, "layer", py_text(shape->parent()));

	switch (shape->type())
	{
	case GraphicType::Line:
	{
		auto l = (Line*)shape;
		py_put(d, "type", py_text("line"));
		py_put(d, "p1", py_vec(l->p1()));
		py_put(d, "p2", py_vec(l->p2()));
		break;
	}
	case GraphicType::Circle:
	{
		auto c = (Circle*)shape;
		py_put(d, "type", py_text("circle"));
		py_put(d, "center", py_vec(c->center()));
		py_put(d, "radius", PyFloat_FromDouble(c->radius()));
		break;
	}
	case GraphicType::Arc:
	{
		auto a = (Arc*)shape;
		py_put(d, "type", py_text("arc"));
		py_put(d, "start", py_vec(a->start()));
		py_put(d, "center", py_vec(a->center()));
		py_put(d, "stop", py_vec(a->stop()));
		py_put(d, "cw", PyBool_FromLong(a->cw()));
		py_put(d, "radius", PyFloat_FromDouble(a->radius()));
		break;
	}
	case GraphicType::Point:
	{
		auto p = (Point*)shape;
		py_put(d, "type", py_text("point"));
		py_put(d, "p1", py_vec(p->p1()));
		break;
	}
	case GraphicType::Ellipse:
	{
		auto e = (Ellipse*)shape;
		py_put(d, "type", py_text("ellipse"));
		py_put(d, "center", py_vec(e->center()));
		py_put(d, "major", PyFloat_FromDouble(e->major()));
		py_put(d, "minor", PyFloat_FromDouble(e->minor()));
		py_put(d, "start", PyFloat_FromDouble(e->start()));
		py_put(d, "stop", PyFloat_FromDouble(e->stop()));
		py_put(d, "angle", PyFloat_FromDouble(e->angle()));
		break;
	}
	case GraphicType::Polyline:
	{
		static const char* modes[] = { "polyline", "bspline", "natural_cubic", "lagrange", "catmull", "bezier" };
		auto p = (Polyline*)shape;
		py_put(d, "type", py_text("polyline"));
		py_put(d, "mode", py_text(modes[(int)p->mode()]));
		PyObject* points = PyList_New(0);
		for (auto v : p->points())
		{
			PyObject* item = py_vec(v);
			PyList_Append(points, item);
			Py_DECREF(item);
		}
		py_put(d, "points", points);
		py_put(d, "closed", PyBool_FromLong(glm::distance(p->first(), p->last()) < 0.0001f));
		break;
	}
	case GraphicType::Spline:
	{
		// each segment starts at its point; an arc is described by its center and its direction
		static const char* kinds[] = { "line", "arc", "circle" };
		auto sp = (Spline*)shape;
		py_put(d, "type", py_text("spline"));
		PyObject* segments = PyList_New(0);
		for (auto& segment : sp->curve())
		{
			PyObject* item = PyDict_New();
			py_put(item, "type", py_text(kinds[(int)segment.type]));
			py_put(item, "point", py_vec(segment.point));
			if (segment.type != SegmentType::Line)
			{
				py_put(item, "center", py_vec(segment.center));
				py_put(item, "radius", PyFloat_FromDouble(segment.radius));
				py_put(item, "cw", PyBool_FromLong(segment.cw));
			}
			PyList_Append(segments, item);
			Py_DECREF(item);
		}
		py_put(d, "segments", segments);
		py_put(d, "closed", PyBool_FromLong(glm::distance(sp->first(), sp->last()) < 0.0001f));
		break;
	}
	case GraphicType::Text:
	{
		auto t = (Text*)shape;
		py_put(d, "type", py_text("text"));
		py_put(d, "p1", py_vec(t->p1()));
		py_put(d, "text", py_text(t->text()));
		py_put(d, "family", py_text(t->fontFamily()));
		py_put(d, "height", PyFloat_FromDouble(t->height()));
		py_put(d, "inter", PyFloat_FromDouble(t->inter()));
		py_put(d, "bold", PyBool_FromLong(t->bold()));
		py_put(d, "italic", PyBool_FromLong(t->italic()));
		break;
	}
	default:
		Py_DECREF(d);
		return NULL;
	}

	return d;
}

void CadScript::set_context(glm::vec2 mouse_pos, Document* doc)
{
	try
	{
		set("set_mouse", "ff", mouse_pos.x, mouse_pos.y);

		if (doc != nullptr)
		{
			set("set_unit", "s", doc->unit() == Unit::Inch ? "inch" : "mm");

			// only the shapes are sent: the toolpaths and the groups are not drawing
			PyObject* list = PyList_New(0);
			for (Graphic* g : doc->selected())
			{
				if (g->shape())
				{
					PyObject* item = shape_to_dict((Shape*)g);
					if (item != NULL)
					{
						PyList_Append(list, item);
						Py_DECREF(item);
					}
				}
			}
			set("set_selection", "O", list);
			Py_DECREF(list);
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("Running script " + module() + " -> " + std::string(e.what()));
	}
}

void CadScript::set_cadscript()
{
	set("set_script", "O", _py_script_object);

	PyObject* result = PyObject_CallMethod(_py_script_object, "set_core", "O", _py_core_object);

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

CadScript::~CadScript()
{
	finalize();
}

bool CadScript::initialize(std::string module_path, std::string lang)
{
	Script::initialize("cadcore", lang);
	
	if (!module_path.empty())
	{
		if (stringex::end_with(module_path, ".py"))
		{
			_path = module_path;
			auto name = std::filesystem::path(module_path).filename().string();
			_module = name.substr(0, name.size() - 3);
		}
		else
			_module = module_path;

		if (stringex::end_with(module_path, ".py"))
		{
			_path = module_path;
			auto name = std::filesystem::path(module_path).filename().string();
			_module = name.substr(0, name.size() - 3);
		}
		else
			_module = module_path;
	}

	// instanciate class Postpro into _py_post_object
	try
	{
		_py_script_module = PyImport_ImportModule(_module.c_str());

		if (_py_script_module != NULL && !PyErr_Occurred())
		{
			_py_script_class = PyObject_GetAttrString(_py_script_module, "Cadscript");

			if (_py_script_class && PyCallable_Check(_py_script_class) && !PyErr_Occurred())
			{
				_py_script_object = PyObject_CallObject(_py_script_class, NULL);
			}
			else
			{
				_error = "Class \"Cadscript\" is missing [" + Script::python_error() + "]";
				return false;
			}
		}
		else
		{
			_error = "Enable to load script module [" + Script::python_error() + "]";
			return false;
		}

		set_cadscript();

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
	}
	catch (const std::exception& e)
	{
		_error = e.what();
		return false;
	}

	return true;
}


void CadScript::finalize()
{
	save_properties();

	if (_py_script_object != NULL) Py_CLEAR(_py_script_object);
	if (_py_script_class != NULL) Py_CLEAR(_py_script_class);
	if (_py_script_module != NULL) Py_CLEAR(_py_script_module);

	Script::finalize();
}

std::string CadScript::run(glm::vec2 mouse_pos, Document* doc)
{
	initialize();

	std::string result;

	set_context(mouse_pos, doc);

	try
	{
		result = get_string("run");
	}
	catch (const std::exception& e)
	{
		Logger::error("Running script " + module() + " -> " + std::string(e.what()));
	}

	finalize();

	return result;
}

bool CadScript::begin_dialog(glm::vec2 mouse_pos, Document* doc)
{
	_mouse = mouse_pos;

	if (!initialize())
	{
		Logger::error("Running script " + module() + " -> " + error());
		finalize();
		return false;
	}

	// the selection is the one of the click on the menu, it does not change while the dialog box is open
	set_context(mouse_pos, doc);

	begin_properties();
	return true;
}

MessageBoxResult CadScript::render_GUI(std::string& output)
{
	auto result = draw_properties();

	if (result == MessageBoxResult::Ok)
	{
		// the python object already holds the values chosen in the dialog box
		try
		{
			set("set_mouse", "ff", _mouse.x, _mouse.y);
			output = get_string("run");
		}
		catch (const std::exception& e)
		{
			Logger::error("Running script " + module() + " -> " + std::string(e.what()));
		}
	}

	// finalize saves the properties and frees python memory
	if (result != MessageBoxResult::None)
		finalize();

	return result;
}
