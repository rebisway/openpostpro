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

// Lines in 3D with one color: the paths, the axes, the box of the block, the tool (moved with `offset`).

layout(location = 0) in vec3 in_vertex;

uniform mat4 projection;
uniform mat4 modelview;
uniform vec3 offset;

void main()
{
	gl_Position = projection * modelview * vec4(in_vertex + offset, 1.0);
}
