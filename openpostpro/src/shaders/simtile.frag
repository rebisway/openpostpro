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

// Same lighting as sim.frag, for the block made of tiles: the top is the parts 0 (plateaus) and 6 (tiles with heights)

in vec3 f_world;
in vec3 f_normal;
in float f_cut;
in float f_hole;

uniform vec3 eye;				// position of the camera: the light follows it
uniform vec4 top_color;			// the surface which was not touched
uniform vec4 cut_color;			// the surface which was machined
uniform vec4 side_color;
uniform int part;

out vec4 color;

void main()
{
	// where the tool went through, there is nothing
	if (f_hole > 0.5)
		discard;

	vec3 n = normalize(f_normal);
	vec3 to_eye = normalize(eye - f_world);
	// the faces are seen from both sides
	if (dot(n, to_eye) < 0.0)
		n = -n;

	// a light on the camera, and a weak one from above, so the relief shows
	float key = max(dot(n, to_eye), 0.0);
	float fill = max(dot(n, normalize(vec3(0.4, -0.3, 1.0))), 0.0);
	float light = 0.28 + 0.55 * key + 0.25 * fill;

	bool is_top = part == 0 || part == 6;
	vec4 base = is_top ? mix(top_color, cut_color, clamp(f_cut, 0.0, 1.0)) : side_color;
	color = vec4(base.rgb * light, base.a);
}
