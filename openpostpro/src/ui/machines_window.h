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
#include <machine.h>

class Document;

// The window of the machines (menu Workshop): the table of the machines of the user, and the machine of the document. A machine gives
// the area of the table, the origin and the direction of the axes: the drawing shows the area, and the post-processor and the
// simulator follow the direction of the axes.
class MachinesWindow
{
private:
	bool _open = false;
	std::string _selected;		// name of the machine being edited

public:
	void show() { _open = true; }
	bool visible() { return _open; }
	void draw(Document* document);
};
