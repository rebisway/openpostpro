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

// A camera in perspective which turns around a point (the z axis is up), for the scenes in 3D.

#pragma once
#ifndef ENGINE_ORBIT_CAMERA_H
#define ENGINE_ORBIT_CAMERA_H

#include "camera.h"

class OrbitCamera : public Camera
{
private:
	glm::vec3 _target = glm::vec3(0.0f);
	float _distance = 100.0f;
	float _yaw = -1.05f;			// radians, around the vertical axis (0 looks from the +x side)
	float _pitch = 0.7f;			// radians above the horizon
	float _fov = 0.7854f;			// 45 degrees, vertical
	float _aspect = 1.0f;
	float _radius = 100.0f;			// size of what is looked at, for the clipping planes

public:
	OrbitCamera();

	void set_size(float width, float height) override;

	// the camera looks at the box (the corners are in the coordinates of the scene), from the current direction
	void fit(const glm::vec3& low, const glm::vec3& high);
	// the directions of the views: 0 top, 1 front, 2 right side, 3 isometric
	void view(int preset);

	// the mouse: a movement in pixels turns the camera, moves the point looked at, or zooms
	void rotate(float dx, float dy);
	void pan(float dx, float dy);
	void zoom(float wheel);

	glm::vec3 eye() const;
	glm::vec3 target() const { return _target; }
	float distance() const { return _distance; }
	float yaw() const { return _yaw; }
	float pitch() const { return _pitch; }

	void compute();
};

#endif
