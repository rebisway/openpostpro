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

// Test bridge: drives the running application from a text file and takes screenshots, to test the interface without a person
// (mouse, keys, screenshots of what is really drawn).
//
// It exists in the Debug build only, and is active only when the environment variable OPP_BRIDGE holds a folder:
//   - commands are read from <folder>/commands.txt, one per line (the file is then renamed commands.done), '#' starts a comment;
//   - what happens is written to <folder>/bridge.log, and <folder>/bridge.status holds "busy" or "idle <number of commands done>".
//
// Commands:
//   wait <frames>                     wait that number of drawn frames
//   size <width> <height>             size of the window
//   move <x> <y>                      mouse position in the window (pixels)
//   down [left|right]  /  up [left|right]
//   click <x> <y> [left|right]        move, press, release
//   drag <x1> <y1> <x2> <y2> [left|right]   press at the first point, move to the second, release
//   scroll <dy>                       mouse wheel
//   key <name> [shift|ctrl|alt]       a key press and release: a to z, 0 to 9, enter, escape, space, tab, delete, left, right, up, down, f1 to f12
//   type <text>                       characters typed in the field which has the focus
//   screenshot <file.png>             the picture of the window, saved in the folder
//   log <text>                        a line in the log of the bridge
//   quit                              closes the application
// Any other command is given to the application (Application::bridge_command), for example "module sim" or "sim play".

#pragma once
#ifndef _BRIDGE_H
#define _BRIDGE_H

#include <functional>
#include <string>
#include <vector>

class Window;

namespace bridge
{
	// Set by the application: runs a command which is not one of the generic ones. Returns false if it does not know it.
	extern std::function<bool(const std::vector<std::string>&)> application;

	// Writes a line in the log of the bridge (does nothing when the bridge is not active)
	void log(const std::string& text);

	// Called by the main loop before the interface frame is built, and after it is drawn (before the buffers are swapped)
	void poll(Window* window);
	void after_render(Window* window);
}

#endif
