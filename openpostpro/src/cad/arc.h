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
#ifndef _ARC_H
#define _ARC_H

#include "shape.h"
#include <anchor.h>

enum class ArcMode
{
	Center,
	ThreePoints
};

class Arc : public Shape
{
private:
	glm::vec2 _start = geometry::vec2_empty;
	glm::vec2 _center = geometry::vec2_empty;
	glm::vec2 _stop = geometry::vec2_empty;
	bool _cw = true;
	float _radius = 10.0f;
	int _move_anchor = 0;
	ArcMode _mode = ArcMode::Center;
	int _points_count = 0;


	Anchor* _a1 = nullptr;
	Anchor* _a2 = nullptr;
	Anchor* _a3 = nullptr;

	std::vector<glm::vec3> _vertices;
	Buffer* _buffer = nullptr;

	// test for rectangle computation
	 //Buffer* _test = nullptr;

public:
	// properties
	glm::vec2 start() { return _start; }
	glm::vec2 center() { return _center; }
	glm::vec2 stop() { return _stop; }
	bool cw() { return _cw; }
	float radius() { return _radius; }
	float start_angle() { return geometry::oriented_angle(_start, _center); }
	float stop_angle() { return geometry::oriented_angle(_stop, _center); }

	void start(glm::vec2 value);
	void center(glm::vec2 value);
	void stop(glm::vec2 value);
	void cw(bool value);
	void set(glm::vec2 s, glm::vec2 c, glm::vec2 d, bool cw);

	// constructor/destructor
	Arc(Renderer* r);
	~Arc() override;
	Shape* clone() override;

	ArcMode mode() { return _mode; }
	void mode(ArcMode value) { _mode = value; }

	// overrides
	
	void reverse() override;

	glm::vec2 first() override;
	void first(glm::vec2 value) override;
	glm::vec2 last() override;
	void last(glm::vec2 value) override;

	// Shape type
	GraphicType type() override { return GraphicType::Arc; }

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

#endif