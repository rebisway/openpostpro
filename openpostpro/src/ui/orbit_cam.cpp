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

#include "orbit_cam.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

OrbitCamera::OrbitCamera()
{
	compute();
}

void OrbitCamera::set_size(float width, float height)
{
	Camera::set_size(width, height);
	_aspect = height > 0.0f ? width / height : 1.0f;
	compute();
}

glm::vec3 OrbitCamera::eye() const
{
	return _target + _distance * glm::vec3(std::cos(_pitch) * std::cos(_yaw), std::cos(_pitch) * std::sin(_yaw), std::sin(_pitch));
}

void OrbitCamera::compute()
{
	// the planes follow the size of the scene and the distance, so the depth buffer keeps its precision
	_znear = std::max(0.05f, _distance * 0.02f);
	_zfar = _distance + _radius * 4.0f + 10.0f;
	modelview(glm::lookAt(eye(), _target, glm::vec3(0.0f, 0.0f, 1.0f)));
	projection(glm::perspective(_fov, _aspect, _znear, _zfar));
}

void OrbitCamera::fit(const glm::vec3& low, const glm::vec3& high)
{
	_target = (low + high) * 0.5f;
	_radius = std::max(1.0f, glm::length(high - low) * 0.5f);

	// the distance at which the eight corners of the box are all in the field of view, from the present direction
	glm::vec3 forward = glm::normalize(-glm::vec3(std::cos(_pitch) * std::cos(_yaw), std::cos(_pitch) * std::sin(_yaw), std::sin(_pitch)));
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
	glm::vec3 up = glm::cross(right, forward);
	float tan_v = std::tan(_fov * 0.5f);
	float tan_h = tan_v * std::max(_aspect, 0.1f);

	float distance = 1.0f;
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 corner((i & 1) ? high.x : low.x, (i & 2) ? high.y : low.y, (i & 4) ? high.z : low.z);
		glm::vec3 d = corner - _target;
		float depth = glm::dot(d, forward);		// positive when the corner is farther than the target
		distance = std::max(distance, std::fabs(glm::dot(d, right)) / tan_h - depth);
		distance = std::max(distance, std::fabs(glm::dot(d, up)) / tan_v - depth);
	}
	_distance = distance * 1.08f + 1.0f;
	compute();
}

void OrbitCamera::view(int preset)
{
	switch (preset)
	{
	case 0: _yaw = -1.5708f; _pitch = 1.5607f; break;		// top (a little short of the vertical, which has no direction)
	case 1: _yaw = -1.5708f; _pitch = 0.0f; break;			// front
	case 2: _yaw = 0.0f; _pitch = 0.0f; break;				// right side
	default: _yaw = -1.05f; _pitch = 0.7f; break;			// isometric
	}
	compute();
}

void OrbitCamera::rotate(float dx, float dy)
{
	_yaw -= dx * 0.008f;
	_pitch = std::clamp(_pitch + dy * 0.008f, -1.5607f, 1.5607f);
	compute();
}

void OrbitCamera::pan(float dx, float dy)
{
	// a movement of the mouse moves the scene by the same distance on the screen
	float world_per_pixel = 2.0f * _distance * std::tan(_fov * 0.5f) / std::max(_height, 1.0f);
	glm::vec3 forward = glm::normalize(_target - eye());
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
	glm::vec3 up = glm::cross(right, forward);
	_target += (-right * dx + up * dy) * world_per_pixel;
	compute();
}

void OrbitCamera::zoom(float wheel)
{
	_distance = std::clamp(_distance * std::pow(0.88f, wheel), 0.5f, 100000.0f);
	compute();
}
