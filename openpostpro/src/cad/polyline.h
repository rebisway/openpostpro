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
#ifndef _POLYLINE_H
#define _POLYLINE_H

#include "shape.h"
#include <anchor.h>

enum class PolylineMode
{
	Polyline,
	BSpline,
	NaturalCubic,
	lagrange,
	Catmull,
	Bezier
};

class Polyline : public Shape
{
private:
	std::vector<glm::vec2> _points;					// array of control points
	std::vector<glm::vec2> _coordinates;			// array of points to display
	std::vector<Anchor*> _anchors;					// array of anchors
	Buffer* _buffer = nullptr;						// vertices buffer for drawing
	PolylineMode _mode = PolylineMode::Polyline;	// type of polyline to display
	bool _transform = false;						// true if a transformation is applied
	int count = 0;
	bool _move = false;

public:
	//properties
	std::vector<glm::vec2> points() { return _points; }
	void points(std::vector<glm::vec2> points);
	// during the creation: the last placed point (the one before the moving point)
	bool last_placed(glm::vec2& value);
	// during the creation: removes the last placed points which are on the previous one (a point placed twice at the same place)
	void remove_repeated_last();
	PolylineMode mode() { return _mode; }
	void mode(PolylineMode value) { if (value != _mode) { _mode = value; if (_points.size() > 2) update(); } }

	// constructor/destructor
	Polyline(Renderer* r);
	~Polyline() override;
	Shape* clone() override;

	std::vector<glm::vec2> coordinates();

	void reverse() override;

	glm::vec2 first() override;
	void first(glm::vec2 value) override;
	glm::vec2 last() override;
	void last(glm::vec2 value) override;

	// overrides
	// Shape type
	GraphicType type() override { return GraphicType::Polyline; }

	// Matrix transformation
	void transform(glm::tmat4x4<float> mat) override;

	// mouse construction / modification
	void move(glm::vec2 point) override;
	bool point(glm::vec2 point) override;
	void done(bool value) override;
	bool done() override;

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