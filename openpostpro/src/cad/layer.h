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
#include <string>
#include <vector>
#include "shape.h"
#include "graphic.h"

class Document;

class Layer : public Graphic
{
private:
	std::vector<Shape*> _shapes;
	glm::vec4 _color;
	bool _visible = true;
	// a locked layer can not be modified: its shapes can not be moved, edited, deleted, cut or extended. They stay visible,
	// can be selected (to machine them) and the magnets work on them
	bool _locked = false;
	// the document the layer belongs to: it is told when a shape enters or leaves, to keep its table of the blocks
	Document* _document = nullptr;

public:
	std::vector<Shape*>& shapes() { return _shapes; }
	void document(Document* value) { _document = value; }
	glm::vec4 color() { return _color; }
	void color(glm::vec4 color) { _color = color; }
	bool visible() { return _visible; }
	void visible(bool value) { _visible = value; }
	bool locked() { return _locked; }
	void locked(bool value) { _locked = value; }
	GraphicType type() override { return GraphicType::Layer; }

	void add(Shape* value);
	void remove(Shape* value);
	void remove(std::string name);
	void remove(unsigned int id);

	Shape* shape(std::string name);
	Shape* shape(unsigned int id);

	Layer(Renderer* r);
	~Layer();

	void reset(Renderer* r) override;
	void scaled() override;
	void draw() override;
	void ui() override;
	void draw_anchors() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;
};