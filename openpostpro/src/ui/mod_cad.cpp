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

#include "mod_cad.h"
#include <lang.h>
#include <config.h>
#include <logger.h>
#include <geometry.h>
#include <imgui_internal.h>
#include <anchor.h>
#include <glm/gtx/transform.hpp>

#include <point.h>
#include <line.h>
#include <circle.h>
#include <arc.h>
#include <ellipse.h>
#include <polyline.h>
#include <text.h>
#include <environment.h>
#include <strings.h>
#include <clipboard.h>
#include <cad.h>
#include <moveTo.h>
#include <follow.h>
#include <offset.h>
#include <drill.h>
#include <pocket.h>
#include<chrono>

ModCad::ModCad(Window* window) : Module(window)
{
	_code = "MOD_CAD";
	_index = 0;
	_title = Lang::l("WINDOW_CAD");
	_full_screen_ui = false;
	_selected = true;

	_camera = new OrthoCamera();

	_render = new OpenGlRenderer();
	_render->initialize();
	_render->initialize_GLFW();
	_render->blend(true);
	_render->camera(_camera);

	std::vector<glm::vec3> axe{ glm::vec3(_width * 2, 0.0f, 0.0f), glm::vec3(-_width * 2, 0.0f, 0.0f), glm::vec3(0.0f, -_height * 2, 0.0f), glm::vec3(0.0f, _height * 2, 0.0f) };
	_b_axe = _render->create_buffer(axe);
	_b_grid = _render->create_buffer(axe);
	_b_decoration = _render->create_buffer(axe);
	_b_selection = _render->create_buffer(axe);
	_b_cursor = _render->create_buffer(axe);
	_b_magnet = _render->create_buffer(axe);

	// loading icon textures
	_textures["CAD_POINT"] = _render->load_texture("point.png");
	_textures["CAD_LINE"] = _render->load_texture("line.png");
	_textures["CAD_CIRCLE"] = _render->load_texture("circle.png");
	_textures["CAD_ARC"] = _render->load_texture("arc.png");
	_textures["CAD_ELLIPSE"] = _render->load_texture("ellipse.png");
	_textures["CAD_POLYLINE"] = _render->load_texture("polyline.png");
	_textures["CAD_TEXT"] = _render->load_texture("text.png");
	_textures["CAD_ORIGIN"] = _render->load_texture("origin.png");
	_textures["CAD_OFFSET"] = _render->load_texture("cad-offset.png");
	_textures["CAD_SYMMETRY"] = _render->load_texture("symmetry.png");
	_textures["CAD_SCALE"] = _render->load_texture("scale.png");
	_textures["CAD_ROTATION"] = _render->load_texture("rotation.png");
	_textures["CAD_CHAMFER"] = _render->load_texture("chamfer.png");
	_textures["CAM_MOVETO"] = _render->load_texture("move_to.png");
	_textures["CAM_DRILL"] = _render->load_texture("drill.png");
	_textures["CAM_FOLLOW"] = _render->load_texture("follow.png");
	_textures["CAM_OFFSET"] = _render->load_texture("offset.png");
	_textures["CAM_POCKET"] = _render->load_texture("pocket.png");
	_textures["BTN_ADD"] = _render->load_texture("plus.png");
	_textures["BTN_REMOVE"] = _render->load_texture("minus.png");
	_textures["BTN_UP"] = _render->load_texture("up.png");
	_textures["BTN_DOWN"] = _render->load_texture("down.png");
}

ModCad::~ModCad()
{
	delete _b_grid;
	delete _b_decoration;
	delete _b_selection;
	delete _b_magnet;
	delete _b_axe;
	delete _render;
	delete _camera;
}

/// ***************************************************************************************************
/// 
///	SELECTION/UNSELECTION
/// 
/// ***************************************************************************************************

bool ModCad::show()
{
	return true;
}

bool ModCad::closable()
{
	return false;
}

void ModCad::unselect()
{
	_document->unselect_all();
}

void ModCad::unselect(Graphic* g)
{
	_document->unselect(g);
}

void ModCad::select(Graphic* g)
{
	_document->unselect_all();
	_document->select(g);

	if (g->type() == GraphicType::Layer)
		_document->current_layer((Layer*)g);

	if (g->type() == GraphicType::Group)
		_document->current_group((Group*)g);
}

void ModCad::select(std::vector<Graphic*> graphics)
{
	_document->select(graphics);
}

void ModCad::select(std::vector<Shape*> shapes)
{
	_document->unselect_all();
	for (Shape* s : shapes)
		_document->select(s);
}

void ModCad::select(std::vector<int> ids)
{
	_document->unselect_all();
	for (Layer* l : _document->layers())
	{
		for (Shape* s : l->shapes())
		{
			if (std::find(ids.begin(), ids.end(), s->id()) != ids.end())
				_document->select(s);
		}
	}
}

void ModCad::selecting(Shape* shape)
{
	if (_shift) // if shift pressed, unselect the shape
	{
		shape->selected(false);
		auto it = std::find(_document->selected().begin(), _document->selected().end(), shape);
		if (it != _document->selected().end())
		{
			_document->selected().erase(it);
		}
	}
	else if (_ctrl || _shape_hovered == nullptr ) // select the shape
	{
		shape->selected(true);
		auto it = std::find(_document->selected().begin(), _document->selected().end(), shape);
		if (it == _document->selected().end())
		{
			_document->selected().push_back(shape);
		}
	}
	else // select only the shape
		select(shape);
}

/// ***************************************************************************************************
/// 
///	ADD/Removing shapes
/// 
/// ***************************************************************************************************

void ModCad::construct_shape()
{
	ArcMode _arc_mode = ArcMode::Center;
	_construction_shape->done(true);
	_document->current_layer()->add(_construction_shape);
	
	// history
	//push_action(DoActionType::Add, _construction_shape->write());
	History::undo(HistoryActionType::Add, _construction_shape->write());
	
	select(_construction_shape);
	PolylineMode mode;
	switch (_construction_shape->type())
	{
	case GraphicType::Circle:
		_construction_shape = new Circle(_render);
		break;
	case GraphicType::Point:
		_construction_shape = new Point(_render);
		break;
	case GraphicType::Arc:
		_arc_mode = ((Arc*)_construction_shape)->mode();
		_construction_shape = new Arc(_render);
		((Arc*)_construction_shape)->mode(_arc_mode);
		break;
	case GraphicType::Ellipse:
		_construction_shape = new Ellipse(_render);
		break;
	case GraphicType::Line:
		_construction_shape = new Line(_render);
		break;
	case GraphicType::Polyline:
		mode = ((Polyline*)_construction_shape)->mode();
		_construction_shape = new Polyline(_render);
		((Polyline*)_construction_shape)->mode(mode);
		break;
	case GraphicType::Text:
		_construction_shape = nullptr;
		_cad_creation = GraphicType::None;
		break;
	}
}

void ModCad::remove(std::vector<Shape*> shapes)
{
	std::string data = "";
	for (Shape* s : shapes)
		data += s->write() + '\n';

	History::undo(HistoryActionType::Delete, data);

	for (Shape* s : shapes)
	{
		if (s->selected())
			unselect(s);
		_document->layer(s->parent())->remove(s);
	}
}

void ModCad::add(std::vector<Shape*> shapes)
{
	std::string data = "";
	for (Shape* s : shapes)
		data += s->write() + '\n';

	History::undo(HistoryActionType::Add, data);

	for (Shape* s : shapes)
	{
		_document->layer(s->parent())->add(s);
	}
}


void ModCad::document(Document* document)
{
	Module::document(document);

	document->render(_render);
}

void ModCad::document_loaded()
{
	// updating groups
	for (auto g : _document->groups())
	{
		for (auto t : g->toolpaths())
		{
			auto  references = t->references();
			if (references.size() > 0)
			{
				select(t->references());
				TreeCurve* tree = cad_tree();
				t->tree(cad_tree());
				t->compute();
				delete tree;
			}
		}
	}
	_document->unselect_all();
	_camera->center();
}

void ModCad::display(float left, float top, float width, float height)
{

	Module::display(left, top, width, height);

	_render->set_size(width, height);
	_camera->set_size(width, height);
	
	if ( !_initialized )
	{ 
		_initialized = true;
		_camera->center();
	}

	updateAxe();
	updateGrid();
}

/// ***************************************************************************************************
///
///                                         MOUSE
///
/// ***************************************************************************************************
void ModCad::left_button_down() {
	_left_down = true;
	_camera->mouseLeftDown();
	_left_mouse_pos = _last_mouse_pos;
	_left_mouse_mag = _magnet_point;

	if (_cad_creation == GraphicType::None && _shape_hovered != nullptr)
	{
		if (_shape_hovered->type() == GraphicType::Point)
			_left_mouse_mag = ((Point*)_shape_hovered)->p1();
		else {
			auto a = _shape_hovered->anchor(_magnet_point);
			if (a != nullptr)
				_left_mouse_mag = a->point();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// if we are constructing a new shape
	if (_cad_creation != GraphicType::None && _cad_creation != GraphicType::CadSymmetry && _cad_creation != GraphicType::CadRotation)
	{
		if (_construction_shape != nullptr)	// if true then new shape is fully constructed
		{
			if (_construction_shape->point(_magnet_point))
			{
				construct_shape();
				// depending on shape type, we may start another one of same type
				if (_construction_shape != nullptr && _construction_shape->type() == GraphicType::Polyline && ((Polyline*)_construction_shape)->mode() != PolylineMode::Bezier)
					_construction_shape->point(_magnet_point);
			}
		}
		else if (_cad_creation == GraphicType::CadOrigin)
			cad_origin();
		else if (_cad_creation == GraphicType::CamMoveTo)
			cam_moveto();
		else if (_cad_creation == GraphicType::CamDrill)
			cam_drill();
	}
	else
	{
		// if controle and shift keys are not pressed, we unselect everything
		if (!_ctrl && !_shift)
			_document->unselect_all();

		// if the mouse is over a shape, we select it if shift key is not pressed, otherwise we unselect it
		if (_shape_hovered != nullptr)
		{
			// if mouse is over an anchor, we do not change
			auto a = _shape_hovered->anchor(_magnet_point);
			if (a == nullptr)
			{
				if (_shift)
					_document->unselect(_shape_hovered);
				else
					_document->select(_shape_hovered);
			}
			
			_shape_hovered->modification(a);

			if (a != nullptr && _shape_hovered->type() == GraphicType::Text && ((Text*)_shape_hovered)->reference() != -1)
				_disable_magnet = true;
		}
	}
}

void ModCad::left_button_up() {
	_left_down = false;
	_start_moving = false;
	_camera->mouseLeftUp();
	_mode_selection = false;
	_magnet_shapes.clear();
	_modification_started = false;

	if (_cad_creation == GraphicType::None || _cad_creation == GraphicType::CadSymmetry)
	{
		std::vector<Shape*> selection;
		// cursor is not over a shape, so we are looking for every shapes inside the region
		// described from _left_mous_pos to _last_mouse_pos
		if (_shape_hovered == nullptr)
		{
			auto left_down = glm::vec2(glm::min(_left_mouse_pos.x, _last_mouse_pos.x), glm::max(_left_mouse_pos.y, _last_mouse_pos.y));
			auto left_up = glm::vec2(glm::max(_left_mouse_pos.x, _last_mouse_pos.x), glm::min(_left_mouse_pos.y, _last_mouse_pos.y));
			
			for (Layer* layer : _document->layers())
			{
				for (Shape* shape : layer->shapes())
				{
					if (geometry::rectangle_contains(left_down, left_up, shape->topLeft(), shape->bottomRight()))
						selection.push_back(shape);
				}
			}
		}
		else // otherwise, we are dealing only with the shape under mouse
		{
			selection.push_back(_shape_hovered);
			_shape_hovered->modification(nullptr);
		}

		if (!_ctrl && !_shift && !(_document->selected().size() > 0 && _document->selected()[0]->toolpath()))
			_document->unselect_all();

		for (Shape* s : selection)
		{
			if (_shift)
				_document->unselect(s);
			else
				_document->select(s);
		}
	}

	_magnet_point = _magnet_point_v = _magnet_point_h = _last_mouse_pos;
	_disable_magnet = false;
}

void ModCad::mouseMouve(double x_pos, double y_pos)
{
	_camera->mouseMove((float)x_pos - _left, (float)y_pos - _top);
	auto previous_mouse_pos = _last_mouse_pos;

	// hold the last mouse position with camera scale applied
	_last_mouse_pos = _camera->getPosition(_camera->xpos(), _camera->ypos());

	// screen clipping coordinates
	auto top_left = _camera->getPosition(0.0f, 0.0f);
	auto bottom_right = _camera->getPosition((float)_render->width(), (float)_render->height());

	if (_right_down)
	{
		updateAxe();
		updateGrid();
		updateMagnet();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Magnet computation
	// tested points are magnets() points returned from shapes stored in _magnet_shapes list
	// _magnet_shapes list is filled when the mouse passes over a shape when
	// we modify, move or create a shape - see _left_down section in this method
	// _magnet_point_v will store the vertical coordinates
	// _magnet_point_h will store the horizontal coordinates
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_magnet_point = _magnet_point_v = _magnet_point_h = _last_mouse_pos;
	if (!_disable_magnet && (_left_down || (_cad_creation != GraphicType::None && _cad_creation != GraphicType::CadSymmetry)))
	{
		if (!_mode_selection)
		{
			///
			auto itl = _document->layers().rbegin();
			bool end_loop = false;
			while (!end_loop && itl != _document->layers().rend())
			{
				auto its = (*itl)->shapes().rbegin();
				while (!end_loop && its != (*itl)->shapes().rend())
				{
					if ((*its)->bounds().contains(_last_mouse_pos))
					{
						if ((*its)->is_over(_last_mouse_pos))
						{
							if (std::find(_magnet_shapes.begin(), _magnet_shapes.end(), *its) == _magnet_shapes.end())
								_magnet_shapes.push_back(*its);
							end_loop = true;
						}
					}
					++its;
				}
				++itl;
			}
		}

		std::vector<glm::vec2> magnets;

		if (config.use_grid)
		{
			float inc_grid = config.inc_grid;
			if (inc_grid != 0)
			{
				auto cx = ((int)(_last_mouse_pos.x / inc_grid)) * inc_grid;
				if (cx > _last_mouse_pos.x)
					cx -= inc_grid;
				auto cy = ((int)(_last_mouse_pos.y / inc_grid)) * inc_grid;
				if (cy > _last_mouse_pos.y)
					cy -= inc_grid;
				// (cx;cy) represents the ref_point on the grid which is on the bottom_left corner of _last_mouse_pos, so we need to include the four points around
				magnets.push_back(glm::vec2(cx, cy));
				magnets.push_back(glm::vec2(cx + inc_grid, cy));
				magnets.push_back(glm::vec2(cx + inc_grid, cy + inc_grid));
				magnets.push_back(glm::vec2(cx, cy + inc_grid));
			}
		}
		else
		{
			magnets.push_back(glm::vec2(0.0f, 0.0f)); // origin
			for (Shape* ms : _magnet_shapes)
			{
				auto p = ms->magnets();
				magnets.insert(magnets.end(), p.begin(), p.end());
			}

			if (_construction_shape != nullptr && _construction_shape->magnets().size() > 0)
			{
				auto points = _construction_shape->magnets();
				magnets.insert(magnets.end(), points.begin(), points.end());
			}
		}

		// now that we have candidates, we test
		float precision = 2 * PRECISION / _camera->scale(), d_v, d_h, v, h, p_v = _last_mouse_pos.y, p_h = _last_mouse_pos.x;
		v = h = d_v = d_h = std::numeric_limits<float>::max();
		
		for (auto m : magnets)
		{
			auto dh = glm::abs(m.x - _last_mouse_pos.x);
			auto dv = glm::abs(m.y - _last_mouse_pos.y);

			if (dh < precision) // if the distance between _last_mouse_pos.x and m.x is less than precision, we deal with x
			{
				if (dh < d_h) // if the distance is less than previous detection, we use this magnet ref_point
				{
					d_h = dh;
					p_h = m.x;
					_magnet_point_h = m;
				}
				if (dv < v)	// if the y distance is less than previous y position, we use this y coordinate
				{
					v = dv;
					_magnet_point_h.y = m.y;
				}
			}
			if (dv < precision) // same for vertical magnet
			{
				if (dv < d_v)
				{
					d_v = dv;
					p_v = m.y;
					_magnet_point_v = m;
				}
				if (dh < h)
				{
					h = dh;
					_magnet_point_v.x = m.x;
				}
			}
		}

		_magnet_point = glm::vec2(p_h, p_v);
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// End of magnet part
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (_cad_creation != GraphicType::None && _cad_creation != GraphicType::CadSymmetry && _cad_creation != GraphicType::CadRotation)
	{
		// we can be in CONSTRUCTION
		if (_construction_shape != nullptr)
			_construction_shape->move(_magnet_point);

		updateMagnet();
	}
	// left button is up, so we are detecting if the mouse is over a shape
	else if (!_left_down)
	{
		// we check if already over selected shape
		if (_document->selected().size() == 1 && _document->selected()[0]->shape())
		{
			auto shape = (Shape*)_document->selected()[0];
			if (shape->is_over(_last_mouse_pos) || shape->anchor(_last_mouse_pos) != nullptr)
			{
				if (_shape_hovered != nullptr && _shape_hovered != shape)
					_shape_hovered->over(false);
				_shape_hovered = shape;
			}
		}

		// we check if mouse is already over a shape
		if (_shape_hovered != nullptr)
		{
			if (!_shape_hovered->is_over(_last_mouse_pos))
			{
				_shape_hovered->over(false);
				_shape_hovered = nullptr;
			}
			else
				_shape_hovered->anchor(_last_mouse_pos);
		}

		// if not, we look for the top shape that might be hovered by mouse cursor
		auto itl = _document->layers().rbegin();
		while (_shape_hovered == nullptr && itl != _document->layers().rend())
		{
			auto its = (*itl)->shapes().rbegin();
			while (_shape_hovered == nullptr && its != (*itl)->shapes().rend())
			{
				if ((*its)->bounds().contains(_last_mouse_pos))
				{
					if ((*its)->is_over(_last_mouse_pos))
					{
						_shape_hovered = *its;
					}
				}
				++its;
			}
			++itl;
		}

		if (_shape_hovered != nullptr)
			_shape_hovered->over(true);
	}
	// left button is down, we are in the following mode : selection, multiple selection, modification, move, construction
	else
	{
		if (_shape_hovered != nullptr)
		{
			// _start_moving is used to avoid micro-displacements
			if (!_start_moving)
			{
				_start_moving = geometry::distance(_left_mouse_pos, _last_mouse_pos) > 1;
				if (_start_moving)
					previous_mouse_pos = _left_mouse_pos; // as the shapes/anchors are not yet moved, we set the start coordinates to _left_mouse_pos to have the correct coordinates diff
			}
			if (_start_moving)
			{
				std::string modify;
				auto a = _shape_hovered->modification();
				// mouse over an anchor so the shape is in modification mode
				// otherwise the shape is in moving mode
				if (a != nullptr)
				{
					if (!_modification_started)	modify = _shape_hovered->write(); // for history modification record, stores original value

					_shape_hovered->move(_magnet_point);	// processing shape modification from anchor
				}
				else
				{
					auto diff = _last_mouse_pos - previous_mouse_pos;
					auto m = glm::translate(glm::vec3(diff.x, diff.y, 1.0f));
					for (Graphic* g : _document->selected())
					{
						if (g->shape())
						{
							if (!_modification_started)
								modify += ((Shape*)g)->write() + "\n"; // for history modification record, stores original value

							if (((Shape*)g)->type() == GraphicType::Text && ((Text*)g)->reference() != -1)
								((Shape*)g)->move(_last_mouse_pos); // processing shape move
							else
								((Shape*)g)->transform(m); // processing shape move
						}
					}
				}
				updateMagnet();

				if (!_modification_started)
				{
					_modification_started = true;
					History::undo(HistoryActionType::Modify, modify);
				}
			}
		}
		else // selection mode
			updateSelection();
	}

	updateCursor();
}

// called when left button is down
void ModCad::right_button_down() {
	_right_down = true;
	_camera->mouseRightDown();
	_mode_selection = false;
	if (_cad_creation != GraphicType::None && _construction_shape != nullptr)
	{
		delete _construction_shape;
		_construction_shape = nullptr;
		_cad_creation = GraphicType::None;
		reset_display_boxes();
		unselect();
	}
}

// called when left button is down
void ModCad::right_button_up() {
	_right_down = false;
	_cad_creation = GraphicType::None;
	reset_display_boxes();
	_camera->mouseRightUp();
	updateCursor();
	_magnet_shapes.clear();
}

void ModCad::scroll(double xdelta, double ydelta)
{
	_camera->mouseWheel((float)xdelta, (float)ydelta);

	// we set the new pixel dependent precision factor for mouse detection
	Shape::precision = PRECISION / _camera->scale();

	updateAxe();
	updateCursor();
	updateGrid();

	for (auto l : _document->layers())
		l->scaled();

	for (auto g : _document->groups())
		g->scaled();
}

//***************************************************************************************************
//
//                                         KEYBOARD
//
//***************************************************************************************************


void ModCad::key_pressed(int key, int scancode, int action, int mods)
{
	auto k = glfwGetKeyName(key, scancode);

	if (action == GLFW_PRESS)
	{
		// DELETE Key pressed, deleting selected shapes
		if (key == GLFW_KEY_DELETE && _document->selected().size() > 0)
		{
			remove();
		}
		else if (key == GLFW_KEY_SPACE ) // Space bar pressed, used for ending polyline construction
		{
			if (_cad_creation != GraphicType::None)
				construct_shape();

			config.field_space_toggled = !config.field_space_toggled;
		}
		else if (k != nullptr && *k == 'a' && _ctrl)
		{
			select_all();
		}
		else if (k != nullptr && *k == 'z' && _ctrl)
		{
			undo();
		}
		else if (k != nullptr && *k == 'y' && _ctrl)
		{
			redo();
		}
		else if (k != nullptr && *k == 'x' && _ctrl)
		{
			cut();
		}
		else if (k != nullptr && *k == 'c' && _ctrl)
		{
			copy();
		}
		else if (k != nullptr && *k == 'v' && _ctrl)
		{
			paste();
		}
		// rename layer, group, shape or toolpath
		else if (key == GLFW_KEY_F1)
		{
			config.field_f1_toggled = !config.field_f1_toggled;
		}
		else if (key == GLFW_KEY_F2)
		{
			_f2 = true;
			config.field_f2_toggled = !config.field_f2_toggled;
		}
		else if (key == GLFW_KEY_F3)
		{
			config.field_f3_toggled = !config.field_f3_toggled;
		}
		else if (key == GLFW_KEY_F4)
		{
			config.field_f4_toggled = !config.field_f4_toggled;
		}
		else if (key == GLFW_KEY_F5)
		{
			config.field_f5_toggled = !config.field_f5_toggled;
		}
		else if (key == GLFW_KEY_F6)
		{
			config.field_f6_toggled = !config.field_f6_toggled;
		}
		else if (key == GLFW_KEY_F7)
		{
			config.field_f7_toggled = !config.field_f7_toggled;
		}
		else if (key == GLFW_KEY_F8)
		{
			config.field_f8_toggled = !config.field_f8_toggled;
		}
		else if (key == GLFW_KEY_F9)
		{
			config.field_f9_toggled = !config.field_f9_toggled;
		}
		// rename layer, group, shape or toolpath
		else if (key == GLFW_KEY_ESCAPE)
		{
			_f2 = false;
		}
	}

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) // CTRL and SHIFT keys for shapes' selection
			_ctrl = true;
		else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			_shift = true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
			_ctrl = false;
		else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			_shift = false;
	}
	 
}

//***************************************************************************************************
//
//                                         RENDERING/UI
//
//***************************************************************************************************



void ModCad::render_frame()
{
	// clear opengl window
	_render->clear(config.background2DColor.r, config.background2DColor.g, config.background2DColor.b, config.background2DColor.a);

	// set the projecton/modelview matrix computed by camera
	_render->use_program("vertices");
	_render->set_uniform("projection", _camera->projection());
	_render->set_uniform("modelview", _camera->modelview());
	
	// draw axes
	if (config.use_grid)
	{
		_render->set_uniform("inner_color", config.gridColor);
		_b_grid->draw(GL_POINTS);
	}
	_render->set_uniform("inner_color", config.axeColor);
	_b_axe->draw(GL_LINES);

	// draw document
	if (_document) 
	{
		_document->draw();
		_document->anchors();

		if (_construction_shape != nullptr)
			_construction_shape->draw();
	}

	// draw magnets or selection box
	if (!config.use_grid && (_magnet_point_v != _last_mouse_pos || _magnet_point_h != _last_mouse_pos))
	{
		_render->set_uniform("inner_color", config.decorationColor);
		_b_magnet->draw(GL_LINES);
	}

	if (_mode_selection)
	{
		_render->set_uniform("inner_color", config.decorationColor);
		_b_selection->draw(GL_LINES);
	}

	// draw cursor
	_render->set_uniform("inner_color", config.axeColor);
	_b_cursor->draw(GL_LINES);

}

void ModCad::render_GUI()
{
	_window->push_font("properties");

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
	else
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

	// display the window listing layers/groups
	// properties are displayed directly by layers/shapes/groups/toolpaths
	ImGui::Begin((Lang::t("PROJECT")+"###PROJECT").c_str());
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("L&G_TAB_BAR", tab_bar_flags))
	{
		ImGuiTreeNodeFlags header_flag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (ImGui::BeginTabItem(Lang::l("LAYER")))
		{
			if (set_shortcut("BTN_ADD", ImVec2(15, 15)))
			{
				add_layer();
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_REMOVE", ImVec2(15, 15)))
			{
				delete_layer();
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_UP", ImVec2(15, 15)))
			{
				if (_document->selected().size() > 0 && (_document->selected()[0]->type() == GraphicType::Layer || _document->selected()[0]->shape()))
					_document->up(_document->selected()[0]);
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_DOWN", ImVec2(15, 15)))
			{
				if (_document->selected().size() > 0 && (_document->selected()[0]->type() == GraphicType::Layer || _document->selected()[0]->shape()))
					_document->down(_document->selected()[0]);
			}

			for (Layer* l : _document->layers())
			{
				// Layer name is displayed if not in edition mode (F2 pressed)
				ImGuiTreeNodeFlags hflags = header_flag;
				std::string name = l->name();
				if (l->selected())
				{
					if (_f2) // F2 is pressed, we edit the name
						name = "##LAYER_NAME_" + l->name();
					else
						hflags |= ImGuiTreeNodeFlags_Selected; // otherwise we display node as selected
				}

				if (ImGui::TreeNodeEx(name.c_str(), hflags))
				{
					if (ImGui::IsItemClicked())
						select(l);

					// if F2 key pressed, display InputText instead
					if (_f2 && l->selected())
					{
						ImGui::SameLine();
						char input[20];
						sprintf_s(input, "%s", l->name().c_str());
						ImGui::SetKeyboardFocusHere();
						if (ImGui::InputText("##_F2_INPUT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
						{
							l->name(input);
							for (Shape* s : l->shapes())
								s->parent(input);
							_f2 = false;
						}
					}
					ImGui::TreePop(); // we close tree to keep shapes on the same left position, not really a tree but more a list showned
					
					for (Shape* s : l->shapes())
					{
						ImGuiTreeNodeFlags nflags = node_flags;
						std::string name = s->name();
						if (s->selected())
						{
							if (_f2) // name edition, else treenode name
								name = "##SHAPE_NAME_" + s->name();
							else
								nflags |= ImGuiTreeNodeFlags_Selected;
						}
						if (ImGui::TreeNodeEx(name.c_str(), nflags))
						{
							if (ImGui::IsItemClicked())
								select(s);

							// if F2 key pressed, display InputText instead
							if (_f2 && s->selected())
							{
								ImGui::SameLine();
								char input[20];
								sprintf_s(input, "%s", s->name().c_str());
								ImGui::SetKeyboardFocusHere();
								if (ImGui::InputText("##_F2_INPUT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
								{
									s->name(input);
									_f2 = false;
								}
							}
						}
					}
				}
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem(Lang::l("GROUP"), 0, _tab_flags))
		{
			if (set_shortcut("BTN_ADD", ImVec2(15,15)))
			{
				add_group();
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_REMOVE", ImVec2(15, 15)))
			{
				delete_group();
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_UP", ImVec2(15, 15)))
			{
				if (_document->selected().size() > 0 && (_document->selected()[0]->type() == GraphicType::Group || _document->selected()[0]->toolpath()))
					_document->up(_document->selected()[0]);
			}
			ImGui::SameLine();
			if (set_shortcut("BTN_DOWN", ImVec2(15, 15)))
			{
				if (_document->selected().size() > 0 && (_document->selected()[0]->type() == GraphicType::Group || _document->selected()[0]->toolpath()))
					_document->down(_document->selected()[0]);
			}

			_tab_flags = 0;
			for (Group* g : _document->groups())
			{
				// Group name is displayed if not in edition mode (F2 pressed)
				ImGuiTreeNodeFlags hflags = header_flag;
				std::string name = g->name();
				if (g->selected())
				{
					if (_f2) // F2 is pressed, we edit the name
						name = "##GROUP_NAME_" + g->name();
					else
						hflags |= ImGuiTreeNodeFlags_Selected; // otherwise we display node as selected

				}
	
				if (ImGui::TreeNodeEx(name.c_str(), hflags))
				{
					if (ImGui::IsItemClicked())
						select(g);

					// if F2 key pressed, display InputText instead
					if (_f2 && g->selected())
					{
						ImGui::SameLine();
						char input[20];
						sprintf_s(input, "%s", g->name().c_str());
						ImGui::SetKeyboardFocusHere();
						if (ImGui::InputText("##_F2_INPUT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
						{
							g->name(input);
							for (Toolpath* t : g->toolpaths())
								t->parent(input);
							_f2 = false;
						}
					}
					ImGui::TreePop(); // we close tree to keep shapes on the same left position, not really a tree but more a list showned
					
					for (Toolpath* t : g->toolpaths())
					{
						ImGuiTreeNodeFlags nflags = node_flags;
						std::string name = t->name();
						if (t->selected())
						{
							if (_f2) // name edition, else treenode name
								name = "##TOOLPATH_NAME_" + t->name();
							else
								nflags |= ImGuiTreeNodeFlags_Selected;
						}
						if (ImGui::TreeNodeEx(name.c_str(), nflags))
						{
							if (ImGui::IsItemClicked())
								select(t);

							// if F2 key pressed, display InputText instead
							if (_f2 && t->selected())
							{
								ImGui::SameLine();
								char input[20];
								sprintf_s(input, "%s", t->name().c_str());
								ImGui::SetKeyboardFocusHere();
								if (ImGui::InputText("##_F2_INPUT", input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
								{
									t->name(input);
									_f2 = false;
								}
							}
						}
					}
				}
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	if (_document->selected().size() == 0)
	{
		ImGui::Begin("###OBJECT");
		ImGui::End();
	}

	// icon shortcuts
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
	ImGui::SetNextWindowPos(ImVec2(_width - 40.0f, 60.0f));
	ImGui::SetNextWindowSize(ImVec2(38.0f, _height));
	ImGui::Begin("###SHORTCUTS", 0, window_flags);
	
	if (set_shortcut("CAD_POINT", ImVec2(20, 20)))
	{
		reset_display_boxes();
		unselect();
		if (_construction_shape != nullptr) delete _construction_shape;
		_construction_shape = new Point(_render);
		_cad_creation = GraphicType::Point;
	}
	if (set_shortcut("CAD_LINE", ImVec2(20, 20)))
	{
		reset_display_boxes();
		unselect();
		if (_construction_shape != nullptr) delete _construction_shape;
		_construction_shape = new Line(_render);
		_cad_creation = GraphicType::Line;
	}
	if (set_shortcut("CAD_CIRCLE", ImVec2(20, 20)))
	{
		reset_display_boxes();
		unselect();
		if (_construction_shape != nullptr) delete _construction_shape;
		_construction_shape = new Circle(_render);
		_cad_creation = GraphicType::Circle;
	}
	if (set_shortcut("CAD_ARC", ImVec2(20, 20)))
	{
		reset_display_boxes();
		unselect();
		//if (_construction_shape != nullptr) delete _construction_shape;
		//_construction_shape = new Arc(_render);
		//_cad_creation = GraphicType::Arc;
		if (_construction_shape != nullptr)
		{
			delete _construction_shape;
			_construction_shape = nullptr;
		}
		_cad_creation = GraphicType::None;
		_display_arc_box = true;
		_btn_pos = ImGui::GetCursorScreenPos();
	}
	if (set_shortcut("CAD_ELLIPSE", ImVec2(20, 20)))
	{
		reset_display_boxes();
		unselect();
		if (_construction_shape != nullptr) delete _construction_shape;
		_construction_shape = new Ellipse(_render);
		_cad_creation = GraphicType::Ellipse;
	}
	if (set_shortcut("CAD_POLYLINE", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_construction_shape != nullptr)
		{
			delete _construction_shape;
			_construction_shape = nullptr;
		}
		_cad_creation = GraphicType::None;
		_display_poly_box = true;
		_btn_pos = ImGui::GetCursorScreenPos();
	}
	if (set_shortcut("CAD_TEXT", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_construction_shape != nullptr) delete _construction_shape;
		_construction_shape = new Text(_render);
		_cad_creation = GraphicType::Text;
	}

	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();

	if (set_shortcut("CAD_ORIGIN", ImVec2(20, 20)))
	{
		reset_display_boxes();
		_cad_creation = GraphicType::CadOrigin;
	}

	if (set_shortcut("CAD_OFFSET", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_document->selected().size() == 0)
		{
			_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
		}
		else
		{
			_cad_creation = GraphicType::CadOffset;
			_message_box.input(Lang::t("CAD_OFFSET"), Lang::t("MESSAGE_BOX_INPUT_OFFSET"));
		}
	}

	if (set_shortcut("CAD_SYMMETRY", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_document->selected().size() == 0)
		{
			_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
		}
		else
		{
			store_operation();
			_cad_creation = GraphicType::CadSymmetry;
			_message_box.display(Lang::t("CAD_SYMMETRY"), Lang::t("MESSAGE_BOX_SELECT_AXE"));
		}
	}

	if (set_shortcut("CAD_SCALE", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_document->selected().size() == 0)
		{
			_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
		}
		else
		{
			_display_poly_box = _display_arc_box = false;
			_cad_creation = GraphicType::CadScale;
			_message_box.input(Lang::t("CAD_SCALE"), Lang::t("MESSAGE_BOX_INPUT_SCALE"));
		}
	}

	if (set_shortcut("CAD_ROTATION", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_document->selected().size() == 0)
		{
			_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
		}
		else
		{
			store_operation();
			_cad_creation = GraphicType::CadRotation;
			_message_box.input(Lang::t("CAD_ROTATION"), Lang::t("MESSAGE_BOX_SELECT_ROTATION"));
		}
	}


	if (set_shortcut("CAD_CHAMFER", ImVec2(20, 20)))
	{
		reset_display_boxes();
		if (_document->selected().size() == 0)
		{
			_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
		}
		else
		{
			_btn_pos = ImGui::GetCursorScreenPos();
			_display_chamfer_box = true;
		}
	}

	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();

	if (set_shortcut("CAM_MOVETO", ImVec2(20, 20)))
	{
		reset_display_boxes();
		_cad_creation = GraphicType::CamMoveTo;
	}
	if (set_shortcut("CAM_DRILL", ImVec2(20, 20)))
	{
		reset_display_boxes();
		_cad_creation = GraphicType::CamDrill;
	}
	if (set_shortcut("CAM_FOLLOW", ImVec2(20, 20)))
	{
		reset_display_boxes();
		cam_follow();
	}
	if (set_shortcut("CAM_OFFSET", ImVec2(20, 20)))
	{
		reset_display_boxes();
		_btn_pos = ImGui::GetCursorScreenPos();
		_display_offset_box = true;
	}
	if (set_shortcut("CAM_POCKET", ImVec2(20, 20)))
	{
		reset_display_boxes();
		_btn_pos = ImGui::GetCursorScreenPos();
		_display_pocket_box = true;
	}
	ImGui::End();


	// display the context menu for arc creation
	if (_display_arc_box)
	{
		ImGui::SetNextWindowPos(ImVec2(_btn_pos.x - _last_size.x - 5.0f, _btn_pos.y));
		ImGui::Begin("##ARC_TYPE_SELECTION", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		_last_size = ImGui::GetWindowSize();

		const char* items[]{ Lang::l("CAD_ARC_CENTER"), Lang::l("CAD_ARC_3P") };
		int selected_item = -1;
		if (_cad_creation == GraphicType::None && ImGui::ListBox("##ARC_MODE", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			unselect();
			if (_construction_shape != nullptr) delete _construction_shape;
			_construction_shape = new Arc(_render);
			_cad_creation = GraphicType::Arc;
			((Arc*)_construction_shape)->mode((ArcMode)selected_item);
			_display_arc_box = false;
		}
		ImGui::End();
	}


	// display the context menu for polyline type selection
	if (_display_poly_box)
	{
		ImGui::SetNextWindowPos(ImVec2(_btn_pos.x - _last_size.x - 5.0f, _btn_pos.y));
		ImGui::Begin("##POLY_TYPE_SELECTION", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		_last_size = ImGui::GetWindowSize();

		const char* items[]{ "Poly","Bezier"/*,"BSpline"*/, "Natural Cubic", "Lagrange" };
		int selected_item = -1;
		if (_cad_creation == GraphicType::None && ImGui::ListBox("##POLY_MODE", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			unselect();
			if (_construction_shape != nullptr) delete _construction_shape;
			_construction_shape = new Polyline(_render);
			_cad_creation = GraphicType::Polyline;
			((Polyline*)_construction_shape)->mode((PolylineMode)selected_item);
		}
		ImGui::End();

		// display the buttons 'Add' and 'Close' for polyline creation
		if (_construction_shape != nullptr && ((Polyline*)_construction_shape)->mode() != PolylineMode::Bezier)
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 50.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::Begin("##APPROVING", 0, wf);
			if (ImGui::Button(Lang::l("ADD")))
			{
				if (((Polyline*)_construction_shape)->points().size() > 2)
				{
					construct_shape();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Lang::l("CLOSE")))
			{
				((Polyline*)_construction_shape)->points().clear();
				delete _construction_shape;
				_construction_shape = nullptr;
				_cad_creation = GraphicType::None;
				_display_poly_box = false;
			}
			ImGui::End();
		}
	}

	if (_display_chamfer_box)
	{
		ImGui::SetNextWindowPos(ImVec2(_btn_pos.x - _last_size.x - 5.0f, _btn_pos.y));
		ImGui::Begin("##CHAMFER_BOX_SELECTION", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		_last_size = ImGui::GetWindowSize();

		const char* items[]{ Lang::l("CAD_CHAMFER"), Lang::l("CAD_ROUNDED_CHAMFER") };
		int selected_item = -1;
		if (ImGui::ListBox("##CHAMFER_LIST", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			//if (selected_item == 0)
			//	_cad_creation = GraphicType::CadSChamfer;
			//if (selected_item == 1)
			//	_cad_creation = GraphicType::CadRChamfer;
		}
	
		char input[20];
		sprintf_s(input, "%.3f", config.chamfer_radius);
		if (ImGui::InputText(Lang::l("RADIUS"), input, 20 * sizeof(char), NULL))
		{
			try
			{
				config.chamfer_radius = std::stof(input);
			}
			catch (const std::exception& e)
			{
				Logger::log(e.what());
			}
		}

		if (selected_item != -1)
		{
			if (selected_item == 0)
				cad_chamfer(config.chamfer_radius);
			else
				cad_fillet(config.chamfer_radius);
			reset_display_boxes();
		}
		
		ImGui::End();
	}

	if (_display_offset_box)
	{
		ImGui::SetNextWindowPos(ImVec2(_btn_pos.x - _last_size.x - 5.0f, _btn_pos.y));
		ImGui::Begin("##OFFSET_BOX_SELECTION", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		_last_size = ImGui::GetWindowSize();

		const char* items[]{ Lang::l("CAM_OFFSET_INTERIOR"), Lang::l("CAM_OFFSET_EXTERIOR") };
		int selected_item = -1;
		if (ImGui::ListBox("##OFFSET_LIST", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			cam_offset(selected_item == 0);
			_display_offset_box = false;
		}
		ImGui::End();
	}

	if (_display_pocket_box)
	{
		ImGui::SetNextWindowPos(ImVec2(_btn_pos.x - _last_size.x - 5.0f, _btn_pos.y));
		ImGui::Begin("##POCKET_BOX_SELECTION", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		_last_size = ImGui::GetWindowSize();

		const char* items[]{ "Zigzag", "Offset"};
		int selected_item = -1;
		if (ImGui::ListBox("##POCKET_LIST", &selected_item, items, IM_ARRAYSIZE(items)))
		{
			cam_pocket((PocketMode)selected_item);
			_display_pocket_box = false;
		}
		ImGui::End();
	}

	if (_message_box.show() == MessageBoxStatus::Ok)
	{
		if (_cad_creation == GraphicType::CadSymmetry)
			cad_symmetry();
		else if (_cad_creation == GraphicType::CadOffset || _cad_creation == GraphicType::CadScale || _cad_creation == GraphicType::CadRotation)
		{
			float value = 0;
			try
			{
				value = std::stof(_message_box.result());
				if (value != 0)
				{
					if (_cad_creation == GraphicType::CadOffset)
						cad_offset(value);
					else if (_cad_creation == GraphicType::CadScale)
						cad_scale(value);
					else if (_cad_creation == GraphicType::CadRotation)
						cad_rotation(value);
					_cad_creation = GraphicType::None;
				}
			}
			catch (const std::exception&) {}
		}
	}

	if (_display_delete_layer)
	{
		MessageBox dlg;
		dlg.display(Lang::t("BTN_REMOVE"), Lang::t("DELETE_VALIDATION"), MessageBoxButtons::YesNo);
		auto result = dlg.show();
		if (result == MessageBoxStatus::Yes)
		{
			_display_delete_layer = false;

			auto layer = (Layer*)_document->selected()[0];

			std::string undo = "";
			// History management
			for (Shape* s : layer->shapes())
				undo += s->write() + '\n';
			History::undo(HistoryActionType::Delete, undo + layer->write());

			_document->remove_layer(layer);
		}
		else if (result == MessageBoxStatus::No)
			_display_delete_layer = false;
	}

	if (_display_delete_group)
	{
		MessageBox dlg;
		dlg.display(Lang::t("BTN_REMOVE"), Lang::t("DELETE_VALIDATION"), MessageBoxButtons::YesNo);
		auto result = dlg.show();
		if (result == MessageBoxStatus::Yes)
		{
			_display_delete_group = false;

			auto group = (Group*)_document->selected()[0];

			std::string undo = "";
			// History management
			for (Toolpath* t : group->toolpaths())
				undo += t->write() + '\n';
			History::undo(HistoryActionType::Delete, undo + group->write());

			_document->remove_group(group);
		}
		else if (result == MessageBoxStatus::No)
			_display_delete_group = false;
	}

	// display the properties of selected element
	for (Graphic* graphic : _document->selected())
	{
		graphic->ui();
	}

	_window->pop_font();

	// display bottom line info
	auto p = _camera->getPosition(_camera->xpos(), _camera->ypos());
	ImGui::SetNextWindowPos(ImVec2(0, (float)_height + ImGui::GetItemRectSize().y + 4), 0);
	ImGui::SetNextWindowSize(ImVec2((float)_width, ImGui::GetItemRectSize().y));
	ImGui::Begin("mod_cad_internal", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	ImGui::Text("(%0.3f;%0.3f), zoom %f", p.x, p.y, _camera->scale());
	ImGui::SameLine();
	if (!Logger::trace().empty()) ImGui::Text("Trace: %s", Logger::trace().c_str()); // display trace content
	ImGui::End();
}

void ModCad::render_menu()
{
	if (ImGui::BeginMenu(Lang::l("DRAWING")))
	{
		if (ImGui::MenuItem(Lang::l("DRAWING_ADD_LAYER")))
		{
			add_layer();
		}
		if (ImGui::MenuItem(Lang::l("DRAWING_DELETE_LAYER")))
		{
			delete_layer();
		}
		if (ImGui::MenuItem(Lang::l("DRAWING_RENAME")))
		{
			key_pressed(GLFW_KEY_F2, 0, GLFW_PRESS, 0);
		}
		ImGui::Separator();
		if (ImGui::MenuItem(Lang::l("DRAWING_REVERSE")))
		{
			if (_document->selected().size() > 0)
			{
				reverse();
			}
		}
		if (ImGui::MenuItem(_document->unit() == Unit::Millimeter ? Lang::l("DRAWING_CONVERT_TO_INCH") : Lang::l("DRAWING_CONVERT_TO_MM")))
		{
			convert_unit();
		}
		if (_document->selected().size() > 0)
		{
			if (ImGui::BeginMenu(Lang::l("DRAWING_MOVE_TO")))
			{
				History::begin_undo_record();
				for (Layer* l : _document->layers())
				{
					if (ImGui::MenuItem(l->name().c_str()))
					{
						for (Graphic* g : _document->selected())
						{
							if (g->shape())
							{
								Shape* s = (Shape*)g;
								if (s->parent() != l->name())
								{
									History::undo(HistoryActionType::Delete, s->write());
									_document->layer(s->parent())->remove(s);
									l->add(s);
									History::undo(HistoryActionType::Add, s->write());
								}
							}
						}
					}
					History::end_undo_record();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(Lang::l("DRAWING_BOOLEAN")))
			{
				if (ImGui::MenuItem(Lang::l("DRAWING_UNION")))
				{
					cad_weiler(CAD_UNION);
				}
				if (ImGui::MenuItem(Lang::l("DRAWING_SUBSTRACT")))
				{
					cad_weiler(CAD_SUBSTRACT);
				}
				if (ImGui::MenuItem(Lang::l("DRAWING_INTERSECT")))
				{
					cad_weiler(CAD_INTERSECT);
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem(Lang::l("DRAWING_CUT_AT_INTERSECTIONS")))
			{
				cut_at_intersections();
			}
			for (Graphic* g : _document->selected())
			{
				if (g->type() == GraphicType::Text)
				{
					if (((Text*)g)->reference() == -1)
					{
						if (_document->selected().size() > 1 && ImGui::MenuItem(Lang::l("DRAWING_SET_REFERENCE_TO_TEXT")))
						{
							set_reference_to_text();
							((Text*)g)->compute();
						}
					}
					else if (ImGui::MenuItem(Lang::l("DRAWING_REMOVE_REFERENCE_TO_TEXT")))
					{
						((Text*)g)->reference(-1);
						((Text*)g)->compute();
					}
					if (ImGui::MenuItem(Lang::l("DRAWING_CONVERT_TO_SPLINE")))
					{
						std::vector<Shape*> splines;
						auto coordinates = ((Text*)g)->coordinates();
						for (std::vector<glm::vec2> points : coordinates)
						{
							Spline* s = cad::to_spline(points, _render);
							if (s->size() > 1)
							{
								splines.push_back(s);
							}
						}
						if (splines.size() > 0)
						{
							History::begin_undo_record();
							_document->layer(((Text*)g)->parent())->remove((Text*)g);
							for (Shape* s : splines)
							{
								_document->current_layer()->add(s);
								History::undo(HistoryActionType::Add, s->write());
							}
							History::end_undo_record();
							select(splines);
						}
					}
					break;
				}
				//else if (g->shape())
				//{
				//	if (ImGui::MenuItem(Lang::l("DRAWING_CONNECT_SHAPES")))
				//	{
				//		connect_shapes();
				//	}
				//	if (ImGui::MenuItem(Lang::l("DRAWING_LOOK_FOR_SIBLINGS")))
				//	{
				//		auto shapes = look_for_siblings((Shape*)_document->selected()[0]);
				//		select(shapes);
				//		info(shapes);
				//	}
				//	break;
				//}
				/*else if (g->shape())
				{
					if (ImGui::MenuItem(Lang::l("DRAWING_MAKE_CW")))
					{
						select(look_for_siblings((Shape*)_document->selected()[0]));
					}
					break;
				}*/
			}

			if (ImGui::MenuItem(Lang::l("DRAWING_DEBUG_OFFSET")))
			{
				auto shapes = look_for_siblings((Shape*)_document->selected()[0]);
				std::vector<Shape*> splines;

				float o = -6.4f;
				if (shapes[0]->type() == GraphicType::Text)
				{
					auto coordinates = ((Text*)shapes[0])->coordinates();
					int count = 0;
					for (std::vector<glm::vec2> points : coordinates)
					{
						Spline* s = cad::to_spline(points, _render);
						if (s->size() > 1 && count == 3)
						{
							s->offset(o);
							splines.push_back(s);
						}
						count++;
					}
				}
				else
				{
					Spline* s = cad::to_spline(shapes, _render);

					if (s != nullptr)
					{
						s->offset(o);
						splines.push_back(s);
					}
				}
				if (splines.size() > 0)
				{
					History::begin_undo_record();
					remove(shapes);
					for (Shape* s : splines)
					{
						_document->current_layer()->add(s);
						History::undo(HistoryActionType::Add, s->write());
					}
					History::end_undo_record();
					select(splines);
				}
			}
			if (ImGui::MenuItem(Lang::l("DRAWING_DEBUG_CONVERT_TOOLPATH_TO_SPLINE")))
			{
				if ((Shape*)_document->selected()[0]->toolpath())
				{
					Toolpath* t = (Toolpath*)(Shape*)_document->selected()[0];
					std::vector<Shape*> splines;
					History::begin_undo_record();
					for (Curve& c : t->coordinates())
					{
						Spline* s = new Spline(_render);
						s->add(c);
						s->done(true);
						s->compute();
						_document->current_layer()->add(s);
						splines.push_back(s);
						History::undo(HistoryActionType::Add, s->write());
					}
					History::end_undo_record();
					_document->current_group()->remove(t);
					select(splines);
				}
			}
			//if (ImGui::MenuItem(Lang::l("DRAWING_CONVERT_TO_SPLINE")))
			//{
			//	auto shapes = look_for_siblings((Shape*)_document->selected()[0]);
			//	std::vector<Shape*> splines;

			//	if (shapes[0]->type() == GraphicType::Text)
			//	{
			//		auto coordinates = ((Text*)shapes[0])->coordinates();
			//		for (std::vector<glm::vec2> points : coordinates)
			//		{
			//			Spline* s = cad::to_spline(points, _render);
			//			if (s->size() > 1)
			//				splines.push_back(s);
			//		}
			//	}
			//	else
			//	{
			//		Spline* s = cad::to_spline(shapes, _render);

			//		if (s != nullptr)
			//		{
			//			splines.push_back(s);
			//		}
			//	}

			/*	if (splines.size() > 0)
				{
					History::begin_undo_record();
					remove(shapes);
					for (Shape* s : splines)
					{
						_document->current_layer()->add(s);
						History::undo(HistoryActionType::Add, s->write());
					}
					History::end_undo_record();
					select(splines);*/
			//	}
			//}
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu(Lang::l("WORKSHOP")))
	{
		if (ImGui::MenuItem(Lang::l("WORKSHOP_ADD_GROUP")))
		{
			add_group();
		}
		if (ImGui::MenuItem(Lang::l("WORKSHOP_DELETE_GROUP")))
		{
			delete_group();
		}
		if (ImGui::MenuItem(Lang::l("WORKSHOP_RENAME")))
		{
			key_pressed(GLFW_KEY_F2, 0, GLFW_PRESS, 0);
		}
		if (ImGui::MenuItem(Lang::l("WORKSHOP_DELETE_TOOLPATH")))
		{
			if (_document->selected().size() == 1 && _document->selected()[0]->toolpath())
			{
				Toolpath* t = (Toolpath*)_document->selected()[0];
				_document->group(t->parent())->remove(t);
			}
		}
		ImGui::EndMenu();
	}
}

void ModCad::updateAxe()
{
	// change axe line following scale factor
	float w = (config.small_axe ? 50 : 2 * (float)_width) / _camera->scale();
	float h = (config.small_axe ? 50 : 2 * (float)_height) / _camera->scale();
	std::vector<glm::vec3> axe {
		glm::vec3(w, 0.0f, 0.0f),
		glm::vec3(-w, 0.0f, 0.0f),
		glm::vec3(0.0f, -h, 0.0f),
		glm::vec3(0.0f, h, 0.0f) };
	_b_axe->update(axe);
}

void ModCad::updateCursor()
{
	// change axe line following scale factor
	float scale = _camera->scale();
	std::vector<glm::vec3> cursor;
	if (_cad_creation != GraphicType::None)
	{
		float offset = 5.0f;
		cursor.push_back(glm::vec3((float)_magnet_point.x - offset / scale, (float)_magnet_point.y, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x + offset / scale, (float)_magnet_point.y, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x, (float)_magnet_point.y - offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x, (float)_magnet_point.y + offset / scale, 0.0f));
	}
	else
	{
		float offset = 3.0f;
		cursor.push_back(glm::vec3((float)_magnet_point.x - offset / scale, (float)_magnet_point.y - offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x - offset / scale, (float)_magnet_point.y + offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x - offset / scale, (float)_magnet_point.y + offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x + offset / scale, (float)_magnet_point.y + offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x + offset / scale, (float)_magnet_point.y + offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x + offset / scale, (float)_magnet_point.y - offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x + offset / scale, (float)_magnet_point.y - offset / scale, 0.0f));
		cursor.push_back(glm::vec3((float)_magnet_point.x - offset / scale, (float)_magnet_point.y - offset / scale, 0.0f));
	}
	if (_b_cursor->size() != cursor.size())
	{
		delete _b_cursor;
		_b_cursor = _render->create_buffer(cursor);
	}
	else
		_b_cursor->update(cursor);
}

void ModCad::updateGrid()
{
	glm::vec2 top_left = _camera->getPosition(0,0);
	glm::vec2 bottom_right = _camera->getPosition((float)_width, (float)_height);

	if (config.use_grid)
	{
		float inc_grid = config.inc_grid;

		std::vector<glm::vec3> vertices;

		// start from (0,0) coordinates x+ y+
		for (float x = 0; x < bottom_right.x; x += inc_grid)
			for (float y = 0; y < top_left.y; y += inc_grid)
				vertices.push_back(glm::vec3(x, y, 0.0f));
		// x+ y-
		for (float x = 0; x < bottom_right.x; x += inc_grid)
			for (float y = -inc_grid; y > bottom_right.y; y -= inc_grid)
				vertices.push_back(glm::vec3(x, y, 0.0f));
		// x- y-
		for (float x = -inc_grid; x > top_left.x; x -= inc_grid)
			for (float y = -inc_grid; y > bottom_right.y; y -= inc_grid)
				vertices.push_back(glm::vec3(x, y, 0.0f));
		// x- y+
		for (float x = -inc_grid; x > top_left.x; x -= inc_grid)
			for (float y = 0; y < top_left.y; y += inc_grid)
				vertices.push_back(glm::vec3(x, y, 0.0f));

		if (vertices.size() != _b_grid->size())
		{
			delete _b_grid;
			_b_grid = _render->create_buffer(vertices);
		}
		else
			_b_grid->update(vertices);
	}
}

void ModCad::updateSelection()
{
	_mode_selection = true;

	std::vector<glm::vec3> vertices;
	
	vertices.insert(vertices.end(), {
		glm::vec3(_left_mouse_pos.x, _left_mouse_pos.y, 0.0f),
		glm::vec3(_last_mouse_pos.x, _left_mouse_pos.y, 0.0f),
		glm::vec3(_last_mouse_pos.x, _left_mouse_pos.y, 0.0f),
		glm::vec3(_last_mouse_pos.x, _last_mouse_pos.y, 0.0f),
		glm::vec3(_last_mouse_pos.x, _last_mouse_pos.y, 0.0f),
		glm::vec3(_left_mouse_pos.x, _last_mouse_pos.y, 0.0f),
		glm::vec3(_left_mouse_pos.x, _last_mouse_pos.y, 0.0f),
		glm::vec3(_left_mouse_pos.x, _left_mouse_pos.y, 0.0f)
			});
	_b_selection->flush(vertices);
}

void ModCad::updateMagnet()
{
	std::vector<glm::vec3> vertices;

	auto precision = PRECISION / _camera->scale();

	if (_magnet_point_h != _last_mouse_pos)
	{
		float start = glm::min(_magnet_point_h.y, _magnet_point.y);
		float end = glm::max(_magnet_point_h.y, _magnet_point.y);
		for (float y = start; y < end; y += 2 * precision)
		{	// makes simple dashed line
			vertices.push_back(glm::vec3(_magnet_point_h.x, y, 0.0f));
			vertices.push_back(glm::vec3(_magnet_point_h.x, glm::min(y + precision, end), 0.0f));
		}
		vertices.push_back(glm::vec3(_magnet_point_h.x - ANCHOR_SIZE / _camera->scale(), _magnet_point_h.y - ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_h.x + ANCHOR_SIZE / _camera->scale(), _magnet_point_h.y + ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_h.x - ANCHOR_SIZE / _camera->scale(), _magnet_point_h.y + ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_h.x + ANCHOR_SIZE / _camera->scale(), _magnet_point_h.y - ANCHOR_SIZE / _camera->scale(), 0.0f));
	}

	if (_magnet_point_v != _last_mouse_pos)
	{
		float start = glm::min(_magnet_point_v.x, _magnet_point.x);
		float end = glm::max(_magnet_point_v.x, _magnet_point.x);
		for (float x = start; x < end; x += 2 * precision)
		{	// makes simple dashed line
			vertices.push_back(glm::vec3(x, _magnet_point_v.y, 0.0f));
			vertices.push_back(glm::vec3(glm::min(x + precision, end), _magnet_point_v.y, 0.0f));
		}
		vertices.push_back(glm::vec3(_magnet_point_v.x - ANCHOR_SIZE / _camera->scale(), _magnet_point_v.y - ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_v.x + ANCHOR_SIZE / _camera->scale(), _magnet_point_v.y + ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_v.x - ANCHOR_SIZE / _camera->scale(), _magnet_point_v.y + ANCHOR_SIZE / _camera->scale(), 0.0f));
		vertices.push_back(glm::vec3(_magnet_point_v.x + ANCHOR_SIZE / _camera->scale(), _magnet_point_v.y - ANCHOR_SIZE / _camera->scale(), 0.0f));
	}

	if (vertices.size() > 0)
		_b_magnet->flush(vertices);
}

void ModCad::update()
{
	updateAxe();
	updateGrid();
	for (auto l : _document->layers())
		l->scaled();
	for (auto g : _document->groups())
		g->scaled();
}


bool ModCad::set_shortcut(std::string code, ImVec2 size)
{
	Texture* t = _textures[code];
	if (t != NULL)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 1 });
		bool click = ImGui::ImageButton(code.c_str(), t->id(), size, ImVec2(0, 0), ImVec2(1.0f, 1.0f), ImVec4(0,0,0,1));
		ImGui::PopStyleVar();
		if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1)
			ImGui::SetTooltip(Lang::l(code));
		return click;
	}
	return false;
}

//***************************************************************************************************
//
//                                         EDITION
//
//***************************************************************************************************


void ModCad::select_all()
{
	_document->selected().clear();
	for (Layer* l : _document->layers())
	{
		for (Shape* s : l->shapes())
		{
			s->selected(true);
			_document->selected().push_back(s);
		}
	}
}

void ModCad::unselect_all()
{
	_document->selected().clear();
	for (Layer* l : _document->layers())
	{
		for (Shape* s : l->shapes())
		{
			s->selected(false);
			_document->selected().push_back(s);
		}
	}
}

void ModCad::reverse_selection()
{
	_document->selected().clear();
	for (Layer* l : _document->layers())
	{
		for (Shape* s : l->shapes())
		{
			if (!s->selected())
			{
				s->selected(true);
				_document->selected().push_back(s);
			}
			else
				s->selected(false);
		}
	}
}

void ModCad::cut()
{
	copy();

	std::vector<Graphic*> graphics(_document->selected());

	std::string clip = "";


	for (Graphic* g : graphics)
	{
		clip += g->write() + '\n';
		g->selected(false);
	}
	unselect();

	Clipboard::text(clip);

	History::undo(HistoryActionType::Delete, clip);

	for (Graphic* g : graphics)
	{
		if (g->shape())
			_document->layer(((Shape*)g)->parent())->remove((Shape*)g);
	}
}

void ModCad::copy()
{
	std::string clip = "";

	for (Graphic* g : _document->selected())
		clip += g->write() + '\n';

	Clipboard::text(clip);
}

void ModCad::paste()
{
	std::string clip = Clipboard::text();
	float x = (std::numeric_limits<float>::max)(), y = (std::numeric_limits<float>::min)();

	if (!clip.empty())
	{
		auto list = stringex::split(clip, '\n');
		std::vector<Graphic*> graphics;

		for (std::string v : list)
		{
			auto data = stringex::split(v, ';');
			GraphicType type = (GraphicType)std::stoi(data[0]);
			Graphic* g = nullptr;
			switch (type)
			{
			case GraphicType::None:
				break;
			case GraphicType::Layer:
				break;
			case GraphicType::Group:
				break;
			case GraphicType::Circle:
				g = new Circle(_render);
				break;
			case GraphicType::Point:
				g = new Point(_render);
				break;
			case GraphicType::Arc:
				g = new Arc(_render);
				break;
			case GraphicType::Line:
				g = new Line(_render);
				break;
			case GraphicType::Ellipse:
				g = new Ellipse(_render);
				break;
			case GraphicType::Polyline:
				g = new Polyline(_render);
				break;
			case GraphicType::Text:
				g = new Text(_render);
				break;
			case GraphicType::CamMoveTo:
				break;
			case GraphicType::CamFollow:
				break;
			case GraphicType::CamOffset:
				break;
			case GraphicType::CamSpiral:
				break;
			default:
				break;
			}

			if (g != nullptr)
			{
				auto id = g->id();
				g->read(v);
				g->id(id);
				if (g->shape())
				{
					x = glm::min(x, ((Shape*)g)->topLeft().x);
					y = glm::max(y, ((Shape*)g)->topLeft().y);
				}

				graphics.push_back(g);
			}
		}

		// if a mouse up is recorded after selection, then we paste new element top_left corner of selection to last mouse position
		// otherwise, shapes are pasted on source shapes
		auto mat = _mode_selection ? glm::translate(glm::vec3(0, 0, 0)) : glm::translate(glm::vec3(_last_mouse_pos.x - x, _last_mouse_pos.y - y, 0));
		for (Graphic* g : graphics)
		{
			if (g->shape())
			{
				((Shape*)g)->transform(mat);
				_document->current_layer()->add((Shape*)g);
			}
		}

		select(graphics);
		
		clip = "";
		for (Graphic* g : graphics)
			clip += g->write() + '\n';
		History::undo(HistoryActionType::Add, clip);
	}

}

void ModCad::remove()
{
	std::vector graphics(_document->selected());
	std::string undo = "";
	for (Graphic* g : graphics)
	{
		if (g->shape())
		{
			Layer* l = _document->layer(((Shape*)g)->parent());
			if (l != nullptr)
			{
				g->selected(false);
				undo = g->write() + '\n';
				l->remove((Shape*)g);
			}
		}
		if (g->toolpath())
		{
			Group* t = _document->group(((Toolpath*)g)->parent());
			if (t != nullptr)
			{
				g->selected(false);
				undo = g->write() + '\n';
				t->remove((Toolpath*)g);
			}
		}
	}
	_document->selected().clear();
	History::undo(HistoryActionType::Delete, undo.substr(0, undo.size() - 1));
}

void ModCad::add_layer()
{
	auto layer = _document->layer();

	select(layer);

	// History management
	History::undo(HistoryActionType::Add, layer->write());
}

void ModCad::delete_layer()
{
	if (_document->selected().size() == 1 && _document->selected()[0]->type() == GraphicType::Layer)
	{
		_display_delete_layer = true;
	}
}

void ModCad::add_group()
{
	auto group = _document->group();
	_document->selected().clear();
	_document->selected().push_back(group);
	group->selected(true);

	// History management
	History::undo(HistoryActionType::Add, group->write());
}

void ModCad::delete_group()
{
	if (_document->selected().size() == 1 && _document->selected()[0]->type() == GraphicType::Group)
	{
		_display_delete_group = true;
	}
}

std::vector<Shape*> ModCad::get_selection()
{
	std::vector<Shape*> shapes;

	for (Graphic* g : _document->selected())
	{
		if (g->shape())
			shapes.push_back((Shape*)g);
	}

	return shapes;
}


std::vector<int> ModCad::get_references(std::vector<Graphic*> shapes)
{
	std::vector<int> result;

	for (auto s : shapes)
	{
		if (std::find(result.begin(), result.end(), s->id()) == result.end())
			result.push_back(s->id());
	}

	return result;
}

//***************************************************************************************************
//
//                                         HISTORY
//
//***************************************************************************************************



HistoryAction ModCad::process_action(HistoryAction action)
{
	// get actual state and injects it into _redo
	if (action.type == HistoryActionType::Add)
	{
		// we delete actual shapes
		auto list = stringex::split(action.value, '\n');
		for (std::string v : list)
		{
			auto data = stringex::split(v, ';');
			GraphicType type = (GraphicType)std::stoi(data[0]);
			switch (type)
			{
			case GraphicType::Layer:
				_document->remove_layer(data[2]);
				break;
			case GraphicType::Group:
				_document->remove_group(data[2]);
				break;
			case GraphicType::Circle:
			case GraphicType::Point:
			case GraphicType::Arc:
			case GraphicType::Line:
			case GraphicType::Ellipse:
			case GraphicType::Polyline:
			case GraphicType::Text:
			case GraphicType::Spline:
				_document->layer(data[2])->remove(std::stoi(data[1]));
				break;
			case GraphicType::CamMoveTo:
			case GraphicType::CamFollow:
			case GraphicType::CamOffset:
			case GraphicType::CamSpiral:
				break;
			}
		}
		return HistoryAction(HistoryActionType::Delete, action.value);
	}
	else if (action.type == HistoryActionType::Delete)
	{
		// we delete actual shapes
		auto list = stringex::split(action.value, '\n');
		Shape* s = nullptr;
		Toolpath* m = nullptr;
		Graphic* g = nullptr;
		for (std::string v : list)
		{
			auto data = stringex::split(v, ';');
			GraphicType type = (GraphicType)std::stoi(data[0]);
			switch (type)
			{
			case GraphicType::Layer:
				g = _document->layer(data[2]);
				g->read(v);
				break;
			case GraphicType::Group:
				g = _document->group(data[2]);
				g->read(v);
				break;
			case GraphicType::Circle:
				s = new Circle(_render);
				break;
			case GraphicType::Point:
				s = new Point(_render);
				break;
			case GraphicType::Arc:
				s = new Arc(_render);
				break;
			case GraphicType::Line:
				s = new Line(_render);
				break;
			case GraphicType::Ellipse:
				s = new Ellipse(_render);
				break;
			case GraphicType::Polyline:
				s = new Polyline(_render);
				break;
			case GraphicType::Text:
				s = new Text(_render);
				break;
			case GraphicType::Spline:
				s = new Spline(_render);
				break;
			case GraphicType::CamMoveTo:
			case GraphicType::CamFollow:
			case GraphicType::CamOffset:
			case GraphicType::CamSpiral:
				break;
			}

			if (s != nullptr)
			{
				s->read(v);
				_document->layer(s->parent())->add(s);
				s->compute();
				s = nullptr;
			}

			if (m != nullptr)
			{
				m->read(v);
				_document->group(m->parent())->add(m);
				m->compute();
				m = nullptr;
			}
		}
		return HistoryAction(HistoryActionType::Add, action.value);
	}
	else if (action.type == HistoryActionType::Modify)
	{
		// we delete actual shapes
		auto list = stringex::split(action.value, '\n');
		std::string output = "";
		Graphic* g = nullptr;
		for (std::string v : list)
		{
			g = nullptr;
			auto data = stringex::split(v, ';');
			GraphicType type = (GraphicType)std::stoi(data[0]);
			switch (type)
			{
			case GraphicType::Layer:
				break;
			case GraphicType::Group:
				break;
			case GraphicType::Circle:
			case GraphicType::Point:
			case GraphicType::Arc:
			case GraphicType::Line:
			case GraphicType::Ellipse:
			case GraphicType::Polyline:
			case GraphicType::Text:
			case GraphicType::Spline:
				g = _document->layer(data[2])->shape(std::stoi(data[1]));
				break;
			case GraphicType::CamMoveTo:
			case GraphicType::CamFollow:
			case GraphicType::CamOffset:
			case GraphicType::CamSpiral:
			case GraphicType::CamPocket:
			case GraphicType::CamDrill:
				g = _document->group(data[2])->toolpath(std::stoi(data[1]));

				break;
			}

			if (g != nullptr)
			{
				output += g->write() + '\n';
				g->read(v);
			}
		}

		if (!output.empty())
			return HistoryAction(HistoryActionType::Modify, output);
		else 
			return HistoryAction(HistoryActionType::None, "");
	}
	else 
		return action;
}

void ModCad::convert_unit()
{
	auto target = _document->unit() == Unit::Millimeter ? Unit::Inch : Unit::Millimeter;
	float factor = target == Unit::Inch ? 1.0f / 25.4f : 25.4f;
	auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(factor, factor, 1.0f));

	// only the drawing is converted, the toolpaths are defined afterwards
	for (Layer* layer : _document->layers())
	{
		for (Shape* shape : layer->shapes())
		{
			shape->transform(scale);
			if (shape->type() == GraphicType::Text)
			{
				auto text = (Text*)shape;
				text->height(text->height() * factor);
				text->inter(text->inter() * factor);
			}
		}
	}

	_document->unit(target);
	_document->modified(true);

	// the unit change cannot be undone, the history starts again
	clear_history();
}

void ModCad::undo()
{
	HistoryAction action = History::undo();
	if (action.type == HistoryActionType::EndRecord)
	{
		History::begin_redo_record();
		action = History::undo();

		do
		{
			HistoryAction next = process_action(action);
			History::redo(next);
			action = History::undo(); 
		} while (action.type != HistoryActionType::BeginRecord);

		History::end_redo_record();
	}
	else
	{
		HistoryAction next = process_action(action);
		History::redo(next);
	}
}

void ModCad::redo()
{
	HistoryAction action = History::redo();
	if (action.type == HistoryActionType::EndRecord)
	{
		History::begin_undo_record(false);
		action = History::redo();

		do
		{
			HistoryAction next = process_action(action);
			History::undo(next, false);
			action = History::redo();
		} while (action.type != HistoryActionType::BeginRecord);

		History::end_undo_record(false);
	}
	else
	{
		HistoryAction next = process_action(action);
		History::undo(next, false);
	}
}

void ModCad::clear_history()
{
	History::clear();
}

bool ModCad::can_undo()
{
	return History::has_undo();
}

bool ModCad::can_redo()
{
	return History::has_redo();
}

//***************************************************************************************************
//
//                                         CAD PROCESSING
//
//***************************************************************************************************


void ModCad::reverse()
{
	bool proceed = false;
	for (Graphic* g : _document->selected())
	{
		if (g->shape())
		{
			proceed = true;
			break;
		}
	}
	if (proceed)
	{
		History::begin_undo_record();
		
		for (Graphic* g : _document->selected())
		{
			if (g->shape())
				((Shape*)g)->reverse();
		}

		History::end_undo_record();
	}
}

void ModCad::cut_at_intersections()
{
	std::vector<Shape*> original, shapes, removed;
	for (Graphic* g : _document->selected())
	{
		if (g->shape())
		{
			original.push_back((Shape*)g);
		}
	}
	
	for(Shape* s : original)
	{
		if (s->type() == GraphicType::Polyline)
		{// check for self-intersection
			auto result = cad::split(((Polyline*)s)->coordinates());
			if (result.size() > 1)
			{
				removed.push_back(s);
				for (std::vector<glm::vec2> points : result)
				{
					Shape* p = nullptr;
					if (points.size() == 2)
					{
						p = new Line(_render);
						((Line*)p)->set(points[0], points[1]);
					}
					else
					{
						p = new Polyline(_render);
						((Polyline*)p)->points(points);
					}
					p->parent(s->parent());
					shapes.push_back(p);
				}
			}
			else
				shapes.push_back(s);
		}
		else
			shapes.push_back(s);
	}

	std::vector<std::vector<glm::vec2>> list_points;
	list_points.reserve(shapes.size());
	for (size_t i = 0; i < shapes.size(); i++)
		list_points.push_back(std::vector<glm::vec2>());

	for (size_t i = 0; i < shapes.size(); i++)
	{
		for (size_t j = i+1; j < shapes.size(); j++)
		{
			Shape* a = shapes[i], * b = shapes[j];

			if (geometry::rectangle_intersect(a->topLeft(), a->bottomRight(), b->topLeft(), b->bottomRight()) ||
				geometry::rectangle_contains(a->topLeft(), a->bottomRight(), b->topLeft(), b->bottomRight()) ||
				geometry::rectangle_contains(b->topLeft(), b->bottomRight(), a->topLeft(), a->bottomRight()))
			{
				if (a->type() == GraphicType::Circle && b->type() == GraphicType::Line
					|| a->type() == GraphicType::Arc && b->type() == GraphicType::Line
					|| a->type() == GraphicType::Arc && b->type() == GraphicType::Circle
					|| a->type() == GraphicType::Polyline && b->type() == GraphicType::Line
					|| a->type() == GraphicType::Ellipse && b->type() == GraphicType::Line
					|| a->type() == GraphicType::Polyline && b->type() == GraphicType::Circle
					|| a->type() == GraphicType::Ellipse && b->type() == GraphicType::Circle
					|| a->type() == GraphicType::Polyline && b->type() == GraphicType::Arc
					|| a->type() == GraphicType::Ellipse && b->type() == GraphicType::Arc)
				{
					Shape* c = a;
					a = b; b = c;
				}

				if (a->type() == GraphicType::Line)
				{
					if (b->type() == GraphicType::Line)
					{
						glm::vec2 p = glm::vec2();
						if (geometry::segment_segment_intersect(((Line*)a)->p1(), ((Line*)a)->p2(), ((Line*)b)->p1(), ((Line*)b)->p2(), p))
						{
							list_points[i].push_back(p);
							list_points[j].push_back(p);
						}
					}
					else if (b->type() == GraphicType::Circle)
					{
						glm::vec2 p[2] = { glm::vec2(),glm::vec2() };
						int count = geometry::segment_circle_intersect(((Line*)a)->p1(), ((Line*)a)->p2(), ((Circle*)b)->center(), ((Circle*)b)->radius(), p);
						if (count > 0)
						{
							for (int index = 0; index < count; index++)
							{
								list_points[i].push_back(p[index]);
								list_points[j].push_back(p[index]);
							}
						}
					}
					else if (b->type() == GraphicType::Arc)
					{
						Arc* s = (Arc*)b;
						glm::vec2 p[2] = { glm::vec2(),glm::vec2() };
						bool reversed = false;
						if (reversed = s->cw())
							s->reverse();
						int count = geometry::segment_arc_intersect(((Line*)a)->p1(), ((Line*)a)->p2(), s->start_angle(), s->center(), s->stop_angle(), s->radius(), s->cw(), p);
						if (count > 0)
						{
							for (int index = 0; index < count; index++)
							{
								list_points[i].push_back(p[index]);
								list_points[j].push_back(p[index]);
							}
						}
						if (reversed)
							s->reverse();
					}
					else if (b->type() == GraphicType::Ellipse || b->type() == GraphicType::Polyline)
					{
						auto coordinates = b->type() == GraphicType::Ellipse ? ((Ellipse*)b)->coordinates() : ((Polyline*)b)->coordinates();
						std::vector<glm::vec2> points = std::vector<glm::vec2>();
						if (geometry::segment_polyline_intersect(((Line*)a)->p1(), ((Line*)a)->p2(), coordinates, points))
						{
							for (glm::vec2 p : points)
							{
								list_points[i].push_back(p);
								list_points[j].push_back(p);
							}
						}
					}
				}
				else if (a->type() == GraphicType::Circle)
				{
					if (b->type() == GraphicType::Circle)
					{
						Circle* c1 = (Circle*)a;
						Circle* c2 = (Circle*)b;
						glm::vec2 p[2] = { glm::vec2(),glm::vec2() };
						int count = geometry::circle_circle_intersect(c1->center(), c1->radius(), c2->center(), c2->radius(), p);
						if (count > 0)
						{
							for (int index = 0; index < count; index++)
							{
								list_points[i].push_back(p[index]);
								list_points[j].push_back(p[index]);
							}
						}
					}
					else if (b->type() == GraphicType::Arc)
					{
						Circle* c = (Circle*)a;
						Arc* s = (Arc*)b;
						bool reversed = false;
						if (reversed = s->cw())
							s->reverse();
						glm::vec2 p[2] = { glm::vec2(),glm::vec2() };
						int count = geometry::arc_circle_intersect(s->start_angle(), s->center(), s->stop_angle(), s->radius(), s->cw(), c->center(), c->radius(), p);
						if (count > 0)
						{
							for (int index = 0; index < count; index++)
							{
								list_points[i].push_back(p[index]);
								list_points[j].push_back(p[index]);
							}
						}
						if (reversed)
							s->reverse();
					}
					else if (b->type() == GraphicType::Ellipse || b->type() == GraphicType::Polyline)
					{
						auto coordinates = b->type() == GraphicType::Ellipse ? ((Ellipse*)b)->coordinates() : ((Polyline*)b)->coordinates();
						std::vector<glm::vec2> points = std::vector<glm::vec2>();
						if (geometry::circle_polyline_intersect(((Circle*)a)->center(), ((Circle*)a)->radius(), coordinates, points))
						{
							for (glm::vec2 p : points)
							{
								list_points[i].push_back(p);
								list_points[j].push_back(p);
							}
						}
					}
				}
				else if (a->type() == GraphicType::Arc)
				{
					if (b->type() == GraphicType::Arc)
					{
						Arc* c1 = (Arc*)a;
						Arc* c2 = (Arc*)b;
						bool reversed1 = false;
						if (reversed1 = c1->cw())
							c1->reverse();
						bool reversed2 = false;
						if (reversed2 = c2->cw())
							c2->reverse();
						glm::vec2 p[2] = { glm::vec2(),glm::vec2() };
						int count = geometry::arc_arc_intersect(c1->start_angle(),c1->center(),  c1->stop_angle(), c1->radius(), c1->cw(), c2->start_angle(), c2->center(), c2->stop_angle(), c2->radius(), c2->cw(), p);
						if (count > 0)
						{
							for (int index = 0; index < count; index++)
							{
								list_points[i].push_back(p[index]);
								list_points[j].push_back(p[index]);
							}
						}
						if (reversed1)
							c1->reverse();
						if (reversed2)
							c2->reverse();
					}
					else if (b->type() == GraphicType::Ellipse || b->type() == GraphicType::Polyline)
					{
						bool reversed = false;
						if (reversed = ((Arc*)a)->cw())
							((Arc*)a)->reverse();
						auto coordinates = b->type() == GraphicType::Ellipse ? ((Ellipse*)b)->coordinates() : ((Polyline*)b)->coordinates();
						std::vector<glm::vec2> points = std::vector<glm::vec2>();
						if (geometry::arc_polyline_intersect(((Arc*)a)->start_angle(), ((Arc*)a)->center(), ((Arc*)a)->stop_angle(), ((Arc*)a)->radius(), ((Arc*)a)->cw(), coordinates, points))
						{
							for (glm::vec2 p : points)
							{
								list_points[i].push_back(p);
								list_points[j].push_back(p);
							}
						}
						if (reversed)
							((Arc*)a)->reverse();
					}
				}
				else if (a->type() == GraphicType::Polyline || a->type() == GraphicType::Ellipse)
				{
					if (b->type() == GraphicType::Ellipse || b->type() == GraphicType::Polyline)
					{
						auto coord1 = a->type() == GraphicType::Ellipse ? ((Ellipse*)a)->coordinates() : ((Polyline*)a)->coordinates();
						auto coord2 = b->type() == GraphicType::Ellipse ? ((Ellipse*)b)->coordinates() : ((Polyline*)b)->coordinates();
						std::vector<glm::vec2> points = std::vector<glm::vec2>();
						if (geometry::polyline_polyline_intersect(coord1, coord2, points))
						{
							for (glm::vec2 p : points)
							{
								list_points[i].push_back(p);
								list_points[j].push_back(p);
							}
						}
					}
				}
			}
		}
	}

	// we look for new created shapes
	std::vector<Shape*> new_shapes;
	for (int i = 0; i < shapes.size(); i++)
	{
		if (list_points[i].size() > 0)
		{
			//if (shapes[i]->type() == GraphicType::Line)
			//{
				std::vector<Shape*> r = cad::split(shapes[i], list_points[i]);

				// we get sure that a is from original shape list before including it into removed list
				if (std::find(original.begin(), original.end(), shapes[i]) != original.end())
					removed.push_back(shapes[i]);
				new_shapes.insert(new_shapes.end(), r.begin(), r.end());
			//}
		}
		else if(std::find(original.begin(), original.end(), shapes[i]) == original.end())
			new_shapes.push_back(shapes[i]);
	}

	if (removed.size() > 0 || new_shapes.size() > 0)
	{
		History::begin_undo_record();
		
		remove(removed);
		add(new_shapes);

		History::end_undo_record();
	}

	// put back unmodified shapes to new_shape list to set the new selected list
	for (Shape* s : original)
	{
		if (std::find(removed.begin(), removed.end(), s) == removed.end())
			new_shapes.push_back(s);
	}

	unselect_all();
	select(new_shapes);
}

void ModCad::set_reference_to_text()
{
	Text* t = nullptr;
	Shape* s = nullptr;

	if (_document->selected()[0]->type() == GraphicType::Text)
		t = (Text*)_document->selected()[0];
	else if (_document->selected()[0]->shape())
		s = (Shape*)_document->selected()[0];

	if (_document->selected()[1]->type() == GraphicType::Text)
		t = (Text*)_document->selected()[1];
	else if (_document->selected()[1]->shape())
		s = (Shape*)_document->selected()[1];

	if (s == nullptr || t == nullptr)
		return;

	t->reference_path(std::vector<glm::vec2>(), false);

	std::vector<Shape*> shapes = look_for_siblings(s);

	std::vector<glm::vec2> coordinates = cad::get_raw_coordinates(shapes);

	bool closed = false;

	if (shapes.size() == 1)
	{
		if (shapes[0]->type() == GraphicType::Circle || (shapes.size() == 1 && shapes[0]->type() == GraphicType::Ellipse))
			closed = true;
	}
	else
	{

	}

	t->reference(s->id());
	t->reference_path(coordinates, closed);
	t->reference_pos(0);

	select(shapes);
}

std::vector<Shape*> ModCad::look_for_siblings(Shape* s)
{
	std::vector<Shape*> result;
	result.push_back(s);

	if (s->type() == GraphicType::Line || s->type() == GraphicType::Arc || s->type() == GraphicType::Polyline)
	{
		std::vector<Shape*> shapes;
		for (Shape* shape : _document->layer(s->parent())->shapes())
			if (shape != s && (shape->type() == GraphicType::Line || shape->type() == GraphicType::Arc || shape->type() == GraphicType::Polyline))
				shapes.push_back(shape);

		if (shapes.size() > 0)
		{
			glm::vec2 left = s->first(), right = s->last();

			std::vector<Shape*>::iterator it = shapes.begin();
			while(it != shapes.end())
			{
				if ((*it)->last() == right)
					(*it)->reverse();
				else if (geometry::distance((*it)->last(), right) < geometry::ERR_FLOAT6)
				{
					if ((*it)->type() == GraphicType::Line)
						((Line*)(*it))->p2(right);
					(*it)->reverse();
				}

				if ((*it)->first() == left)
					(*it)->reverse();
				else if (geometry::distance((*it)->first(), left) < geometry::ERR_FLOAT6)
				{
					if ((*it)->type() == GraphicType::Line)
						((Line*)(*it))->p1(left);
					(*it)->reverse();
				}

				if ((*it)->first() == right)
				{
					right = (*it)->last();
					result.push_back((*it));
					shapes.erase(it);
					it = shapes.begin();
				}
				else if ((*it)->last() == left)
				{
					left = (*it)->first();
					result.insert(result.begin(), (*it));
					shapes.erase(it);
					it = shapes.begin();
				}
				else if (geometry::distance((*it)->first(), left) < geometry::ERR_FLOAT6)
				{
					if ((*it)->type() == GraphicType::Line)
						((Line*)(*it))->p1(left);
					(*it)->reverse();
				}
				else
					it++;
			}
		}
	}

	return result;
}

void ModCad::connect_shapes()
{
	float precision = geometry::ERR_FLOAT3;

	for (Layer* l : _document->layers())
	{
		std::list<Shape*> shapes;
		for (Shape* s : l->shapes())
		{
			if (s->type() == GraphicType::Line || s->type() == GraphicType::Arc || s->type() == GraphicType::Polyline)
				shapes.push_back(s);
		}

		if (shapes.size() < 2)
			return;

		bool end = false, found = false;
		std::list<Shape*>::iterator it = shapes.begin(), previous;
		std::vector<Shape*> construction;
		construction.push_back(*it);
		shapes.pop_front();
		it = shapes.begin();
		int i = 0, j = 0;

		while (shapes.size() > 0)
		{
			i++;
			j = (int)shapes.size();
			Shape* s1 = construction.back();
			Shape* s2 = (*it);

			if (are_shapes_connected(s1, s2, precision, true))
			{
				construction.push_back(s2);
				previous = it;
				it = std::next(it);
				if (it == shapes.end())
				{
					it = shapes.begin();
					shapes.pop_back();
				}
				else
				{
					shapes.erase(previous);
				}
				if (construction.size() > 1)
				{
					if (are_shapes_connected(construction.back(),construction.front(), precision, false))
						end = true;
				}
				found = true;
			}
			else
			{
				it = std::next(it);
				if (it == shapes.end())
				{
					if (found && shapes.size() > 0)
					{
						it = shapes.begin();
						found = false;
					}
					else
						end = true;
				}
			}

			if (end)
			{
				found = false;
				construction.clear();
				j = (int)shapes.size();
				if (shapes.size() > 0)
				{
					construction.push_back(shapes.front());
					shapes.pop_front();
					it = shapes.begin();
					end = false;
				}
			}
		}


	}
}

bool ModCad::are_shapes_connected(Shape* s1, Shape* s2, float precision, bool reverse)
{

	if (s1->last() == s2->first())
	{
		return true;
	}
	else if (geometry::distance(s1->last(), s2->first()) < precision)
	{
		if (s1->type() == GraphicType::Line || s1->type() == GraphicType::Polyline) // try to not modify arcs
		{
			s1->last(s2->first());
			return true;
		}
		else
		{
			s2->first(s1->last());
			return true;
		}
	}
	else if (s1->last() == s2->last())
	{
		s2->reverse();
		return true;
	}
	else if (geometry::distance(s1->last(), s2->last()) < precision)
	{
		s2->reverse();
		if (s1->type() == GraphicType::Line || s1->type() == GraphicType::Polyline) // try to not modify arcs
		{
			s1->last(s2->first());
			return true;
		}
		else
		{
			s2->first(s1->last());
			return true;
		}
	}
	return false;
}

void ModCad::info(std::vector<Shape*> shapes)
{
	Logger::trace(cad::info(shapes));
}

TreeCurve* ModCad::cad_tree()
{
	std::vector<Shape*> candidates;
	std::vector<Shape*> splines;
	std::vector<Curve> curves;
	std::vector<Graphic*> selection;

	for (Graphic* g : _document->selected())
	{
		if (g->shape())
			candidates.push_back((Shape*)g);
	}

	int tag = 0;

	while (candidates.size() > 0)
	{
		auto shapes = look_for_siblings(candidates[0]);

		// stores shapes which are part of tree
		for (Shape* s : shapes)
			if (std::find(selection.begin(), selection.end(), s) == selection.end())
				selection.push_back(s);

		if (shapes[0]->type() == GraphicType::Text)
		{
			shapes[0]->tag(tag);
			auto coordinates = ((Text*)shapes[0])->coordinates();
			if (coordinates.size() > 2)
			{
				for (std::vector<glm::vec2> points : coordinates)
				{
					Curve c = cad::to_curve(points);
					c.close();
					c.tag(tag);
					c.reference(shapes.front()->id());
					if (c.size() > 1)
						curves.push_back(c);
				}
			}
		}
		else
		{
			Curve c = cad::to_curve(shapes);
			c.tag(tag);
			c.reference(shapes.front()->id());
			if (c.size() >= 2)
			{
				curves.push_back(c);
			}
		}

		for (Shape* s : shapes)
		{
			s->tag(tag);
			auto it = std::find(candidates.begin(), candidates.end(), s);
			if (it != candidates.end())
				candidates.erase(it);
		}

		tag++;
	}

	TreeCurve* tree = new TreeCurve(curves);
	tree->sort();
	tree->cw_alter();

	_document->unselect_all();
	_document->select(selection);

	return tree;
}

void ModCad::store_operation()
{
	_operation_shapes.clear();
	for (Graphic* g : _document->selected())
	{
		if (g->shape())
			_operation_shapes.push_back((Shape*)g);
	}
}



void ModCad::cad_offset(float value)
{
	auto t = std::chrono::high_resolution_clock::now();

	if (_document->selected().size() > 0 && _document->selected()[0]->shape())
	{
		TreeCurve* tree = cad_tree();

		if (tree->size() > 0)
		{
			std::vector<Shape*> off_splines;
			for (TreeCurve* t : tree->children)
			{
				bool cw = t->curve->cw();
				auto curves = t->curves();

				for (Curve c : curves)
				{
					auto curves = c.offset(c.cw() == cw ? value : -value);

					for (Curve c : curves)
					{
						Spline* spline = new Spline(_render);
						spline->add(c);
						spline->close();
						off_splines.push_back(spline);
						spline->done(true);
						spline->compute();
					}
				}
			}
			
			History::begin_undo_record();
			for (Shape* s : off_splines)
			{
				_document->current_layer()->add(s);
				History::undo(HistoryActionType::Add, s->write());
			}
			History::end_undo_record();
			select(off_splines);
		}
		delete tree;
	}
	Logger::log("cad_offset process time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
}

void ModCad::cad_weiler(int code)
{
	auto t = std::chrono::high_resolution_clock::now();

	if (_document->selected().size() > 0 && _document->selected()[0]->shape())
	{
		std::vector<int> indices;
		TreeCurve* tree = cad_tree();
		std::vector<Curve> result;

		if (tree->size() > 0)
		{
			std::vector<Curve> curves = tree->curves();

			if (curves.size() > 1)
			{
				std::vector<Curve>::iterator it1 = curves.begin();
				while (it1 != curves.end() && curves.size() > 1)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != curves.end())
					{
						std::vector<Curve> merged_curves;
						if (code == 0)
							merged_curves = (*it1).boolean_union((*it2));
						else if (code == 1)
							merged_curves = (*it1).boolean_substract((*it2));
						else if (code == 2)
							merged_curves = (*it1).boolean_intersect((*it2));

						if (merged = merged_curves.size() > 0)
						{
							indices.push_back(it2->tag());
							indices.push_back(it1->tag());
							curves.erase(it2);
							curves.erase(it1);
							curves.insert(curves.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
							it2++;
					}
					if (merged)
						it1 = curves.begin();
					else
						it1++;
				}
			}

			for(Curve& c : curves)
				if (c.index() == -1)
					result.push_back(c);
		}
		delete tree;

		std::vector<Shape*> off_splines;
		for (Curve c : result)
		{
			Spline* spline = new Spline(_render);
			spline->add(c);
			spline->close();
			off_splines.push_back(spline);
			spline->done(true);
			spline->compute();
		}

		std::vector<Shape*> s_to_remove;
		for (Layer* l : _document->layers())
		{
			for (Shape* s : l->shapes())
				if (s->tag() != -1 && std::find(indices.begin(), indices.end(), s->tag()) != indices.end())
					s_to_remove.push_back(s);
				else
					s->tag(-1);
		}

		History::begin_undo_record();
		for (Shape* s : off_splines)
		{
			_document->current_layer()->add(s);
			History::undo(HistoryActionType::Add, s->write());
		}
		for (Shape* s : s_to_remove)
		{
			History::undo(HistoryActionType::Delete, s->write());
			_document->layer(s->parent())->remove(s);
		}
		History::end_undo_record();
		select(off_splines);
	}
	Logger::log("cad_offset process time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
}

void ModCad::cad_origin()
{
	std::string modify;

	auto m = glm::translate(glm::vec3(-_magnet_point.x, -_magnet_point.y, 1.0f));
	for (Layer* l : _document->layers())
	{
		for(Shape* s : l->shapes())
		{
			modify += s->write() + "\n"; // for history modification record, stores original value
			s->transform(m); // processing shape move
		}
	}
	History::undo(HistoryActionType::Modify, modify);

	//_render->camera()->mouseRightDown();
	//_render->camera()->mouseMove(_magnet_point.x - _left, _magnet_point.y - _top);
	//_render->camera()->mouseRightUp();
}

void ModCad::cad_symmetry()
{
	std::vector<Graphic*> created;
	if (_document->selected().size() > 0 && _document->selected()[0]->shape())
	{
		Shape* symetry = (Shape*)_document->selected()[0];
		if (symetry->type() == GraphicType::Line)
		{
			auto p1 = ((Line*)symetry)->p1();
			auto p2 = ((Line*)symetry)->p2();
			for (Shape* s : _operation_shapes)
			{
				Shape* c = s->symmetry(p1, p2);
				_document->current_layer()->add(c);
				created.push_back(c);
			}
		}
		else if (symetry->type() == GraphicType::Point)
		{
			auto p1 = ((Point*)symetry)->p1();
			for (Shape* s : _operation_shapes)
			{
				Shape* c = s->symmetry(p1);
				_document->current_layer()->add(c);
				created.push_back(c);
			}
		}
	}
	if (created.size() > 0)
	{
		History::begin_undo_record();
		for(Graphic* g : created)
			History::undo(HistoryActionType::Add, g->write());
		History::end_undo_record();
	}
	_document->unselect_all();
	_document->select(created);
	_cad_creation = GraphicType::None;
}

void ModCad::cad_scale(float value)
{
	std::vector<Graphic*> created;
	for(Graphic* g : _document->selected())
	{
		if (g->shape())
		{
			auto m = glm::scale(glm::vec3(value, value, 1.0f));
			Shape* s = ((Shape*)g)->clone();
			s->transform(m);
			_document->current_layer()->add(s);
			created.push_back(s);
		}
	}
	if (created.size() > 0)
	{
		History::begin_undo_record();
		for (Graphic* g : created)
			History::undo(HistoryActionType::Add, g->write());
		History::end_undo_record();
	}
	_document->unselect_all();
	_document->select(created);
	_cad_creation = GraphicType::None;
}

void ModCad::cad_rotation(float value)
{
	std::vector<Graphic*> created;
	for (Graphic* g : _operation_shapes)
	{
		if (g->shape())
		{
			auto m = glm::translate(glm::vec3(-_left_mouse_mag.x, -_left_mouse_mag.y, 1.0f));
			m = glm::rotate(glm::radians(value), glm::vec3(0, 0, 1.0f)) * m;
			m = glm::translate(glm::vec3(+_left_mouse_mag.x, +_left_mouse_mag.y, 1.0f)) * m;
			Shape* s = ((Shape*)g)->clone();
			s->transform(m);
			_document->current_layer()->add(s);
			created.push_back(s);
		}
	}
	if (created.size() > 0)
	{
		History::begin_undo_record();
		for (Graphic* g : created)
			History::undo(HistoryActionType::Add, g->write());
		History::end_undo_record();
	}
	_document->unselect_all();
	_document->select(created);
	_cad_creation = GraphicType::None;
}

void ModCad::cad_fillet(float value)
{
	if (_document->selected().size() > 0 && _document->selected()[0]->shape())
	{
		std::vector<Shape*> shapes;
		for (Graphic* g : _document->selected())
		{
			if (g->shape())
				shapes.push_back(((Shape*)g));
		}

		auto result = cad::fillet(shapes, value);

		std::string history_modify = "", history_add = "";

		for (auto s : result)
		{
			bool new_shape = true;
			for (auto c : shapes)
			{
				if (s->id() == c->id())
				{
					history_modify += c->write() + "\n";
					c->read(s->write());
					new_shape = false;
					break;
				}
			}
			if (new_shape)
			{
				_document->current_layer()->add(s);
				history_add += s->write() + "\n";
			}
		}

		History::begin_undo_record();
		History::undo(HistoryActionType::Add, history_add);
		History::undo(HistoryActionType::Modify, history_modify);
		History::end_undo_record();
	}
}

Shape* ModCad::find_shape(unsigned int id)
{
	for (Layer* l : _document->layers())
	{
		for (Shape* s : l->shapes())
		{
			if (s->id() == id)
				return s;
		}
	}
	return nullptr;
}

void ModCad::insert(std::string code)
{
	auto data = stringex::split(code, ':');

	// D:id -> delete a shape
	if (data[0] == "D" && data.size() > 1)
	{
		auto s = find_shape((unsigned int)std::stoul(data[1]));
		Layer* layer = s != nullptr ? _document->layer(s->parent()) : nullptr;
		if (layer == nullptr)
		{
			Logger::error("Script: shape " + data[1] + " not found");
			return;
		}

		History::undo(HistoryActionType::Delete, s->write());
		s->selected(false);
		auto& selection = _document->selected();
		selection.erase(std::remove(selection.begin(), selection.end(), (Graphic*)s), selection.end());
		layer->remove(s);
		return;
	}

	// M:id:L|C|A:parameters -> change the parameters of a shape, the id and the type do not change
	if (data[0] == "M" && data.size() > 2)
	{
		auto s = find_shape((unsigned int)std::stoul(data[1]));
		if (s == nullptr)
		{
			Logger::error("Script: shape " + data[1] + " not found");
			return;
		}

		auto v = [&](size_t i) { return (float)std::atof(data[i].c_str()); };

		if (data[2] == "L" && s->type() == GraphicType::Line && data.size() >= 7)
		{
			History::undo(HistoryActionType::Modify, s->write());
			((Line*)s)->set(glm::vec2(v(3), v(4)), glm::vec2(v(5), v(6)));
		}
		else if (data[2] == "C" && s->type() == GraphicType::Circle && data.size() >= 6)
		{
			History::undo(HistoryActionType::Modify, s->write());
			((Circle*)s)->set(glm::vec2(v(3), v(4)), v(5));
		}
		else if (data[2] == "A" && s->type() == GraphicType::Arc && data.size() >= 10)
		{
			History::undo(HistoryActionType::Modify, s->write());
			((Arc*)s)->set(glm::vec2(v(3), v(4)), glm::vec2(v(5), v(6)), glm::vec2(v(7), v(8)), std::atoi(data[9].c_str()) != 0);
		}
		else
			Logger::error("Script: shape " + data[1] + " keeps its type, cannot apply " + data[2]);
		return;
	}
	if (data[0] == "L")
	{
		Line* l = new Line(_render);

		l->point(glm::vec2(std::atof(data[1].c_str()), std::atof(data[2].c_str())));
		l->point(glm::vec2(std::atof(data[3].c_str()), std::atof(data[4].c_str())));
		l->done();
		l->compute();
		_document->current_layer()->add(l);
		History::undo(HistoryActionType::Add, l->write());
	}
	if (data[0] == "C")
	{
		Circle* c = new Circle(_render);

		c->center(glm::vec2(std::atof(data[1].c_str()), std::atof(data[2].c_str())));
		c->radius((float)std::atof(data[3].c_str()));
		c->done();
		c->compute();
		_document->current_layer()->add(c);
		History::undo(HistoryActionType::Add, c->write());
	}
	if (data[0] == "A")
	{
		Arc* a = new Arc(_render);

		a->start(glm::vec2(std::atof(data[1].c_str()), std::atof(data[2].c_str())));
		a->center(glm::vec2(std::atof(data[3].c_str()), std::atof(data[4].c_str())));
		a->stop(glm::vec2(std::atof(data[5].c_str()), std::atof(data[6].c_str())));
		a->cw(std::atoi(data[7].c_str()));
		a->done();
		a->compute();
		_document->current_layer()->add(a);
		History::undo(HistoryActionType::Add, a->write());
	}
}

void ModCad::cad_chamfer(float value)
{
	if (_document->selected().size() > 0 && _document->selected()[0]->shape())
	{
		std::vector<Shape*> shapes;
		for (Graphic* g : _document->selected())
		{
			if (g->shape())
				shapes.push_back(((Shape*)g));
		}

		auto result = cad::chamfer(shapes, value);

		std::string history_modify = "", history_add = "";

		for (auto s : result)
		{
			bool new_shape = true;
			for (auto c : shapes)
			{
				if (s->id() == c->id())
				{
					history_modify += c->write() + "\n";
					c->read(s->write());
					new_shape = false;
					break;
				}
			}
			if (new_shape)
			{
				_document->current_layer()->add(s);
				history_add += s->write() + "\n";
			}
		}

		History::begin_undo_record();
		History::undo(HistoryActionType::Add, history_add);
		History::undo(HistoryActionType::Modify, history_modify);
		History::end_undo_record();
	}
}

//***************************************************************************************************
//
//                                         CAM PROCESSING
//
//***************************************************************************************************


void ModCad::cam_moveto()
{
	MoveTo* m = new	MoveTo(_render);
	Curve c;
	c.add(_magnet_point);
	m->add(c);
	m->compute();
	_tab_flags = ImGuiTabItemFlags_SetSelected;
	ImGui::SetWindowFocus(Lang::l("GROUP"));
	_document->current_group()->add(m);
	_document->unselect_all();
	_document->select(m);
}

void ModCad::cam_drill()
{
	Drill* d = new	Drill(_render);
	Curve c;
	_tab_flags = ImGuiTabItemFlags_SetSelected;
	ImGui::SetWindowFocus(Lang::l("GROUP"));
	c.add(_magnet_point);
	d->add(c);
	d->compute();
	_document->current_group()->add(d);
	_document->unselect_all();
	_document->select(d);
}

void ModCad::cam_spiral()
{
	_tab_flags = ImGuiTabItemFlags_SetSelected;
}

void ModCad::cam_follow()
{
	auto shapes = get_selection();

	if (_document->selected().size() == 0 || !_document->selected()[0]->shape())
	{
		_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
	}
	else
	{
			TreeCurve* tree = cad_tree();

			if (tree->size() > 0)
			{
				//Follow* f = new	Follow(_render);

				///*for (auto c : curves)
				//	f->add(c);*/
				//f->references(get_references(_document->selected()));
				//f->cw(_document->current_group()->cw());
				//f->tree(tree);
				//f->compute();
				//_document->current_group()->add(f);
				//_document->unselect_all();
				//_document->select(f);
				//_tab_flags = ImGuiTabItemFlags_SetSelected;

				auto curves = tree->curves();
				for (auto& c : curves)
				{
					Follow* f = new Follow(_render);
					f->add(c);
					f->reference(c.reference());
					bool cw = _document->current_group()->cw();
					f->cw(c.level() % 2 == 0 ? !cw : cw);
					f->compute();
					_document->current_group()->add(f);
					_document->unselect_all();
					_document->select(f);
					_tab_flags = ImGuiTabItemFlags_SetSelected;
				}
			}
	}
}

void ModCad::cam_offset(bool interior)
{
	auto shapes = get_selection();

	if (shapes.size() == 0)
	{
		_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
	}
	else
	{
		auto t = std::chrono::high_resolution_clock::now();

		if (_document->selected().size() > 0 && _document->selected()[0]->shape())
		{
			TreeCurve* tree = cad_tree();

			if (tree->size() > 0)
			{
				//Offset* o = new Offset(_render);
				//o->references(get_references(_document->selected()));
				//o->interior(interior);
				//o->radius(_document->current_group()->tool_radius());
				//o->cw(_document->current_group()->cw());
				//o->tree(tree);
				//o->compute();
				//_document->current_group()->add(o);
				//_document->unselect_all();
				//_document->select(o);
				//_tab_flags = ImGuiTabItemFlags_SetSelected;
	
				auto curves = tree->curves();
				for (auto& c : curves)
				{
					Offset* o = new Offset(_render);
					//o->references(get_references(_document->selected()));
					o->reference(c.reference());
					o->interior(c.level() % 2 == 0 ? !interior : interior);
					o->radius(_document->current_group()->tool_radius());
					o->cw(_document->current_group()->cw());
					bool cw = _document->current_group()->cw();
					o->cw(c.level() % 2 == 0 ? !cw : cw);
					o->add(c);
					o->compute();
					_document->current_group()->add(o);
					_document->unselect_all();
					_document->select(o);
					_tab_flags = ImGuiTabItemFlags_SetSelected;
				}
			}
		}
		Logger::log("cad_offset process time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
	}
}

void ModCad::cam_pocket(PocketMode mode)
{
	auto shapes = get_selection();

	if (shapes.size() == 0)
	{
		_message_box.display(Lang::t("ERROR") + "###MSG_BOX", Lang::l("ERR_SELECTION"));
	}
	else
	{
		auto t = std::chrono::high_resolution_clock::now();

		if (_document->selected().size() > 0 && _document->selected()[0]->shape())
		{
			TreeCurve* tree = cad_tree();

			if (tree->size() > 0)
			{
				Pocket* o = new Pocket(_render);
				o->mode(mode);
				o->radius(_document->current_group()->tool_radius());
				o->cw(_document->current_group()->cw());
				o->tree(tree);
				o->compute();
				_document->current_group()->add(o);
				_document->unselect_all();
				_document->select(o);
				_tab_flags = ImGuiTabItemFlags_SetSelected;
			}
		}
		Logger::log("cad_offset process time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
	}
}

