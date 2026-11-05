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

#include "shape.h"
#include <anchor.h>
#include <geometry.h>

class Line : public Shape
{
private:
	glm::vec2 _p1 = geometry::vec2_empty;		// starting line coordinates
	glm::vec2 _p2 = geometry::vec2_empty;		// ending line coordinates
	Anchor* _a1 = nullptr;					// anchor linked to _p1
	Anchor* _a2 = nullptr;					// anchor linked to _p2
	Buffer* _buffer = nullptr;				// vertices buffer for drawing

public:
	// properties
	glm::vec2 p1() { return _p1; }
	glm::vec2 p2() { return _p2; }

	void p1(glm::vec2 value);
	void p2(glm::vec2 value);
	void set(glm::vec2 p1, glm::vec2 p2);

	// constructor/destructor
	Line(Renderer* r);
	~Line() override;
	Shape* clone() override;

	void reverse() override;

	glm::vec2 first() override;
	void first(glm::vec2 value) override;
	glm::vec2 last() override;
	void last(glm::vec2 value) override;

	// overrides
	// Shape type
	GraphicType type() override { return GraphicType::Line; }

	// Matrix transformation
	void transform(glm::tmat4x4<float> mat) override;

	// mouse construction / modification
	void move(glm::vec2 point) override;
	bool point(glm::vec2 point) override;

	// ref_point is over shape
	bool is_over(glm::vec2 point) override;

	// return anchor under ref_point
	Anchor* anchor(glm::vec2 point) override;

	// return anchor under ref_point
	virtual std::vector<Anchor*> anchors() override;

	// return magnet points
	std::vector<glm::vec2> magnets() override;

	// construction
	Shape* symmetry(glm::vec2 p1, glm::vec2 p2) override;
	Shape* symmetry(glm::vec2 center) override;

	// display
	void compute() override;
	void update() override;
	void draw() override;
	void draw_anchors() override;
	void properties() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;

}; 