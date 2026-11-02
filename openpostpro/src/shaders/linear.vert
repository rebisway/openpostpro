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

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec2 in_vertex;
layout(location = 1) in float in_aa;

// Values that stay constant for the whole mesh.
uniform mat4 projection;
uniform mat4 modelview;

out float f_aa;

void main(){
    // Output position of the vertex, in clip space : projection * position
    gl_Position = projection * modelview * vec4(in_vertex,0,1);
	f_aa = in_aa;
}