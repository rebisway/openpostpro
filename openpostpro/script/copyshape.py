# OpenPostPro - www.rebisway.com - contact@rebisway.com
# Copyright (C) 2024-2026 Thomas Gourgnier
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Library: copies a shape of the selection with a rigid move (translation and/or rotation).
# Use it with "import copyshape". The text keeps its direction: only its position moves.

import math


# move: function that gives the new position of a point; turn: rotation in degrees applied to the directions
def copy_shape(core, shape, move, turn=0.0):
	core.set_layer(shape.get('layer', ''))
	t = shape['type']
	if t == 'line':
		(x1, y1), (x2, y2) = move(shape['p1']), move(shape['p2'])
		core.line(x1, y1, x2, y2)
	elif t == 'circle':
		x, y = move(shape['center'])
		core.circle(x, y, shape['radius'])
	elif t == 'arc':
		(x1, y1), (cx, cy), (x2, y2) = move(shape['start']), move(shape['center']), move(shape['stop'])
		core.arc(x1, y1, cx, cy, x2, y2, shape['cw'])
	elif t == 'point':
		x, y = move(shape['p1'])
		core.point(x, y)
	elif t == 'ellipse':
		x, y = move(shape['center'])
		core.ellipse(x, y, shape['major'], shape['minor'], shape['start'], shape['stop'], shape['angle'] + math.radians(turn))
	elif t == 'polyline':
		core.polyline([move(p) for p in shape['points']], shape['mode'])
	elif t == 'spline':
		segments = []
		for s in shape['segments']:
			c = dict(s)
			c['point'] = move(s['point'])
			if 'center' in s:
				c['center'] = move(s['center'])
			segments.append(c)
		core.spline(segments)
	elif t == 'text':
		x, y = move(shape['p1'])
		core.text(x, y, shape['text'], shape['height'], shape['family'], shape['bold'], shape['italic'], shape['inter'])
