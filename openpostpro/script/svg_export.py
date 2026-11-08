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

# SVG export: the selected shapes, or the whole drawing if nothing is selected, are written into an SVG file.
# The size is in millimeters, the layers are groups (Inkscape layers), the Y axis goes down as in every SVG file.
# A file dialog (tkinter) asks for the name of the file.
#
# Polylines with a curve mode: the Bezier and Catmull-Rom curves are written as cubic curves; the other modes (b-spline, natural cubic,
# Lagrange) are written by their control points, as a straight polyline.

import math
import os


class Cadscript:
	def __init__(self):
		self.file = ''
		self.en_us = {'description': 'SVG export', 'file': 'File (.svg)', 'dialog': 'SVG file', 'nothing': 'The drawing is empty.',
			'done': 'SVG exported: %d shapes in ', 'approx': '%d polylines with a curve mode are written by their control points.', 'error': 'The file can not be written: '}
		self.fr_fr = {'description': 'Export SVG', 'file': 'Fichier (.svg)', 'dialog': 'Fichier SVG', 'nothing': 'Le dessin est vide.',
			'done': 'SVG exporté : %d formes dans ', 'approx': '%d polylignes en mode courbe sont écrites par leurs points de contrôle.', 'error': 'Le fichier ne peut pas être écrit : '}
		self.es_es = {'description': 'Exportar SVG', 'file': 'Archivo (.svg)', 'dialog': 'Archivo SVG', 'nothing': 'El dibujo está vacío.',
			'done': 'SVG exportado: %d formas en ', 'approx': '%d polilíneas con modo curva se escriben por sus puntos de control.', 'error': 'No se puede escribir el archivo: '}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}
		self.approximated = 0

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'export'

	# set the reference to the cad core object, do not modify
	def set_core(self, value):
		self.core = value

	# the script asks the file by its own dialog box (tkinter): no properties
	def get_properties_count(self):
		return 0

	def get_property_description(self, index):
		return ''

	def get_property(self, index):
		return None

	def set_property(self, index, val):
		pass

	#######################################################
	# the text of the file                                #
	#######################################################

	@staticmethod
	def num(v):
		text = ('%.4f' % v).rstrip('0').rstrip('.')
		return '0' if text in ('', '-0') else text

	@staticmethod
	def escape(text):
		return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

	# extent of the shapes: (min x, min y, max x, max y)
	@staticmethod
	def bounds(shapes):
		xs, ys = [], []

		def add(x, y, r=0.0):
			xs.extend((x - r, x + r))
			ys.extend((y - r, y + r))

		for s in shapes:
			t = s['type']
			if t == 'line':
				add(*s['p1'])
				add(*s['p2'])
			elif t == 'circle':
				add(s['center'][0], s['center'][1], s['radius'])
			elif t == 'arc':
				# the whole circle is a safe bound for the arc
				add(s['center'][0], s['center'][1], s['radius'])
			elif t == 'ellipse':
				add(s['center'][0], s['center'][1], max(s['major'], s['minor']) / 2.0)
			elif t == 'point':
				add(*s['p1'])
			elif t == 'polyline':
				for p in s['points']:
					add(*p)
			elif t == 'spline':
				for g in s['segments']:
					if g['type'] == 'line':
						add(*g['point'])
					else:
						add(g['center'][0], g['center'][1], g['radius'])
			elif t == 'text':
				add(*s['p1'])
				add(s['p1'][0] + s['height'] * len(s['text']) * 0.6, s['p1'][1] + s['height'])
		if not xs:
			return 0.0, 0.0, 0.0, 0.0
		return min(xs), min(ys), max(xs), max(ys)

	# SVG arc flags of the arc from p0 to p1 around the center: (large arc, sweep)
	@staticmethod
	def arc_flags(p0, center, p1, cw):
		a0 = math.atan2(p0[1] - center[1], p0[0] - center[0])
		a1 = math.atan2(p1[1] - center[1], p1[0] - center[0])
		sweep = a1 - a0
		if cw:
			while sweep >= 0:
				sweep -= 2 * math.pi
		else:
			while sweep <= 0:
				sweep += 2 * math.pi
		# the Y axis goes down in the file: a counter clockwise arc has the sweep flag 0
		return (1 if abs(sweep) > math.pi else 0), (1 if cw else 0)

	def curve_path(self, points, mode, f):
		if mode == 'bezier' and len(points) == 4:
			p = [f(q) for q in points]
			return 'M %s %s C %s %s %s %s %s %s' % tuple(self.num(v) for q in p for v in q)
		if mode == 'catmull' and len(points) >= 3:
			p = [f(q) for q in points]
			d = 'M %s %s' % (self.num(p[0][0]), self.num(p[0][1]))
			for i in range(len(p) - 1):
				a = p[i - 1] if i > 0 else p[i]
				b, c = p[i], p[i + 1]
				e = p[i + 2] if i + 2 < len(p) else p[i + 1]
				c1 = (b[0] + (c[0] - a[0]) / 6.0, b[1] + (c[1] - a[1]) / 6.0)
				c2 = (c[0] - (e[0] - b[0]) / 6.0, c[1] - (e[1] - b[1]) / 6.0)
				d += ' C %s %s %s %s %s %s' % tuple(self.num(v) for q in (c1, c2, c) for v in q)
			return d
		return None

	# the SVG element of a shape, f: coordinates of the drawing to coordinates of the file
	def element(self, s, f):
		n = self.num
		t = s['type']
		if t == 'line':
			(x1, y1), (x2, y2) = f(s['p1']), f(s['p2'])
			return '<line x1="%s" y1="%s" x2="%s" y2="%s"/>' % (n(x1), n(y1), n(x2), n(y2))
		if t == 'circle':
			cx, cy = f(s['center'])
			return '<circle cx="%s" cy="%s" r="%s"/>' % (n(cx), n(cy), n(s['radius']))
		if t == 'point':
			x, y = f(s['p1'])
			return '<circle cx="%s" cy="%s" r="0.25" fill="black" stroke="none"/>' % (n(x), n(y))
		if t == 'arc':
			(x1, y1), (x2, y2) = f(s['start']), f(s['stop'])
			large, sweep = self.arc_flags(s['start'], s['center'], s['stop'], s['cw'])
			r = n(s['radius'])
			return '<path d="M %s %s A %s %s 0 %d %d %s %s"/>' % (n(x1), n(y1), r, r, large, sweep, n(x2), n(y2))
		if t == 'ellipse':
			cx, cy = f(s['center'])
			rx, ry = s['major'] / 2.0, s['minor'] / 2.0
			angle = -math.degrees(s['angle'])		# the Y axis goes down
			whole = abs((s['stop'] - s['start']) - 2 * math.pi) < 1e-3
			if whole:
				return '<ellipse cx="%s" cy="%s" rx="%s" ry="%s" transform="rotate(%s %s %s)"/>' % (n(cx), n(cy), n(rx), n(ry), n(angle), n(cx), n(cy))
			# a part of an ellipse: from its first point to its last one
			def point(a):
				x, y = rx * math.cos(a), ry * math.sin(a)
				c, sn = math.cos(s['angle']), math.sin(s['angle'])
				return f((s['center'][0] + c * x - sn * y, s['center'][1] + sn * x + c * y))
			(x1, y1), (x2, y2) = point(s['start']), point(s['stop'])
			sweep_angle = (s['stop'] - s['start']) % (2 * math.pi)
			return '<path d="M %s %s A %s %s %s %d 0 %s %s"/>' % (n(x1), n(y1), n(rx), n(ry), n(angle), 1 if sweep_angle > math.pi else 0, n(x2), n(y2))
		if t == 'polyline':
			points = [f(p) for p in s['points']]
			mode = s.get('mode', 'polyline')
			if mode != 'polyline':
				d = self.curve_path(s['points'], mode, f)
				if d is not None:
					return '<path d="%s%s"/>' % (d, ' Z' if s.get('closed') else '')
				self.approximated += 1
			closed = s.get('closed') and len(points) > 2
			if closed:
				points = points[:-1]
			text = ' '.join(n(x) + ',' + n(y) for x, y in points)
			return ('<polygon points="%s"/>' if closed else '<polyline points="%s"/>') % text
		if t == 'spline':
			d = ''
			segments = s['segments']
			for i, g in enumerate(segments):
				x, y = f(g['point'])
				if i == 0:
					d += 'M %s %s' % (n(x), n(y))
				if i + 1 >= len(segments):
					break
				nxt = segments[i + 1]['point']
				x2, y2 = f(nxt)
				if g['type'] == 'line':
					d += ' L %s %s' % (n(x2), n(y2))
				elif g['type'] == 'arc':
					large, sweep = self.arc_flags(g['point'], g['center'], nxt, g['cw'])
					d += ' A %s %s 0 %d %d %s %s' % (n(g['radius']), n(g['radius']), large, sweep, n(x2), n(y2))
				else:
					cx, cy = f(g['center'])
					return '<circle cx="%s" cy="%s" r="%s"/>' % (n(cx), n(cy), n(g['radius']))
			return '<path d="%s%s"/>' % (d, ' Z' if s.get('closed') else '')
		if t == 'text':
			x, y = f(s['p1'])
			style = ' font-weight="bold"' if s.get('bold') else ''
			style += ' font-style="italic"' if s.get('italic') else ''
			return '<text x="%s" y="%s" font-size="%s" font-family="%s"%s stroke="none" fill="black">%s</text>' % (
				n(x), n(y), n(s['height']), self.escape(s.get('family', 'Arial')), style, self.escape(s['text']))
		return None

	# the SVG text of the shapes (dictionaries of the selection), the coordinates are in the unit of the drawing: millimeters or inches
	def build(self, shapes, unit='mm'):
		self.approximated = 0
		k = 25.4 if unit == 'inch' else 1.0
		x0, y0, x1, y1 = self.bounds(shapes)
		margin = 1.0 / k
		x0, y0, x1, y1 = x0 - margin, y0 - margin, x1 + margin, y1 + margin
		width, height = (x1 - x0) * k, (y1 - y0) * k

		def f(p):
			return ((p[0] - x0) * k, (y1 - p[1]) * k)

		groups = {}
		order = []
		for s in shapes:
			element = self.element(s, f)
			if element is None:
				continue
			name = s.get('layer', '') or '0'
			if name not in groups:
				groups[name] = []
				order.append(name)
			groups[name].append(element)

		lines = ['<?xml version="1.0" encoding="UTF-8"?>',
			'<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" width="%smm" height="%smm" viewBox="0 0 %s %s">' % (
				self.num(width), self.num(height), self.num(width), self.num(height)),
			'<g fill="none" stroke="black" stroke-width="0.1">']
		for name in order:
			lines.append('<g inkscape:groupmode="layer" inkscape:label="%s" id="%s">' % (self.escape(name), self.escape(name.replace(' ', '_'))))
			lines += groups[name]
			lines.append('</g>')
		lines += ['</g>', '</svg>', '']
		return '\n'.join(lines)

	def ask_file(self):
		return self.core.save_file(self.lang['dialog'], [('SVG', '*.svg'), ('All', '*.*')], '', '.svg')

	def run(self):
		shapes = list(self.core.selection) or list(self.core.drawing)
		if not shapes:
			self.core.message(self.lang['nothing'])
			return
		path = self.ask_file()
		if not path:
			return
		text = self.build(shapes, self.core.unit)
		try:
			with open(path, 'w', encoding='utf-8', newline='\n') as f:
				f.write(text)
		except OSError as e:
			self.core.message(self.lang['error'] + str(e))
			return
		self.core.log((self.lang['done'] % len(shapes)) + path)
		if self.approximated:
			self.core.log(self.lang['approx'] % self.approximated)
