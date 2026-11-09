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
#include <vector>
#include <string>
#include "tools_window.h"
#include "machines_window.h"

#define CAD_UNION 0
#define CAD_SUBSTRACT 1
#define CAD_INTERSECT 2
#define CAD_NOT_INTERSECT 3

class Text;

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
	// preview of the extend tool: the part which would be added to the line or arc under the mouse
	Buffer* _b_extend = NULL;
	// markers of the diagnostic of the drawing (free ends, shapes of length zero or drawn twice), drawn until they are cleared
	Buffer* _b_diag = NULL;
	std::vector<glm::vec2> _diag_points;
	bool _extend_preview = false;
	Shape* _extend_target = nullptr;
	bool _extend_at_start = false;
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
	// the view of the document which has just been read, applied when the size of the drawing area is known
	bool _view_pending = false;
	void apply_view();
	// the view after a change of the camera: precision of the mouse, axes, grid, cursor
	void view_changed();
	// zoom and move the view so that all the shapes of the visible layers are in the drawing area
	void fit_view();
	// the area of the machine of the document under the drawing: the table, the zero of the machine and the direction of its axes
	void draw_machine();
	// the window of the settings of the nesting
	void draw_nesting();
	std::map<std::string, Texture*> _textures;

	MessageBox _message_box;

	bool _shift = false;
	bool _ctrl = false;
	bool _f2 = false;
	bool _is_over = false;
	bool _start_moving = false;
	Shape* _shape_hovered = nullptr;
	glm::vec2 _last_mouse_pos = glm::vec2();
	// double click detection (in pixels: the mouse always moves a little between the two clicks)
	double _last_click_time = 0;
	glm::vec2 _last_click_screen = glm::vec2();
	bool _double_click = false;
	glm::vec2 _right_down_screen = glm::vec2();
	// true while a polyline (not a Bezier) with at least two points placed is being drawn
	bool polyline_in_progress();
	// true while a chain of segments is being drawn and the start of the next segment is placed
	bool line_in_progress();
	void restart_line();
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
	int _shortcut_column = 0; // column (0 or 1) of the next icon in the right toolbar
	ImVec2 _last_size = ImVec2(0, 0);
	bool _display_arc_box = false;
	bool _display_chamfer_box = false;
	bool _display_offset_box = false;
	bool _display_pocket_box = false;
	// nesting: the parts found in the selection and the settings of the window (not saved in the document)
	struct NestPartInfo
	{
		std::string name;
		std::vector<Curve> curves;	// the outer contour, then the contours inside it, at any depth
		std::vector<int> depths;	// the depth of each contour inside the outer one: 1 for a hole, 2 for an island in a hole...
		std::vector<Shape*> shapes;
		float width = 0, height = 0;
		int quantity = 1;
	};
	std::vector<NestPartInfo> _nest_parts;
	bool _display_nesting = false;
	float _nest_width = 1000, _nest_height = 500, _nest_margin = 5, _nest_spacing = 3, _nest_step = 8;
	int _nest_rotation = 2;
	bool _nest_fill_holes = true;
	ToolsWindow _tools_window;
	MachinesWindow _machines_window;
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
	// all the shapes of the hovered block are drawn as hovered
	void hover_block();
	bool selection_grouped(int& shapes);

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
	// links a text to the shape (and the shapes connected to it) it follows, the change is undone in one step
	void link_text(Text* text, Shape* shape);
	// the coordinates of the curve a shape belongs to (the connected shapes), and if it is closed
	std::vector<glm::vec2> chain_coordinates(Shape* shape, bool& closed);
	Shape* find_shape(int id);
	// the linked texts (the given ones, or all of them) read their curve again: after a move of the curve, a change of the origin...
	// the position along the curve is kept. Returns the number of texts updated
	int refresh_texts(const std::vector<Text*>& only = std::vector<Text*>());
	// text waiting for the click on the curve to follow, nullptr if there is none
	Text* _link_text = nullptr;
	// the click which chose the curve is not a selection: the text stays selected when the button is released
	bool _link_done = false;
	void draw_link_hint();
	std::vector<Shape*> look_for_siblings(Shape* s);
	void connect_shapes();
#ifdef _DEBUG
	// development helpers, called from the Debug menu
	void debug_offset();
	void debug_convert_toolpath_to_spline();
#endif
	void info(std::vector<Shape*> shapes);
	void cad_offset(float value);
	void cad_weiler(int code);
	void cad_origin();
	// extend: the line or arc under the point is prolonged from its nearest end up to the nearest other entity
	void cad_extend(glm::vec2 point);
	// cut: the shapes under the click are cut at the clicked point (magnet applied) or at their crossing
	void cad_cut(glm::vec2 mouse, glm::vec2 snapped);
	// true if the shape is on a locked layer: it can not be modified, moved, deleted, cut or extended
	bool is_locked(Shape* shape);
	int extend_find(glm::vec2 point, Shape*& target, bool& at_start, glm::vec2& result);
	void extend_preview(glm::vec2 point);
	void cad_symmetry();
	void cad_scale(float value);
	void cad_rotation(float value);
	void cad_chamfer(float value);
	void cad_fillet(float value);
	// block: the selected shapes are gathered under a name, or the shapes of the selected blocks are released
	void cad_block_create();
	void cad_block_explode();
	// nesting: the window of the settings, then the parts (the blocks) are placed on the sheet
	void cad_nesting();
	void cad_nesting_run();
	// the test bridge: nests every shape of the document
	void cad_nesting_bridge(float width, float height, float spacing, int quantity);

	void insert(std::string code);
	Shape* find_shape(unsigned int id);
	std::string _script_layer;		// layer of the shapes added by the running script, empty for the current layer

	// CAM processing
	std::vector<Shape*> get_selection();
	std::vector<int> get_references(std::vector<Graphic*> shapes);
	// an icon button, its tooltip is the text of the code. active: a toggle which is on, drawn with a highlighted background
	bool set_shortcut(std::string code, ImVec2 size, bool active = false, bool enabled = true);
	void cam_moveto();
	void cam_drill();
	// the click would place the polyline point on the previous one
	bool polyline_click_repeated();
	void cam_follow();
	// is the drawing ready for machining: free ends, shapes of length zero or drawn twice. Reported in the log, with the shapes selected and markers
	void check_drawing();
	// selects every shape connected to the selection (the contour it belongs to)
	void select_contour();
	// computes the selected toolpaths again (all of them without selection) from their referenced shapes, which may have been modified
	void cam_refresh();
	void cam_offset(bool interior);
	void cam_pocket(PocketMode mode);
	void cam_spiral();


};