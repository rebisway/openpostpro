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

#version 300 es
precision highp float;

// Interpolated values from the vertex shaders
in vec2 vec_texture;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh.
uniform sampler2D texture_sampler;

uniform int use_clip;
uniform float clip_x;
uniform float clip_y;
uniform float clip_right;
uniform float clip_bottom;

uniform vec4 inner_color;
uniform  int apply_inner;


void main() 
{
    if ( use_clip == 1 && (gl_FragCoord.x < clip_x || gl_FragCoord.x > clip_right || gl_FragCoord.y > clip_y || gl_FragCoord.y < clip_bottom ) )
        discard;

	// Output color = color of the texture at the specified vec_texture
	color = texture( texture_sampler, vec_texture );

	if ( apply_inner == 1 )
	{
		color = vec4(inner_color.r, inner_color.g,inner_color.b, color.a * inner_color.a); 
	}
}