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

// The block of material of the simulator, seen from above. Nothing is read from a vertex buffer: the vertices are made from
// gl_VertexID, and the height of each column is read in a texture of numbers (texelFetch).
//   part 0 : the top, a grid of triangles (6 vertices for each cell)
//   part 1 to 4 : the four sides, as strips (front: y min, back: y max, left: x min, right: x max)
//   part 5 : the bottom (6 vertices)

uniform mat4 projection;
uniform mat4 modelview;
uniform sampler2D heights;
uniform int grid_w;			// number of columns
uniform int grid_h;
uniform vec2 origin;		// position of the column (0, 0)
uniform float step;			// distance between two columns
uniform float top;			// height of the top of the block
uniform float bottom;		// height of the bottom of the block
uniform int part;

out vec3 f_world;
out vec3 f_normal;
out float f_cut;			// 1 where the tool went
out float f_hole;			// 1 where the tool went through

float height(int x, int y)
{
	return texelFetch(heights, ivec2(clamp(x, 0, grid_w - 1), clamp(y, 0, grid_h - 1)), 0).r;
}

void main()
{
	vec3 p = vec3(0.0);
	vec3 n = vec3(0.0, 0.0, 1.0);
	float z = top;

	if (part == 0)
	{
		int cell = gl_VertexID / 6;
		int corner = gl_VertexID % 6;
		int cells_x = grid_w - 1;
		int cx = cell % cells_x;
		int cy = cell / cells_x;
		// the two triangles of a cell: (0,0) (1,0) (0,1)  and  (0,1) (1,0) (1,1)
		int dx = (corner == 1 || corner == 4 || corner == 5) ? 1 : 0;
		int dy = (corner == 2 || corner == 3 || corner == 5) ? 1 : 0;
		int ix = cx + dx;
		int iy = cy + dy;
		z = height(ix, iy);
		p = vec3(origin + vec2(float(ix), float(iy)) * step, z);
		float hx = height(ix + 1, iy) - height(ix - 1, iy);
		float hy = height(ix, iy + 1) - height(ix, iy - 1);
		n = normalize(vec3(-hx, -hy, 2.0 * step));
	}
	else if (part >= 1 && part <= 4)
	{
		int i = gl_VertexID / 2;
		bool upper = (gl_VertexID % 2) == 0;
		int ix = 0;
		int iy = 0;
		if (part == 1) { ix = i; iy = 0; n = vec3(0.0, -1.0, 0.0); }
		else if (part == 2) { ix = i; iy = grid_h - 1; n = vec3(0.0, 1.0, 0.0); }
		else if (part == 3) { ix = 0; iy = i; n = vec3(-1.0, 0.0, 0.0); }
		else { ix = grid_w - 1; iy = i; n = vec3(1.0, 0.0, 0.0); }
		z = upper ? height(ix, iy) : bottom;
		p = vec3(origin + vec2(float(ix), float(iy)) * step, z);
	}
	else
	{
		// the bottom: two triangles on the corners of the block
		float x1 = origin.x + float(grid_w - 1) * step;
		float y1 = origin.y + float(grid_h - 1) * step;
		int corner = gl_VertexID % 6;
		float px = (corner == 1 || corner == 4 || corner == 5) ? x1 : origin.x;
		float py = (corner == 2 || corner == 3 || corner == 5) ? y1 : origin.y;
		p = vec3(px, py, bottom);
		n = vec3(0.0, 0.0, -1.0);
		z = bottom;
	}

	f_world = p;
	f_normal = n;
	f_cut = (part == 0 && z < top - 0.0005) ? 1.0 : 0.0;
	f_hole = (part == 0 && z <= bottom + 0.0005) ? 1.0 : 0.0;
	gl_Position = projection * modelview * vec4(p, 1.0);
}
