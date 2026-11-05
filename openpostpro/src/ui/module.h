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
#include "window.h"
#include "document.h"
#include "renderer.h"
#include <inifile.h>
#include <config.h>

class Module
{
protected:
	std::string _code;
	std::string _title;
	bool _selected = false;
	int _index=0;
	Window* _window;
	float _left = 0.0f;
	float _top = 0.0f;
	float _width = 500.0f;
	float _height = 500.0f;
	Document* _document = NULL;
	bool _full_screen_ui = true;
	bool _show = false;
	bool _modified = false;

public:
	Module(Window* window) { _window = window;}

	int index() { return _index; }

	bool full_screen_ui() { return _full_screen_ui; }

	std::string title() {
		return _title;
	};

	virtual bool modified() { return _modified; }
	virtual void modified(bool value) { _modified = value; }

	// save the file edited by the module, false if the module has no file to save
	virtual bool save() { return false; }

	virtual bool show() { return _show; }
	virtual void show(bool value) { _show = value; }

	virtual bool closable() { return true; }

	// true if the module is displayed in a panel outside the tabs
	virtual bool docked() { return false; }

	virtual bool selected() { return _selected; }
	virtual void selected(bool value) { _selected = value; }

	std::string code() { return _code; }

	virtual Renderer* render() { return NULL; }

	virtual void document(Document* document) { _document = document; }

	virtual void document_loaded() { }

	virtual void load(std::string path) {}
	virtual void unload() {}

	// display the content in the main area
	virtual void display(float left, float top, float width, float height) { _left = left; _top = top; _width = width; _height = height; }

	// called on mouse mouve over area
	virtual void mouseMouve(double x_pos, double y_pos) {}

	// called when left button is down
	virtual void left_button_down() {}

	// called when left button is down
	virtual void left_button_up() {}

	// called when left button is down
	virtual void right_button_down() {}

	// called when left button is down
	virtual void right_button_up() {}

	// called when the mouse enter the area
	virtual void enter() {}

	// called when the mouse leave the area
	virtual void leave() {}

	// called when scroll moved
	virtual void scroll(double xdelta, double ydelta) {}

	// called when a key is pressed
	virtual void key_pressed(int key, int scancode, int action, int mods) {}

	// called when a unicode character is entered
	virtual void character_input(unsigned int codepoint) {}

	// called when the mouse drop
	virtual void drop(std::vector<std::string> paths) {}

	// call when render a frame
	virtual void render_frame() {}

	// called to render the user interface
	virtual void render_GUI() {}

	// called to draw module dependent menu
	virtual void render_menu() {}

	// called at application starting
	virtual void initialize_GUI(IniFile& ini) {}

	// called at application ending
	virtual void finalize_GUI(IniFile& ini) {}

	virtual void update() {}

	virtual void undo() {}
	virtual void redo() {}
	virtual void clear_history() {}
	virtual bool can_undo() { return false; }
	virtual bool can_redo() { return false; }

	// selection
	virtual void select_all() {}
	virtual void unselect_all() {}
	virtual void reverse_selection() {}

	// clipboard
	virtual void cut() {}
	virtual void copy() {}
	virtual void paste() {}
	virtual void remove() {}
};