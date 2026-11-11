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
#include "window.h"
#include "lang.h"
#include <imgui.h>
#include "module.h"
#include <renderer_opengl.h>
#include <mutex>
#include <postpro.h>
#include <vector>
#include <cad_script.h>

class Application :
	public Window
{
private:
	float _version = 0.1f;

	int _xpos = 0;
	int _ypos = 0;
	int _menu_height = 0;
	std::vector<Module*> _modules;
	Module* _current_module = NULL;
	float _nav_width = 56.0f;

	int _nav_style = 0;

	ImFont* _font = NULL;

	Document* _document = NULL;

	std::mutex _mutex;
	std::mutex _move_mutex;
	bool _initialized = false;
	std::string _imgui_ini;

	glm::vec2 _top_left = glm::vec2();
	glm::vec2 _bottom_right = glm::vec2();

	Texture* _splash = nullptr;				// the image of the start, drawn while the application loads
	bool _splash_on = true;
	Texture* _about_icon = nullptr;			// the icon shown in the About window, loaded when the window is first displayed
	bool _display_preferences = false;
	bool _display_first_run = false;		// language and unit are asked at the very first start
	std::string _first_language = "";
	int _first_unit = 0;
	bool _display_postpro_preferences = false;

	std::string _application_title = "";

	std::vector<Postpro*> _postpros;
	Postpro* _current_postpro = nullptr;

	std::vector<CadScript*> _scripts;
	bool _display_clone = false;			// dialog box to clone the current post-processor
	std::string _clone_name;
	std::string _clone_description;
	std::string _clone_error;			// key of the error text
	std::string _clone_source;			// file to clone
	bool _clone_script = false;			// true for a cad script, false for a post-processor
	bool _save_requested = false;		// CTRL+S pressed, handled when the interface is drawn
	bool _new_requested = false;		// CTRL+N pressed
	bool _open_requested = false;		// CTRL+O pressed
	bool _show_shortcuts = false;		// the window of the keyboard shortcuts is displayed
	CadScript* _current_script = nullptr;	// script whose properties dialog box is displayed

public:
	Application(std::string title);
	~Application();

	void initialize() override;

	// draws one frame of the start screen with the message (the application is loading and does not run its loop yet). The events of the window
	// are not read: the callbacks need the modules, which do not exist yet
	void splash(const std::string& message);
	void splash_end();
	void finalize() override;

	void onSizeChanged(float width, float height) override;
	void onMouseMove(double xpos, double ypos) override;
	void onLeftButtonDown() override;
	void onLeftButtonUp() override;
	void onRightButtonDown() override;
	void onRightButtonUp() override;
	void onScroll(double xoffset, double yoffset) override;
	void onEnter() override;
	void onLeave() override;
	void onKeyPressed(int key, int scancode, int action, int mods) override;
	void onCharacterInput(unsigned int codepoint) override;
	void onDrop(std::vector<std::string> paths) override;
	void onRenderFrame() override;
	void onRenderGUI() override;

	void render_menu();
	bool _show_about = false;		// the About window is displayed

	Postpro* load_postpro(const std::string& file);	// load a post-processor file, null if it fails
	CadScript* load_cad_script(const std::string& file);	// load a cad script file, null if it fails
	std::string clone();							// clone the file _clone_source, returns the key of the error text if it fails
	void save_current();							// save the file edited by the current module, or the document

	void reload_script(const std::string& path);	// use again a script after its compilation

	void new_document();
	// remember: the file is added to the recent documents (not for an example, which is a copy to work on)
	void load_file(std::string path, bool remember = true);
	void open_dialog();								// the dialog box to choose a document, then its loading
	void add_recent(const std::string& path);
	void save_file(std::string path);
	void load_dxf(std::string path);
	void export_dxf(std::string path);

	Module* module(std::string code);

	void insert(std::string content);
};

