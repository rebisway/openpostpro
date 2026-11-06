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

# Center of gravity: a point is added at the centroid of each closed contour of the selection.
# A contour is a closed polyline or spline, a chain of lines and arcs that closes, or a circle / ellipse (its center).
# The area is the one inside the contour: holes are not subtracted.

import geom

TOL = 1e-4		# two ends closer than this are the same point
ARC_STEP = 5.0	# degrees between the points of a sampled arc


# points of an arc from start to end
def arc_points(center, radius, start, end, cw):
	a1, a2, sweep = geom.arc_angles(center, start, end, cw)
	n = max(2, int(abs(sweep) / ARC_STEP) + 1)
	points = [geom.arc_point(center, radius, a1 + sweep * i / n) for i in range(1, n)]
	return [start] + points + [end]


# open pieces (lists of points) and closed contours of a shape: (pieces, contours)
def shape_parts(shape):
	t = shape['type']
	if t == 'line':
		return [[shape['p1'], shape['p2']]], []
	if t == 'arc':
		return [arc_points(shape['center'], shape['radius'], shape['start'], shape['stop'], shape['cw'])], []
	if t == 'polyline':
		points = list(shape['points'])
		if shape.get('closed') or (len(points) > 2 and geom.same(points[0], points[-1], TOL)):
			return [], [points]
		return [points], []
	if t == 'spline':
		segs = shape['segments']
		pieces = []
		n = len(segs)
		for i in range(n):
			s = segs[i]
			if s['type'] == 'circle':
				return [], [arc_points(s['center'], s['radius'], (s['center'][0] + s['radius'], s['center'][1]), (s['center'][0] + s['radius'], s['center'][1]), False)]
			if i + 1 < n:
				nxt = segs[i + 1]['point']
			elif shape.get('closed'):
				nxt = segs[0]['point']
			else:
				break
			if s['type'] == 'arc':
				pieces.append(arc_points(s['center'], s['radius'], s['point'], nxt, s['cw']))
			else:
				pieces.append([s['point'], nxt])
		return pieces, []
	return [], []


# chains the pieces end to end, returns the closed contours
def close_chains(pieces):
	pieces = [list(p) for p in pieces if len(p) > 1]
	contours = []
	while pieces:
		chain = pieces.pop(0)
		grown = True
		while grown and not geom.same(chain[0], chain[-1], TOL):
			grown = False
			for p in pieces:
				if geom.same(chain[-1], p[0], TOL):
					chain += p[1:]
				elif geom.same(chain[-1], p[-1], TOL):
					chain += p[-2::-1]
				elif geom.same(chain[0], p[-1], TOL):
					chain = p[:-1] + chain
				elif geom.same(chain[0], p[0], TOL):
					chain = p[:0:-1] + chain
				else:
					continue
				pieces.remove(p)
				grown = True
				break
		if geom.same(chain[0], chain[-1], TOL) and len(chain) > 3:
			contours.append(chain)
	return contours


# centroid of a polygon, None if it has no area
def centroid(points):
	if len(points) > 1 and geom.same(points[0], points[-1], TOL):
		points = points[:-1]
	a = x = y = 0.0
	n = len(points)
	for i in range(n):
		p, q = points[i], points[(i + 1) % n]
		f = p[0] * q[1] - q[0] * p[1]
		a += f
		x += (p[0] + q[0]) * f
		y += (p[1] + q[1]) * f
	if abs(a) < 1e-12:
		return None
	return (x / (3 * a), y / (3 * a))


class Cadscript:
	def __init__(self):
		self.en_us = {'description': 'Center of gravity', 'nothing': 'Select closed shapes.', 'none': 'No closed shape found in the selection.'}
		self.fr_fr = {'description': 'Centre de gravité', 'nothing': 'Sélectionnez des formes fermées.', 'none': 'Aucune forme fermée dans la sélection.'}
		self.es_es = {'description': 'Centro de gravedad', 'nothing': 'Seleccione formas cerradas.', 'none': 'No hay formas cerradas en la selección.'}

		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'geometry'

	# set the reference to the cad core object, do not modify
	def set_core(self, value):
		self.core = value

	def get_properties_count(self):
		return 0

	def get_property_description(self, index):
		return ''

	def get_property(self, index):
		return None

	def set_property(self, index, val):
		pass

	def run(self):
		if len(self.core.selection) == 0:
			self.core.message(self.lang['nothing'])
			return
		centers = []
		pieces = []
		contours = []
		for shape in self.core.selection:
			if shape['type'] in ('circle', 'ellipse'):
				centers.append(shape['center'])
				continue
			p, c = shape_parts(shape)
			pieces += p
			contours += c
		contours += close_chains(pieces)
		for c in contours:
			g = centroid(c)
			if g is not None:
				centers.append(g)
		if not centers:
			self.core.message(self.lang['none'])
			return
		for x, y in centers:
			self.core.point(x, y)
