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

#include "mod_log.h"
#include <lang.h>
#include "logger.h"
#include "file.h"
#include "lang.h"
#include <imgui.h>
#include <config.h>
#include <algorithm>

void ModLog::show(bool value)
{
	_show = config.display_log = value;
}

ModLog::ModLog(Window* window) : Module(window)
{
	_index = 2;
	_title = Lang::l("WINDOW_LOG");
	_last_modify = glfwGetTime();
	_code = "MOD_LOG";
	_show = config.display_log;
}

bool ModLog::docked()
{
	return config.log_docked;
}

// read the log file when it changed, once per second
void ModLog::pull()
{
	if ((glfwGetTime() - _last_modify) > 1.0)
	{
		auto time = std::filesystem::last_write_time(std::filesystem::path(Logger::path()));
		if (time > _log_file_time)
		{
			_log_file_time = time;
			_buffer.clear();
			_lineOffsets.clear();
			auto lines = file::read_all_lines(Logger::path());
			if (lines.size() > 0)
			{
				_buffer.append(lines[0].c_str());
				_buffer.append("\n");
				for (int i = 1; i < lines.size(); i++)
				{
					_lineOffsets.push_back(_buffer.size());
					_buffer.append(lines[i].c_str());
					_buffer.append("\n");
				}
			}
			_scrollToBottom = true;
		}

		_last_modify = glfwGetTime();
	}
}

// toolbar and text, in a tab or in the docked panel
void ModLog::draw(bool dock)
{
	if (ImGui::Button(Lang::l("CLEAR")))
	{
		Logger::clear();
		_lineOffsets.clear();
		_last_modify = 0;
	}

	ImGui::SameLine();
	_filter.Draw(Lang::l("FILTER"), dock ? -110.0f : -60.0f);
	if (dock)
	{
		ImGui::SameLine();
		if (ImGui::Button(Lang::l("CLOSE")))
			show(false);
	}
	ImGui::Separator();
	ImGui::BeginChild("scrolling");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

	if (_copy)
		ImGui::LogToClipboard();
	_copy = false;

	_window->push_default_font();

	if (_filter.IsActive())
	{
		const char* buf_begin = _buffer.begin();
		const char* line = buf_begin;
		for (int line_no = 0; line != NULL; line_no++)
		{
			const char* line_end = (line_no < _lineOffsets.Size) ? buf_begin + _lineOffsets[line_no] : NULL;
			if (_filter.PassFilter(line, line_end))
				ImGui::TextUnformatted(line, line_end);
			line = line_end && line_end[1] ? line_end + 1 : NULL;
		}
	}
	else
	{
		ImGui::TextUnformatted(_buffer.begin());
	}

	if (_scrollToBottom)
		ImGui::SetScrollHereY(1.0f);
	_scrollToBottom = false;
	_window->pop_font();
	ImGui::PopStyleVar();
	ImGui::EndChild();
}

void ModLog::render_GUI()
{
	pull();

	ImGui::SetNextWindowSize(ImVec2(_width, _height));
	ImGui::SetNextWindowPos(ImVec2(_left, _top));
	ImGui::Begin("mouse_position", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	draw(false);
	ImGui::End();
}

void ModLog::render_dock(float y, float width, float height)
{
	pull();

	ImGui::SetNextWindowPos(ImVec2(0.0f, y));
	ImGui::SetNextWindowSize(ImVec2(width, height));
	ImGui::Begin("LOG_DOCK", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	// the top edge of the panel resizes it
	ImGui::InvisibleButton("##LOG_SPLITTER", ImVec2(ImGui::GetContentRegionAvail().x, 6.0f));
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	if (ImGui::IsItemActive())
	{
		int limit = (int)(ImGui::GetMainViewport()->Size.y * 0.8f);
		config.log_height = (std::min)(limit, (std::max)(80, config.log_height - (int)ImGui::GetIO().MouseDelta.y));
	}

	draw(true);
	ImGui::End();
}
