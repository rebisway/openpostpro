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

#version 300 es
precision highp float;

// Interpolated values from the vertex shaders
in vec2 vec_texture;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh.
uniform vec4 inner_color;
uniform sampler2D texture_sampler;
uniform float onedge_value;
uniform float pixel_dist_scale;
uniform float relative_scale;

uniform float u_buffer;
uniform float u_gamma;

uniform int use_clip;
uniform float clip_x;
uniform float clip_y;
uniform float clip_right;
uniform float clip_bottom;


float stb_lerp(float t, float a, float b) 
{
	return a + t * (b-a);
}

float stb_unlerp(float t, float a, float b)
{
	return (t - a) / (b - a);
}

float stb_linear_remap(float x, float x_min, float x_max, float out_min, float out_max)
{
   return stb_lerp(stb_unlerp(x,x_min,x_max),out_min,out_max);
}

void main() 
{
    if ( use_clip == 1 && (gl_FragCoord.x < clip_x || gl_FragCoord.x > clip_right || gl_FragCoord.y > clip_y || gl_FragCoord.y < clip_bottom ) )
        discard;

	// Output color = color of the texture at the specified vec_texture

	//float sdf_dist = stb_linear_remap(texture( texture_sampler, vec_texture ).a, onedge_value, onedge_value+pixel_dist_scale, 0, 1);
 //   // convert distance in SDF bitmap to distance in output bitmap
 //   float pix_dist = sdf_dist * relative_scale;
 //   // anti-alias by mapping 1/2 pixel around contour from 0..1 alpha
 //   float v = stb_linear_remap(pix_dist, -0.5f, 0.5f, 0, 1);
 //   if (v > 1) v = 1;
 //   if (v > 0)
	//	color = vec4(inner_color.rgb, v);
	
	//float alpha = texture( texture_sampler, vec_texture ).a;

	//color = vec4(inner_color.rgb, alpha);

	float alpha = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, texture( texture_sampler, vec_texture ).a);
    color = vec4(inner_color.rgb, alpha * inner_color.a);

}