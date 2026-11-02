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

#pragma once
#ifndef _SPLINE_H
#define _SPLINE_H

#include "shape.h"
#include <anchor.h>
#include <curve.h>

class Spline : public Shape
{
private:
	Curve _curve;									// curve coordinates
	std::vector<glm::vec2> _coordinates;			// array of points to display
	Buffer* _buffer = nullptr;						// vertices buffer for drawing
	bool _transform = false;						// true if a transformation is applied
	int count = 0;
	bool _move = false;

	Buffer* _untrim_buffer = nullptr;				// vertices buffer for drawing untrim - TEST
	int _primitive = 0;
	std::vector<int> _indices;

	Buffer* _inter_buffer = nullptr;				// vertices buffer for drawing intersection spots - TEST
	std::vector<int> _inter_indices;
	std::vector<glm::vec2> _inter_points;
	
	float _current_offset = 0;

public:
	void add(glm::vec2 point);
	void add(glm::vec2 point, glm::vec2 center, float radius, bool cw, SegmentType type = SegmentType::Arc);
	void add(glm::vec2 center, float radius, bool cw, SegmentType type = SegmentType::Arc);
	void add(Curve& c);

	size_t size() { return _curve.size(); }

	Curve& curve() { return _curve; }

	void close();

	// constructor/destructor
	Spline(Renderer* r);
	~Spline() override;
	Shape* clone() override;

	std::vector<glm::vec2> coordinates();

	void reverse() override;

	glm::vec2 first() override;

	glm::vec2 last() override;

	// overrides
	// Shape type
	GraphicType type() override { return GraphicType::Spline; }

	// Matrix transformation
	void transform(glm::tmat4x4<float> mat) override;

	// ref_point is over shape
	bool is_over(glm::vec2 point) override;

	// return magnet points
	std::vector<glm::vec2> magnets() override;

	void scaled() override;

	// display
	void compute() override;
	void update() override;
	void draw() override;
	void ui() override;

	// construction
	Shape* symmetry(glm::vec2 p1, glm::vec2 p2) override;
	Shape* symmetry(glm::vec2 center) override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;


	// TEST
	void offset(float o);
};

#endif