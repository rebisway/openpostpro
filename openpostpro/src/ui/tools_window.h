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
#include <tool.h>

// The window of the tool table (menu Workshop): tools are created, edited and deleted here, the table is saved at each change.
class ToolsWindow
{
private:
	bool _open = false;
	int _selected = -1;			// number of the tool being edited
	int _new_number = 0;		// number typed in the edit box
	int _material = 0;			// material of the computation of the feeds (see feeds.h)
	bool _rpm_limited = false;	// the last computation was limited by the maximum speed
	float _max_rpm = 24000;		// the spindle of the machine
	int _edited = -2;			// the tool for which _new_number was set

public:
	void show() { _open = true; }
	bool visible() { return _open; }
	void draw();
};
