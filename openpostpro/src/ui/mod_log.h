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
#include "window.h"
#include <filesystem>

class ModLog : public Module
{
private:
	ImGuiTextBuffer _buffer;
	ImGuiTextFilter _filter;
	ImVector<int> _lineOffsets;
	bool _scrollToBottom = false;
	bool _copy = false;
	std::filesystem::file_time_type _log_file_time;

	double _last_modify;

	void pull();
	void draw(bool dock);

public:
	bool docked() override;
	void render_dock(float y, float width, float height);

	using Module::show;		// the override of show(bool) hides the getter
	void show(bool value) override;

	ModLog(Window* window);
	void render_GUI() override;
};