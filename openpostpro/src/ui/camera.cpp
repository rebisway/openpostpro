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

#include "camera.h"

Camera::Camera()
{
	reset();
}

void Camera::reset()
{
	_scale = 1.0f;
	_znear = 1.0f;
	_zfar = 0.0f;
	_xcenter = 0.0f;
	_ycenter = 0.0f;
	_left_down = false;
	_right_down = false;
	_modelview = glm::mat4(1.0f);
	_projection = glm::mat4(1.0f);
}