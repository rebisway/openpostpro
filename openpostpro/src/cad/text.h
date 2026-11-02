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
#ifndef _TEXT_H
#define _TEXT_H

#include "shape.h"
#include <anchor.h>
#include <font.h>

enum class TextPos
{
	Up,
	Middle,
	Down
};


class Text : public Shape
{
private:
	glm::vec2 _p1 = geometry::vec2_empty;			// coordinates
	glm::vec2 _pt = geometry::vec2_empty;			// coordinates trasformed when text is linked to another shape
	std::vector<std::vector<glm::vec2>> _origin;	// hold letters coordinates before transformation
	std::vector<std::vector<glm::vec2>> _transf;	// hold letters coordinates after transformation
	std::vector<glm::vec3> _vertices;				// hold diplayed coordinates, after transformation
	std::vector<int> _indices;						// array that contains indices of polyline for each character in _vertices
	Anchor* _a1 = nullptr;							// anchor on _p1
	Buffer* _buffer = nullptr;						// vertices buffer for drawing
	std::string _text;								// text to draw
	std::string _family;							// font family name
	bool _italic = false;							// italic font
	bool _bold = false;								// bold font
	float _height = 0.0f;							// max for heighest character
	Font* _font = nullptr;							// hold font object
	float _inter = 0.0f;							// distance between characters, default is 0.0f
	std::vector<int> _chars;						//
	std::string _reference_name;
	int _reference = -1;							// reference shape, we will look for all connected shape to reference
	std::vector<glm::vec2> _reference_path;			// hold reference path coordinates
	float _reference_length = 0.0f;					// reference length
	float _reference_pos = 0.0f;					// position of the first letter on reference shape
	bool _reference_closed = false;					// if true, the reference polyline is considered closed
	glm::vec2 _reference_point = geometry::vec2_empty;
	TextPos _position = TextPos::Middle;			// when sticked to another shape, position of text along the reference shape
	bool _mirror;									// reverse the text
	bool _update_origin = false;					// true if it needs to compute outline coordinates

public:
	//properties
	glm::vec2 p1() { return _p1; }
	void p1(glm::vec2 value);
	void text(std::string value);
	std::string text() { return _text; }
	void fontFamily(std::string value);
	std::string fontFamily() { return _family; }
	void italic(bool value);
	bool italic() { return _italic; }
	void bold(bool value);
	bool bold() { return _bold; }
	void inter(float value);
	float inter() { return _inter; }
	void height(float value);
	float height() { return _height; }
	void position(TextPos value);
	TextPos position() { return _position; }
	void mirror(bool value);
	bool mirro() { return _mirror; }
	void reference(int value);
	int reference() { return _reference; }
	void reference_path(std::vector<glm::vec2> path, bool closed);
	void reference_pos(float value);
	float reference_pos() { return _reference_pos; }
	void set(std::string text, std::string family, bool italic, bool bold, float height);

	// constructor
	Text(Renderer* r);
	~Text() override;
	Shape* clone() override;

	std::vector<std::vector<glm::vec2>> coordinates();

	// overrides
	// Shape type
	GraphicType type() override { return GraphicType::Text; }

	// Matrix transformation
	void transform(glm::tmat4x4<float> mat) override;
	//void transform(glm::vec2 ref_point) override;

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
	std::vector<glm::vec2> magnets() override { return std::vector<glm::vec2>({ _a1->point() }); }

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