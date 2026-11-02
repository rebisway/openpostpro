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
 
uniform mat4 projection;
uniform mat4 modelview;

uniform vec4 inner_color;       // filling color or inner gradient color
uniform vec4 outer_color;       // outer gradient color
uniform vec2 start_point;       // linear start point or radial outer center point, need to reverse y on orthogonal view
uniform vec2 end_point;         // linear end point or radial focal point, need to reverse y on orthogonal view
uniform float vector_length;    // length after the multiplication by projection of end_point to start_point

uniform int use_clip;
uniform float clip_x;
uniform float clip_y;
uniform float clip_right;
uniform float clip_bottom;


uniform int  use_aa;
uniform float aa_factor;

// input from vertex shader
in float  f_aa;

// Ouput data
out vec4 out_color;

float l = 0.0;

// return orthogonal projection of C on the line (A,B)]
vec2 orthogonal_projection(vec2 C, vec2 A, vec2 B)
{
    float x = A.x, y = C.y;

    if ( A.y == B.y ) // horizontal
    {
        x = C.x;
        y = A.y;
        l = abs(x - A.x); // calcul de la distance
    }
    else
    if ( A.x == B.x ) // vertical
    {
        x = A.x;
        y = C.y;
        l = abs(y - A.y); // calcul de la distance
    }
    else              // otherwise
    {
        float a = (A.y - B.y) / (A.x - B.x);
        float b = (B.y * A.x - A.y * B.x) / (A.x - B.x);
        float d = C.y + C.x / a;

        x = (-b + d) * a / (a * a + 1.0);
        y = a * x + b;

        l = distance(vec2(x, y), start_point);
    }

    return vec2(x, y);
}


void main() {

    if ( use_clip == 1 && (gl_FragCoord.x < clip_x || gl_FragCoord.x > clip_right || gl_FragCoord.y > clip_y || gl_FragCoord.y < clip_bottom ) )
        discard;
		
	float alpha = 1.0;
	if ( use_aa == 1 )
		alpha = f_aa * aa_factor;

    vec2 p = orthogonal_projection(vec2(gl_FragCoord.x, gl_FragCoord.y), start_point, end_point);

    if ( (start_point.x < end_point.x && p.x < start_point.x) || (end_point.x < start_point.x && p.x > start_point.x ) )
        l = -1.0;
    else 
    if ( (start_point.y < end_point.y && p.y < start_point.y) || (end_point.y < start_point.y && p.y > start_point.y ) )
        l = -1.0;

    float mix = clamp( l / vector_length, 0.0, 1.0);
    out_color = inner_color * ( 1.0 - mix ) + outer_color * mix;
	out_color = vec4(out_color.rgb, out_color.a * alpha);
}