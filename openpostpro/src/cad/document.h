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
#include <string>
#include "graphic.h"
#include <vector>
#include <layer.h>
#include <group.h>
#include <machine.h>

// unit of the document: the drawing has no unit by itself, the unit applies to the post-processor and the imports
enum class Unit {
	Millimeter = 0,
	Inch = 1
};

enum doc_type {
	router
};

class Document
{
private:
	std::string _path;						// file path
	std::string _output;					// file output
	std::vector<Layer*> _layers;			// store layers
	std::vector<Group*> _groups;			// store groups
	doc_type _type = doc_type::router;		// type of document
	Renderer* _render = nullptr;			// rendering device	
	std::vector<Graphic*> _selected;		// store actual selected graphics elements
	std::vector<Graphic*> _references;		// store actual graphics elements used as references
	Layer* _current_layer = nullptr;		// current selected layer
	Group* _current_group = nullptr;		// current selected group
	bool _modified = false;
	// display of all the toolpaths at once, on top of the visibility of each group. Not saved in the file
	bool _show_toolpaths = true;

	float _scale = 1.0f;					// last camera scale factor
	// the view read from the file: the zoom steps, and the point of the drawing which is at the middle of the screen (the screen
	// can have another size than the one of the person who saved the file)
	bool _has_view = false;
	int _view_zoom = 0;
	glm::vec2 _view_middle = glm::vec2(0.0f);
	Machine _machine;						// the machine of the document (a copy of the one of the table: the document can be opened elsewhere)
	bool _has_machine = false;
	Unit _unit = Unit::Millimeter;			// unit of the coordinates

public:
	bool has_view() { return _has_view; }
	int view_zoom() { return _view_zoom; }
	glm::vec2 view_middle() { return _view_middle; }
	float view_scale() { return _scale; }

	Document();
	~Document();

	Renderer* render() { return _render; }
	void render(Renderer* r) { _render = r; reset(); }

	doc_type type() { return _type; }
	void type(doc_type value) { _type = value; }

	std::string path() { return _path; }
	void path(std::string value) { _path = value; }

	std::string output() { return _output; }

	bool has_machine() { return _has_machine; }
	Machine& machine() { return _machine; }
	void machine(const Machine& value) { _machine = value; _has_machine = true; }
	void clear_machine() { _has_machine = false; }

	Unit unit() { return _unit; }
	void unit(Unit value) { _unit = value; }
	void output(std::string value) { _output = value; }

	bool modified() { return _modified; }
	void modified(bool value) { _modified = value; }

	bool show_toolpaths() { return _show_toolpaths; }
	void show_toolpaths(bool value) { _show_toolpaths = value; }

	std::vector<Layer*>& layers() { return _layers; }
	Layer* layer(std::string name="");
	Layer* layer_of(Shape* shape);

	std::vector<Group*>& groups() { return _groups; }
	Group* group(std::string name="");

	std::vector<Graphic*>& selected() { return _selected; }

	Layer* current_layer() { return _current_layer; }
	void current_layer(Layer* value) { _current_layer = value; }
	
	Group* current_group() { return _current_group; }
	void current_group(Group* value) { _current_group = value; }

	/// <summary>
	/// Return the coordinates from a reference shape and all the other connected shapes
	/// </summary>
	/// <param name="r">shape to start from</param>
	/// <returns>coordinates</returns>
	std::vector<glm::vec2> reference(Shape* r);

	void remove_layer(Layer* value);
	void remove_layer(std::string name);
	void remove_group(Group* value);
	void remove_group(std::string name);

	void up(Graphic* g);
	void down(Graphic* g);

	void select_all();
	void unselect_all();
	void select(Graphic* g);
	void select(std::vector<Graphic*> list);
	void unselect(Graphic* g);
	void unselect(std::vector<Graphic*> list);

	void reset();
	void draw();
	void anchors();
	void clear();
	void init();
	void check();

	// IO
	std::string write(float version = 0);
	void read(std::string value);
};