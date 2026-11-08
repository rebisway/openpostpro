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

// The block of material of the simulator made of tiles (SimModelTiles), seen from above. Nothing is read from a vertex buffer: the
// vertices are made from gl_VertexID.
//   part 0 : the tiles which are plateaus, 6 vertices for each tile (the list flat_list gives the tiles)
//   part 6 : the tiles which have their heights, a grid of triangles for each (the list detail_list gives the tiles)
//   part 1 to 4 : the four sides, as strips (front: y min, back: y max, left: x min, right: x max)
//   part 5 : the bottom (6 vertices)
// tile_info has a number for each tile: the height of a plateau, or 1e6 + the place of the tile in the atlas.

const int T = 32;				// cells on the side of a tile
const int V = T + 1;			// heights on the side of a tile in the atlas
const int LIST_WIDTH = 1024;

uniform mat4 projection;
uniform mat4 modelview;
uniform sampler2D tile_info;
uniform sampler2D atlas;
uniform sampler2D flat_list;
uniform sampler2D detail_list;
uniform int tiles_x;
uniform int tiles_y;
uniform int grid_w;				// heights of the whole grid
uniform int grid_h;
uniform int atlas_cols;			// places on a row of the atlas
uniform vec2 origin;			// position of the height (0, 0)
uniform float step;				// distance between two heights
uniform float top;				// height of the top of the block
uniform float bottom;			// height of the bottom of the block
uniform int part;

out vec3 f_world;
out vec3 f_normal;
out float f_cut;				// 1 where the tool went
out float f_hole;				// 1 where the tool went through

int list_item(sampler2D list, int i)
{
	return int(texelFetch(list, ivec2(i % LIST_WIDTH, i / LIST_WIDTH), 0).r + 0.5);
}

// the height of the tile which is at the place `slot` of the atlas, at the column (lx, ly) of the tile
float atlas_height(int slot, int lx, int ly)
{
	return texelFetch(atlas, ivec2((slot % atlas_cols) * V + lx, (slot / atlas_cols) * V + ly), 0).r;
}

float height(int x, int y)
{
	x = clamp(x, 0, grid_w - 1);
	y = clamp(y, 0, grid_h - 1);
	int tx = min(x / T, tiles_x - 1);
	int ty = min(y / T, tiles_y - 1);
	float info = texelFetch(tile_info, ivec2(tx, ty), 0).r;
	if (info < 1.0e5)
		return info;
	return atlas_height(int(info - 1.0e6 + 0.5), x - tx * T, y - ty * T);
}

void main()
{
	vec3 p = vec3(0.0);
	vec3 n = vec3(0.0, 0.0, 1.0);
	float z = top;

	if (part == 0)
	{
		int i = gl_VertexID / 6;
		int corner = gl_VertexID % 6;
		int t = list_item(flat_list, i);
		int tx = t % tiles_x;
		int ty = t / tiles_x;
		// the two triangles of the tile: (0,0) (1,0) (0,1)  and  (0,1) (1,0) (1,1)
		int dx = (corner == 1 || corner == 4 || corner == 5) ? 1 : 0;
		int dy = (corner == 2 || corner == 3 || corner == 5) ? 1 : 0;
		z = texelFetch(tile_info, ivec2(tx, ty), 0).r;
		p = vec3(origin + vec2(float(tx * T + dx * T), float(ty * T + dy * T)) * step, z);
	}
	else if (part == 6)
	{
		const int per_tile = T * T * 6;
		int i = gl_VertexID / per_tile;
		int rest = gl_VertexID % per_tile;
		int t = list_item(detail_list, i);
		int tx = t % tiles_x;
		int ty = t / tiles_x;
		int cell = rest / 6;
		int corner = rest % 6;
		int cx = cell % T;
		int cy = cell / T;
		int dx = (corner == 1 || corner == 4 || corner == 5) ? 1 : 0;
		int dy = (corner == 2 || corner == 3 || corner == 5) ? 1 : 0;
		int lx = cx + dx;
		int ly = cy + dy;
		int ix = tx * T + lx;
		int iy = ty * T + ly;
		float info = texelFetch(tile_info, ivec2(tx, ty), 0).r;
		int slot = int(info - 1.0e6 + 0.5);
		z = atlas_height(slot, lx, ly);
		p = vec3(origin + vec2(float(ix), float(iy)) * step, z);
		// the normal is made with the heights of the tile only (its borders are copies of the neighbours): one side at the border
		int xa = max(lx - 1, 0);
		int xb = min(lx + 1, T);
		int ya = max(ly - 1, 0);
		int yb = min(ly + 1, T);
		float hx = (atlas_height(slot, xb, ly) - atlas_height(slot, xa, ly)) / (float(xb - xa) * step);
		float hy = (atlas_height(slot, lx, yb) - atlas_height(slot, lx, ya)) / (float(yb - ya) * step);
		n = normalize(vec3(-hx, -hy, 1.0));
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

	bool is_top = part == 0 || part == 6;
	f_world = p;
	f_normal = n;
	f_cut = (is_top && z < top - 0.0005) ? 1.0 : 0.0;
	f_hole = (is_top && z <= bottom + 0.0005) ? 1.0 : 0.0;
	gl_Position = projection * modelview * vec4(p, 1.0);
}
