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

/************************************************************************
* Write the drawing of a document into a DXF file (AutoCAD R12, ASCII).
*
* R12 is the most widely read version. It has no ellipse and no spline entity: the ellipses and the
* polylines with a curve mode are written as polylines of short segments. A spline made of lines and
* arcs (the shapes of the application) is written as one polyline with bulges, so its arcs stay exact.
* The toolpaths are not exported, only the shapes. Texts follow the placement of their position:
* a text along another shape is written as outlines.
*************************************************************************/

#pragma once
#include <string>
#include <vector>
#include <set>
#include <document.h>

class DxfExport
{
public:
	// the DXF text of the shapes of all the visible layers, or only of the selected shapes if there are some
	static std::string to_string(Document* document, bool selection_only = false);

	// the same from the layers: the shapes of the selection are the only ones written, all the shapes if it is empty
	static std::string to_string(const std::vector<Layer*>& layers, Unit unit, const std::set<Graphic*>& selection = std::set<Graphic*>());

	// writes the file, false if it can not be written
	static bool write(Document* document, const std::string& path, bool selection_only = false);

	// the name of a layer in a DXF file: letters, digits, $, - and _ only
	static std::string layer_name(const std::string& name);

	// index (1 to 9) of the nearest AutoCAD color of an RGB color
	static int aci_color(glm::vec4 color);
};
