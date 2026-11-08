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

# Library: draws the shapes read by svgparse.py or gerberparse.py with the calls of the engine. Use it with "import drawshapes".

# shapes: dictionaries with 'type' and 'layer' (see svgparse.py); (x, y) is added to every coordinate; returns the number of shapes drawn
def draw(core, shapes, x=0.0, y=0.0):
	layer = None
	count = 0

	def at(p):
		return (p[0] + x, p[1] + y)

	for s in shapes:
		if s['layer'] != layer:
			layer = s['layer']
			core.set_layer(layer)
		t = s['type']
		if t == 'line':
			(x1, y1), (x2, y2) = at(s['p1']), at(s['p2'])
			core.line(x1, y1, x2, y2)
		elif t == 'circle':
			cx, cy = at(s['center'])
			core.circle(cx, cy, s['radius'])
		elif t == 'arc':
			(x1, y1), (cx, cy), (x2, y2) = at(s['start']), at(s['center']), at(s['stop'])
			core.arc(x1, y1, cx, cy, x2, y2, s['cw'])
		elif t == 'ellipse':
			cx, cy = at(s['center'])
			core.ellipse(cx, cy, s['major'], s['minor'], 0.0, 6.283185307179586, s['angle'])
		elif t == 'polyline':
			core.polyline([at(p) for p in s['points']], 'polyline')
		elif t == 'spline':
			segments = []
			for g in s['segments']:
				c = dict(g)
				c['point'] = at(g['point'])
				if 'center' in g:
					c['center'] = at(g['center'])
				segments.append(c)
			core.spline(segments)
		elif t == 'text':
			tx, ty = at(s['p1'])
			core.text(tx, ty, s['text'], s['height'], s['family'], s['bold'], s['italic'])
		else:
			continue
		count += 1
	core.set_layer('')
	return count


# the shapes multiplied by k (the shapes of the libraries are in millimeters, the document can be in inches)
def scale(shapes, k):
	result = []
	for s in shapes:
		c = dict(s)
		for key in ('p1', 'p2', 'center', 'start', 'stop'):
			if key in c:
				c[key] = (c[key][0] * k, c[key][1] * k)
		if 'radius' in c:
			c['radius'] *= k
		if 'points' in c:
			c['points'] = [(p[0] * k, p[1] * k) for p in c['points']]
		if 'segments' in c:
			segments = []
			for g in c['segments']:
				d = dict(g)
				d['point'] = (g['point'][0] * k, g['point'][1] * k)
				if 'center' in g:
					d['center'] = (g['center'][0] * k, g['center'][1] * k)
					d['radius'] = g['radius'] * k
				segments.append(d)
			c['segments'] = segments
		result.append(c)
	return result
