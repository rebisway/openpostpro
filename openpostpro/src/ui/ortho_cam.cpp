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

#include "ortho_cam.h"
#include <glm/ext.hpp>


OrthoCamera::OrthoCamera()
{ 
	_znear = 1.0f; 
	_zfar = 0.0f; 
	_projection = glm::mat4(1.0);
	_modelview = glm::mat4(1.0);
}

/// <summary>
/// Define the screen size. Compute the projection matrix
/// </summary>
/// <param name="width">Screen width in pixels</param>
/// <param name="height">Screen height in pixels</param>
void OrthoCamera::set_size(float width, float height)
{
	Camera::set_size(width, height);

	projection(glm::orthoLH<float>(0, width, height, 0, _znear, _zfar));

	compute();
}

/// <summary>
/// Center the camera to (0,0) coordinate
/// </summary>
void OrthoCamera::center()
{
	_xcenter = _width / 2.0f;
	_ycenter = _height / 2.0f;
	
	compute();
}

/// <summary>
/// Compute modelview matrix
/// </summary>
void OrthoCamera::compute()
{
	glm::mat4 matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, glm::vec3(_xcenter, _ycenter, 1.0f));
	matrix = glm::scale(matrix, glm::vec3(_scale, _scale, 1.0f));

	modelview(matrix);
}

/// <summary>
/// Move camera when right button is down
/// </summary>
/// <param name="x">X coordinate of mouse</param>
/// <param name="y">Y coordinate of mouse</param>
void OrthoCamera::mouseMove(float x, float y)
{
	if (_right_down)
	{
		_xcenter = _xcenter + (x - _xpos);
		_ycenter = _ycenter + (y - _ypos);
	
		compute();
	}
	
	Camera::mouseMove(x, y);
}

/// <summary>
/// Used for zoom operation into modelview
/// </summary>
/// <param name="xdelta">not used</param>
/// <param name="ydelta">wheel delta</param>
void OrthoCamera::mouseWheel(float xdelta, float ydelta)
{
	// get actuel mouse position
	auto am = getPosition(_xpos, _ypos);

	// compute new scale
	if (ydelta > 0)
	{
		_scale = _scale * 1.1f; // zoom in
		_zoom_inc++;
	}
	else
	{
		_scale = _scale * 0.9f; // zoom out
		_zoom_inc--;
	}
	//reset to 1
	if ( _zoom_inc == 0 ) _scale = 1.0f;

	// get new mouse position
	auto nm = getPosition(_xpos, _ypos);
	
	// translate difference to keep zoom centered to mouse
	_xcenter = _xcenter + (nm.x - am.x) * _scale;
	_ycenter = _ycenter - (nm.y - am.y) * _scale;

	compute();
	
	Camera::mouseWheel(xdelta, ydelta);
}

/// <summary>
/// Convert screen coordinate to modelview coordinate
/// </summary>
/// <param name="x">x coordinate</param>
/// <param name="y">y coordinate</param>
/// <returns>Modelview coordinate</returns>
glm::vec2 OrthoCamera::getPosition(float x, float y)
{
	return glm::vec2( 
		(x - _xcenter) / _scale,
		(-y + _ycenter) / _scale);
}

