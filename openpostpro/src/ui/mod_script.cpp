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

#include "mod_script.h"
#include <lang.h>
#include "logger.h"
#include "file.h"
#include "lang.h"
#include <imgui.h>
#include <config.h>

ModScript::ModScript(Window* window) : Module(window)
{
	_index = 1;
	_title = "";
	_code = "MOD_SCRIPT";
	_last_modify = 0;
	_show = false;

	editor.SetShowWhitespaces(false);
	editor.SetColorizerEnable(true);

	//auto lang = TextEditor::LanguageDefinition::LanguageDefinition();
	//lang.mName = "GCODE";
	//lang.mAutoIndentation = false;
	//lang.mCaseSensitive = false;
	//lang.mCommentStart = "(";
	//lang.mCommentEnd = ")";
	//lang.mKeywords.insert("G0");
	//lang.mKeywords.insert("G1");
	//lang.mKeywords.insert("G2");
	//lang.mKeywords.insert("G3");
	//lang.mKeywords.insert("G4");
	//lang.mKeywords.insert("X");
	//lang.mKeywords.insert("Y");
	//lang.mKeywords.insert("Z");
	//lang.mKeywords.insert("I");
	//lang.mKeywords.insert("J");
	//lang.mKeywords.insert("K");

	//editor.SetLanguageDefinition(lang);
}

void ModScript::document_loaded()
{
	_last_modify = 0;
}

void ModScript::load(std::string path)
{
	if (std::filesystem::exists(path))
	{
		_path = path;
		_title = std::filesystem::path(_path).filename().string();
	}
	else
	{
		_path = _data = _title = "";
	}
}

void ModScript::unload()
{
	_title = "";
}

void ModScript::undo()
{
	editor.Undo();
}

void ModScript::redo()
{
	editor.Redo();
}

void ModScript::clear_history()
{
	editor.SetText("");
}

bool ModScript::can_undo()
{
	return editor.CanUndo();
}

bool ModScript::can_redo()
{
	return editor.CanRedo();
}

void ModScript::select_all()
{
	editor.SelectAll();
}

void ModScript::unselect_all()
{
	editor.SetSelectionStart(TextEditor::Coordinates());
	editor.SetSelectionEnd(TextEditor::Coordinates());
}

void ModScript::cut()
{
	editor.Cut();
}

void ModScript::copy()
{
	editor.Copy();
}

void ModScript::paste()
{
	editor.Paste();
}

void ModScript::remove()
{
	editor.Delete();
}

void ModScript::render_GUI()
{
	if (_document == NULL)
		return;

	if (_path != "" && std::filesystem::exists(_path) && (glfwGetTime() - _last_modify) > 1.0) // pull every seconds
	{
		auto time = std::filesystem::last_write_time(_path);
		if (time > _output_file_time)
		{
			_output_file_time = time;
			_data = file::read_all_text(_path);

			editor.SetText(_data);
			editor.SetPalette(config.display_style == 2 ? TextEditor::GetLightPalette() : TextEditor::GetDarkPalette());

			Logger::log("File [" + _path + "] loaded");
		}

		_last_modify = glfwGetTime();
	}

	ImGui::SetNextWindowPos(ImVec2(_left, _top));
	ImGui::SetNextWindowSize(ImVec2(_width, _height));
	
	ImGui::Begin("MOD_SCRIPT_WINDOW", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	ImGui::BeginChild("MOD_SCRIPT_SCROLLING");

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
	_window->push_default_font();

	editor.Render("MOD_SCRIPT_SCROLLING_EDITOR");

	_window->pop_font();
	ImGui::PopStyleVar();

	ImGui::EndChild();
	ImGui::End();

	if (_modified = editor.IsTextChanged())
	{
		_title = "*" + std::filesystem::path(_path).filename().string();
	}
}