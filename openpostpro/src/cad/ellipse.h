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
#ifndef _ELLIPSE_H
#define _ELLIPSE_H
#include "shape.h"
#include <anchor.h>


class Ellipse : public Shape
{
private:
	glm::vec2 _center = geometry::vec2_empty;	// center coordinates
	float _major = -1.0f;					// major axe length
	float _minor = -1.0f;					// minor axe length
	float _start = 0.0f;					// start angle in rad
	float _stop = 0.0f;						// end angle in rad
	float _angle = 0.0f;					// whole angle orientation following major axe

	Anchor* _a1 = nullptr;					// anchor for center
	Anchor* _a2 = nullptr;					// anchor for major right
	Anchor* _a3 = nullptr;					// anchor for minor top

	std::vector<glm::vec2> _coordinates;
	std::vector<glm::vec3> _vertices;
	Buffer* _buffer = nullptr;

public:
	// properties
	glm::vec2 center() { return _center; }
	float start() { return _start; }
	float stop() { return _stop; }
	float minor() { return _minor; }
	float major() { return _major; }
	float angle() { return _angle; }

	void center(glm::vec2 value);
	void start(float value);
	void stop(float value);
	void minor(float value);
	void major(float value);
	void angle(float value);
	void set(glm::vec2 center, float minor, float major, float start, float stop, float angle);

	// constructor/destructor
	Ellipse(Renderer* r);
	~Ellipse() override;
	Shape* clone() override;

	std::vector<glm::vec2> coordinates();

	void reverse() override;

	// overrides
	// Shape type
	GraphicType type() override { return GraphicType::Ellipse; }

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
	void ui() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;
};

#endif