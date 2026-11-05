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

# Fillet: every corner of the selected polygons is replaced by an arc of the given radius.
# chamfer.py uses the same code with a straight cut instead of the arc.
#
# Supported shapes:
#  - lines: the lines that share an end are shortened, the arcs are added. The lines keep their id.
#  - splines made of lines only: modified in place, they keep their id.
#  - polylines: they cannot hold an arc, so they are replaced by a spline (new id, same layer).
# A corner is left sharp if the radius does not fit in the length of its sides.

import math

EPS = 0.0001


def _sub(a, b):
	return (a[0] - b[0], a[1] - b[1])


def _dist(a, b):
	return math.hypot(a[0] - b[0], a[1] - b[1])


def _same(a, b):
	return abs(a[0] - b[0]) < EPS and abs(a[1] - b[1]) < EPS


class Cadscript:
	ROUNDED = True		# True: arc (fillet), False: straight cut (chamfer)

	def __init__(self):
		self.size = 5.0

		self.lang = None
		self.Language = self.texts()
		self.skipped = 0		# corners left sharp because they do not fit

	# texts of the interface by language
	def texts(self):
		return {
			'en-us': {'description': 'Fillet', 'size': 'Radius', 'nothing': 'Select lines, polylines or splines made of lines.', 'skipped': '%d corner(s) left sharp: they are too short for the radius.'},
			'fr-fr': {'description': 'Congé', 'size': 'Rayon', 'nothing': 'Sélectionnez des lignes, des polylignes ou des splines de lignes.', 'skipped': '%d coin(s) laissé(s) vif(s) : les côtés sont trop courts pour le rayon.'}
		}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.Language['en-us'])	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'CAD'

	# set the reference to the cad core object, do not modify
	def set_core(self, value):
		self.core = value

	def get_properties_count(self):
		return 1

	def get_property_key(self, index):
		return 'size'

	def get_property_description(self, index):
		return self.lang['size']

	def get_property(self, index):
		return self.size

	def set_property(self, index, val):
		self.size = float(val)

	#######################################################
	# geometry                                            #
	#######################################################

	# corner at p between the previous and the next points, None if there is nothing to round
	def corner(self, prev, p, nxt):
		v1 = _sub(prev, p)
		v2 = _sub(nxt, p)
		l1 = math.hypot(v1[0], v1[1])
		l2 = math.hypot(v2[0], v2[1])
		if l1 < EPS or l2 < EPS or self.size <= 0:
			return None

		u1 = (v1[0] / l1, v1[1] / l1)
		u2 = (v2[0] / l2, v2[1] / l2)
		angle = math.acos(max(-1.0, min(1.0, u1[0] * u2[0] + u1[1] * u2[1])))
		if angle < 0.001 or angle > math.pi - 0.001:
			return None		# straight line or folded back

		# distance between the corner and the end of the cut, on each side
		t = self.size / math.tan(angle / 2) if self.ROUNDED else self.size
		return {'p': p, 'u1': u1, 'u2': u2, 'angle': angle, 't': t}

	# start and end of the cut, then the center and the direction of the arc (None, False for a chamfer)
	def cut(self, c):
		p, u1, u2, t = c['p'], c['u1'], c['u2'], c['t']
		start = (p[0] + u1[0] * t, p[1] + u1[1] * t)
		end = (p[0] + u2[0] * t, p[1] + u2[1] * t)
		if not self.ROUNDED:
			return start, end, None, False

		bx, by = u1[0] + u2[0], u1[1] + u2[1]
		bl = math.hypot(bx, by)
		d = self.size / math.sin(c['angle'] / 2)
		center = (p[0] + bx / bl * d, p[1] + by / bl * d)
		# the path goes from the previous point to the next one: a right turn is clockwise
		turn = -u1[0] * u2[1] + u1[1] * u2[0]
		return start, end, center, turn < 0

	# remove the corners that do not fit: the cuts of one side must not be longer than the side
	def resolve(self, corners, lengths):
		while True:
			used = {}
			for c in corners:
				for s in c['sides']:
					used[s] = used.get(s, 0.0) + c['t']
			too_long = [s for s in used if used[s] > lengths[s] + EPS]
			if not too_long:
				return corners
			side = too_long[0]
			worst = max((c for c in corners if side in c['sides']), key=lambda c: c['t'])
			corners.remove(worst)
			self.skipped += 1

	# corners of a chain of points
	def chain(self, points, closed):
		n = len(points)
		corners = []
		for i in (range(n) if closed else range(1, n - 1)):
			c = self.corner(points[i - 1], points[i], points[(i + 1) % n])
			if c is not None:
				c['index'] = i
				c['sides'] = ((i - 1) % n, i)
				corners.append(c)

		lengths = {}
		for j in range(n if closed else n - 1):
			lengths[j] = _dist(points[j], points[(j + 1) % n])
		return self.resolve(corners, lengths)

	# segments of a spline following the points, the corners being cut
	def path(self, points, closed, corners):
		n = len(points)
		cuts = {}
		for c in corners:
			cuts[c['index']] = self.cut(c)

		cur = cuts[0][1] if (closed and 0 in cuts) else points[0]
		segments = []
		for k in range(1, n + 1 if closed else n):
			i = k % n
			segments.append({'type': 'line', 'point': cur})
			if i in cuts:
				start, end, center, cw = cuts[i]
				if center is None:
					segments.append({'type': 'line', 'point': start})
				else:
					segments.append({'type': 'arc', 'point': start, 'center': center, 'radius': self.size, 'cw': cw})
				cur = end
			else:
				cur = points[i]
		segments.append({'type': 'line', 'point': cur})
		return segments

	#######################################################
	# shapes                                              #
	#######################################################

	def run_lines(self, lines):
		# the lines are joined by their ends
		ends = {}
		points = []
		for i, shape in enumerate(lines):
			points.append([shape['p1'], shape['p2']])
			for e in (0, 1):
				x, y = points[i][e]
				ends.setdefault((round(x, 4), round(y, 4)), []).append((i, e))

		corners = []
		for key, joined in ends.items():
			if len(joined) == 2 and joined[0][0] != joined[1][0]:
				(a, ea), (b, eb) = joined
				c = self.corner(points[a][1 - ea], points[a][ea], points[b][1 - eb])
				if c is not None:
					c['sides'] = (a, b)
					c['joined'] = joined
					corners.append(c)

		lengths = {i: _dist(p[0], p[1]) for i, p in enumerate(points)}
		corners = self.resolve(corners, lengths)

		changed = set()
		for c in corners:
			(a, ea), (b, eb) = c['joined']
			start, end, center, cw = self.cut(c)
			points[a][ea] = start
			points[b][eb] = end
			changed.add(a)
			changed.add(b)
			self.core.set_layer(lines[a].get('layer', ''))		# the cut goes into the layer of the line
			if center is None:
				self.core.line(start[0], start[1], end[0], end[1])
			else:
				self.core.arc(start[0], start[1], center[0], center[1], end[0], end[1], cw)

		for i in changed:
			self.core.modify_line(lines[i]['id'], points[i][0][0], points[i][0][1], points[i][1][0], points[i][1][1])

	def run_chain(self, shape, points, closed):
		points = list(points)
		if closed and len(points) > 1 and _same(points[0], points[-1]):
			points.pop()
		if len(points) < 3:
			return False

		corners = self.chain(points, closed)
		if not corners:
			return True

		segments = self.path(points, closed, corners)
		if shape['type'] == 'spline':
			self.core.modify_spline(shape['id'], segments)
		else:
			# a polyline cannot hold an arc
			self.core.remove(shape['id'])
			self.core.set_layer(shape.get('layer', ''))
			self.core.spline(segments)
		return True

	def run(self):
		self.skipped = 0
		handled = 0
		lines = []
		for shape in self.core.selection:
			if shape['type'] == 'line':
				lines.append(shape)
			elif shape['type'] == 'polyline' and shape['mode'] == 'polyline':
				handled += self.run_chain(shape, shape['points'], shape['closed'])
			elif shape['type'] == 'spline' and all(s['type'] == 'line' for s in shape['segments']):
				handled += self.run_chain(shape, [s['point'] for s in shape['segments']], shape['closed'])

		if lines:
			self.run_lines(lines)
			handled += 1

		# nothing to work on, or corners that cannot be done
		if handled == 0:
			self.core.message(self.lang['nothing'])
		elif self.skipped > 0:
			self.core.message(self.lang['skipped'] % self.skipped)
