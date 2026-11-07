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

#version 330 core
 
uniform mat4 projection;
uniform mat4 modelview;

uniform vec4 inner_color;       // filling color or inner gradient color
uniform vec4 outer_color;       // outer gradient color
uniform vec2 start_point;       // linear start point or radial outer center point, need to reverse y on orthogonal view
uniform vec2 end_point;         // linear end point or radial focal point, need to reverse y on orthogonal view
uniform float radius;    // length after the multiplication by projection of end_point to start_point

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

vec2 intersection(vec2 frag)
{
    float dx, dy, A, B, C, det, t;
    vec2 result;
                    
    dx = end_point.x - gl_FragCoord.x;
    dy = end_point.y - gl_FragCoord.y;

    A = dx * dx + dy * dy;
    B = 2.0 * (dx * (gl_FragCoord.x - start_point.x) + dy * (gl_FragCoord.y - start_point.y));
    C = (gl_FragCoord.x - start_point.x) * (gl_FragCoord.x - start_point.x) +
        (gl_FragCoord.y - start_point.y) * (gl_FragCoord.y - start_point.y) -
        radius * radius;

    det = B * B - 4.0 * A * C;
    if ((A <= 0.0000001) || (det < 0.0))
    {
        // no solution
    }
    else if (det == 0.0)
    {
        // one solution
        t = -B / (2.0 * A);
        result = vec2(gl_FragCoord.y + t * dx, gl_FragCoord.y + t * dy);
    }
    else
    {
        // two solutions
        t = (-B + sqrt(det)) / (2.0 * A);
        vec2 result1 = vec2(gl_FragCoord.x + t * dx, gl_FragCoord.y + t * dy);
        t = (-B - sqrt(det)) / (2.0 * A);
        vec2 result2 = vec2(gl_FragCoord.x + t * dx, gl_FragCoord.y + t * dy);
        if ( sign(dot(end_point, result1)) != sign(dot(end_point, frag)) )
            result = result1;
        else
            result = result2;
    }
    return result;
}

void main() {

    if ( use_clip == 1 && (gl_FragCoord.x < clip_x || gl_FragCoord.x > clip_right || gl_FragCoord.y > clip_y || gl_FragCoord.y < clip_bottom ) )
        discard;
		
	float alpha = 1.0;
	if ( use_aa == 1 )
		alpha = f_aa * aa_factor;
	
	vec4 color = inner_color;

    float outer = distance(start_point, vec2(gl_FragCoord.x,gl_FragCoord.y));
                        
    if ( outer > radius )
    {
        color = outer_color;
    }
    else
    {
        vec2 frag = vec2(gl_FragCoord.x,gl_FragCoord.y);
        float focal = distance(end_point, frag );
        float mix = 0.0;
        if ( end_point == start_point )
            mix = focal / radius;
        else
            mix = focal/distance(end_point, intersection(frag));
        color = inner_color * ( 1.0 - mix ) + outer_color * mix;                          
    }

	out_color = vec4(color.rgb, color.a * alpha);
}