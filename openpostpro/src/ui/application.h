// OpenPostPro - www.openpostpro.org
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

	bool _display_preferences = false;
	bool _display_postpro_preferences = false;

	std::string _application_title = "";

	std::vector<Postpro*> _postpros;
	Postpro* _current_postpro = nullptr;

	std::vector<CadScript*> _scripts;

public:
	Application(std::string title);
	~Application();

	void initialize() override;
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

	void new_document();
	void load_file(std::string path);
	void save_file(std::string path);
	void load_dxf(std::string path);

	Module* module(std::string code);

	void insert(std::string content);
};

