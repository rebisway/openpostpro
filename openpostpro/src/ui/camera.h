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
#ifndef ENGINE_CAMARA_H
#define ENGINE_CAMARA_H

#include <glm/glm.hpp>

class Camera
{
protected:
	glm::mat4 _modelview;
	glm::mat4 _projection;
	float _scale;
	float _width;
	float _height;
	float _znear;
	float _zfar;
	float _xcenter;		// screen coordinate of (0,0)
	float _ycenter;		// screen coordinate of (0,0)
	float _xpos;		// mouse coordinate in screen coordinate
	float _ypos;		// mouse coordinate in screen coordinate
	float _xdelta;
	float _ydelta;
	bool _left_down;
	bool _right_down;

public:
	Camera();
	~Camera() {}
	glm::mat4 modelview() { return _modelview; }
	glm::mat4 projection() { return _projection; }
	float znear() { return _znear; }
	float zfar() { return _zfar; }
	float scale() { return _scale; }
	float width() { return _width; }
	float height() { return _height; }
	float xcenter() { return _xcenter; }
	float ycenter() { return _ycenter; }
	float xpos() { return _xpos; }
	float ypos() { return _ypos; }
	float xdelta() { return _xdelta; }
	float ydelta() { return _ydelta; }
	float leftButton() { return _left_down; }
	float rightButton() { return _right_down; }

	void modelview(glm::mat4 matrix) { _modelview = matrix; }
	void projection(glm::mat4 matrix) { _projection = matrix; }
	void znear(float n) { _znear = n; }
	void zfar(float n) { _zfar = n; }
	void scale(float n) { _scale = n; }
	void width(float n) { _width = n; }
	void height(float n) { _height = n; }
	void xcenter(float n) { _xcenter = n; }
	void ycenter(float n) { _ycenter = n; }
	void xpos(float n) { _xpos = n; }
	void ypos(float n) { _ypos = n; }
	void xdelta(float n) { _xdelta = n; }
	void ydelta(float n) { _ydelta = n; }

	void reset();

	virtual void mouseMove(float xpos, float ypos) { _xpos = xpos; _ypos = ypos; }
	virtual void mouseWheel(float xdelta, float ydelta) { _xdelta = xdelta; _ydelta = ydelta; }
	void mouseLeftDown() { _left_down = true; }
	void mouseLeftUp() { _left_down = false; }
	void mouseRightDown() { _right_down = true; }
	void mouseRightUp() { _right_down = false; }

	virtual void set_size(float width, float height) { _width = width; _height = height; }
	virtual void center() {}

	virtual glm::vec2 getPosition(float x, float y) { return glm::vec2(); }
	glm::vec2 getPosition(glm::vec2 mouse) { return getPosition(mouse.x, mouse.y); }
};

#endif