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

#include "mod_output.h"
#include <lang.h>
#include "logger.h"
#include "file.h"
#include "lang.h"
#include <imgui.h>
#include <config.h>

ModOutput::ModOutput(Window* window) : Module(window)
{
	_index = 1;
	_title = Lang::l("WINDOW_OUTPUT");
	_code = "MOD_OUTPUT";
	_last_modify = 0;
	_show = config.display_output;

	editor.SetShowWhitespaces(false);
	editor.SetColorizerEnable(true);

	auto lang = TextEditor::LanguageDefinition::LanguageDefinition();
	lang.mName = "GCODE";
	lang.mAutoIndentation = false;
	lang.mCaseSensitive = false;
	lang.mCommentStart = "(";
	lang.mCommentEnd = ")";
	lang.mKeywords.insert("G0");
	lang.mKeywords.insert("G1");
	lang.mKeywords.insert("G2");
	lang.mKeywords.insert("G3");
	lang.mKeywords.insert("G4");
	lang.mKeywords.insert("X");
	lang.mKeywords.insert("Y");
	lang.mKeywords.insert("Z");
	lang.mKeywords.insert("I");
	lang.mKeywords.insert("J");
	lang.mKeywords.insert("K");

	editor.SetLanguageDefinition(lang);
}

void ModOutput::show(bool value)
{
	_show = config.display_output = value;
}

void ModOutput::document_loaded()
{
	_last_modify = 0;
}

void ModOutput::undo()
{
	editor.Undo();
}

void ModOutput::redo()
{
	editor.Redo();
}

void ModOutput::clear_history()
{
	editor.SetText("");
}

bool ModOutput::can_undo()
{
	return editor.CanUndo();
}

bool ModOutput::can_redo()
{
	return editor.CanRedo();
}

void ModOutput::select_all()
{
	editor.SelectAll();
}

void ModOutput::unselect_all()
{
	editor.SetSelectionStart(TextEditor::Coordinates());
	editor.SetSelectionEnd(TextEditor::Coordinates());
}

void ModOutput::cut()
{
	editor.Cut();
}

void ModOutput::copy()
{
	editor.Copy();
}

void ModOutput::paste()
{
	editor.Paste();
}

void ModOutput::remove()
{
	editor.Delete();
}

void ModOutput::render_GUI()
{
	if (_document == NULL)
		return;

	if (std::filesystem::exists(_document->output()))
	{

		if ((glfwGetTime() - _last_modify) > 1.0) // pull every seconds
		{
			auto time = std::filesystem::last_write_time(_document->output());
			if (time > _output_file_time)
			{
				_output_file_time = time;
				auto text = file::read_all_text(_document->output());

				editor.SetText(text);
				editor.SetPalette(config.display_style == 2 ? TextEditor::GetLightPalette() : TextEditor::GetDarkPalette());

				_title = std::filesystem::path(_document->output()).filename().string();
			}

			_last_modify = glfwGetTime();
		}

		ImGui::SetNextWindowPos(ImVec2(_left, _top));
		ImGui::SetNextWindowSize(ImVec2(_width, _height));
		ImGui::Begin("MOD_OUTPUT_WINDOW", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
		ImGui::BeginChild("MOD_OUTPUT_SCROLLING");

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		_window->push_default_font();

		editor.Render("MOD_OUTPUT_EDITOR");

		_window->pop_font();
		ImGui::PopStyleVar();

		ImGui::EndChild();
		ImGui::End();

		if (_modified = editor.IsTextChanged())
		{
			_title = "*" + std::filesystem::path(_document->output()).filename().string();
		}
	}
}