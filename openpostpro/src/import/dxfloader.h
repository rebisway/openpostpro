// OpenPostPro - www.openpostpro.org
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
	static int _count;
	static std::vector<Shape*> add_entity(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_line(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_point(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_circle(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_arc(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_ellipse(DxfEntity* e, Renderer* r);
	static std::vector<Shape*> add_polyline(DxfEntity* e, Renderer* r, std::string name);
	static Shape* add_arc(glm::vec2 p1, glm::vec2 p2, float bulge, DxfEntity* e, Renderer* r, std::string name);
public:
	//static void callback(void (*func)(int count)) { _callback = func; }
	static void read(std::string path, Document* document);
};