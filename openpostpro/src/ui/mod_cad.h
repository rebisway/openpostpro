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
#include "module.h"
#include "renderer_opengl.h"
#include "ortho_cam.h"
#include <history.h>
#include <message_box.h>
#include <curve.h>
#include <pocket.h>

#define CAD_UNION 0
#define CAD_SUBSTRACT 1
#define CAD_INTERSECT 2
#define CAD_NOT_INTERSECT 3

class ModCad : public Module
{
private:
	Renderer* _render = NULL;
	OrthoCamera* _camera = NULL;
	Buffer* _b_cursor = NULL;
	Buffer* _b_axe = NULL;
	Buffer* _b_grid = NULL;
	Buffer* _b_decoration = NULL;
	Buffer* _b_selection = NULL;
	Buffer* _b_magnet = NULL;
	std::vector<int> _dec_indices;
	
	std::vector<glm::vec2> _magnets;
	std::vector<Shape*> _magnet_shapes;
	glm::vec2 _magnet_point = glm::vec2();
	glm::vec2 _magnet_point_v = glm::vec2();
	glm::vec2 _magnet_point_h = glm::vec2();
	bool _disable_magnet = false;

	bool _left_down = false;
	bool _right_down = false;
	bool _initialized = false;
	std::map<std::string, Texture*> _textures;

	MessageBox _message_box;

	bool _shift = false;
	bool _ctrl = false;
	bool _f2 = false;
	bool _is_over = false;
	bool _start_moving = false;
	Shape* _shape_hovered = nullptr;
	glm::vec2 _last_mouse_pos = glm::vec2();
	glm::vec2 _left_mouse_pos = glm::vec2();
	glm::vec2 _left_mouse_mag = glm::vec2();
	bool _mode_selection = false;

	GraphicType _cad_creation = GraphicType::None;
	Shape* _construction_shape = nullptr;
	std::vector<Shape*> _operation_shapes;


	glm::mat4x4 _mat = glm::imat4x4();

	// ui control variables
	bool _display_poly_box = false;
	ImVec2 _btn_pos = ImVec2(0, 0);
	ImVec2 _last_size = ImVec2(0, 0);
	bool _display_arc_box = false;
	bool _display_chamfer_box = false;
	bool _display_offset_box = false;
	bool _display_pocket_box = false;
	bool _display_delete_layer = false;
	bool _display_delete_group = false;
	int _tab_flags = 0;

	//undo/redo
	bool _modification_started = false;

private:
	bool show() override;
	bool closable() override;

	void unselect();
	void unselect(Graphic* g);
	void select(Graphic* g);
	void select(std::vector<Graphic*> graphics);
	void select(std::vector<Shape*> shapes);
	void select(std::vector<int> ids);
	void selecting(Shape* shape);
	void construct_shape();
	void remove(std::vector<Shape*> shapes);
	void add(std::vector<Shape*> shapes);
	bool are_shapes_connected(Shape* s1, Shape* s2, float precision, bool reverse=false);

	void reset_display_boxes() { _display_chamfer_box = _display_arc_box = _display_poly_box = _display_offset_box = false; }

	//undo/redo
	HistoryAction process_action(HistoryAction action);

	TreeCurve* cad_tree();

	void store_operation();

public:
	ModCad(Window* window);
	~ModCad();

	glm::vec2 mouse() { return _left_mouse_mag; }

	Renderer* render() override { return _render; }

	void document(Document* document) override;
	
	void document_loaded() override;

	void display(float left, float top, float width, float height) override;

	void mouseMouve(double x_pos, double y_pos) override;

	// called when left button is down
	void left_button_down() override;

	// called when left button is up
	void left_button_up() override;

	// called when right button is down
	void right_button_down() override;

	// called when right button is up
	void right_button_up() override;

	void scroll(double xdelta, double ydelta) override;

	void key_pressed(int key, int scancode, int action, int mods) override;

	void render_frame() override;

	void render_GUI() override;

	void render_menu() override;

	void updateAxe();

	void updateCursor();

	void updateGrid();

	void updateSelection();

	void updateMagnet();

	void update() override;

	// undo/redo
	void undo() override;
	void redo() override;
	void clear_history() override;
	bool can_undo() override;
	bool can_redo() override;

	// selection
	void select_all() override;
	void unselect_all() override;
	void reverse_selection() override;

	// clipboard
	void cut() override;
	void copy() override;
	void paste() override;
	void remove() override;

	void add_layer();
	void delete_layer();
	void add_group();
	void delete_group();

	
	// CAD processing
	void reverse();

	// scale the drawing to the other unit (millimeter <-> inch) and change the unit of the document
	void convert_unit();
	void cut_at_intersections();
	void set_reference_to_text();
	std::vector<Shape*> look_for_siblings(Shape* s);
	void connect_shapes();
	void info(std::vector<Shape*> shapes);
	void cad_offset(float value);
	void cad_weiler(int code);
	void cad_origin();
	void cad_symmetry();
	void cad_scale(float value);
	void cad_rotation(float value);
	void cad_chamfer(float value);
	void cad_fillet(float value);

	void insert(std::string code);
	Shape* find_shape(unsigned int id);
	std::string _script_layer;		// layer of the shapes added by the running script, empty for the current layer

	// CAM processing
	std::vector<Shape*> get_selection();
	std::vector<int> get_references(std::vector<Graphic*> shapes);
	bool set_shortcut(std::string code, ImVec2 size);
	void cam_moveto();
	void cam_drill();
	void cam_follow();
	void cam_offset(bool interior);
	void cam_pocket(PocketMode mode);
	void cam_spiral();

};