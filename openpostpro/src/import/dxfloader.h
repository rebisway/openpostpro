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
* Convert Dxf data into OpenPostPro dbjects
*************************************************************************/

#pragma once
#include <string>
#include <document.h>
#include "dxf.h"

class DxfLoader
{
public:
	// a shape created from the file and the name of the layer it belongs to
	struct Placed
	{
		Shape* shape;
		std::string layer;
	};

	static int _count;
	static std::vector<Shape*> add_entity(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_line(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_point(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_circle(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_arc(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_ellipse(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_polyline(DxfEntity* e, Renderer* r, std::string name);
	static std::vector<Shape*> add_text(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_mtext(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_spline(DxfEntity* e, Renderer* r);
	// the shapes of the block of an INSERT, placed by its position, scale and rotation. The entities on the layer "0" take the layer of the INSERT
	static void add_insert(Dxf& dxf, DxfEntity* e, Renderer* r, glm::mat4 parent, std::string layer, int depth, std::vector<Placed>& result);
	// the text of a DXF text: %%d, %%p, %%c are replaced, underline and overline codes are dropped
	static std::string plain_text(std::string value);
	// the text of an MTEXT without its formatting codes (fonts, heights, colors, stacking, line breaks which become a space)
	static std::string mtext_content(std::string value);
	// the points of a B-spline (NURBS if there are weights) from its control points
	static std::vector<glm::vec2> spline_points(int degree, const std::vector<glm::vec2>& control, std::vector<float> knots, const std::vector<float>& weights);
	static Shape* add_arc(glm::vec2 p1, glm::vec2 p2, float bulge, DxfEntity* e, Renderer* r, std::string name);
public:
	//static void callback(void (*func)(int count)) { _callback = func; }
	static void read(std::string path, Document* document);
};