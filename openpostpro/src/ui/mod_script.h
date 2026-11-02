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

#ifndef _MODSCRIPT_H
#define _MODSCRIPT_H

#include "module.h"
#include <filesystem>

#include "window.h"
#include <IMGUI_third/ImGuiColorTextEdit/TextEditor.h>

class ModScript : public Module
{
private:
	std::filesystem::file_time_type _output_file_time;
	TextEditor editor;

	double _last_modify;
	bool _modified = false;
	std::string _path;
	std::string _data;

public:
	bool modified() { return _modified; }
	std::string path() { return _path; }
	std::string data() { return _data; }

	ModScript(Window* window);

	void document_loaded() override;

	void load(std::string path) override;
	void unload() override;

	// undo/redo
	void undo() override;
	void redo() override;
	void clear_history() override;
	bool can_undo() override;
	bool can_redo() override;

	// selection
	void select_all() override;
	void unselect_all() override;

	// clipboard
	void cut() override;
	void copy() override;
	void paste() override;
	void remove() override;

	void render_GUI() override;
};

#endif