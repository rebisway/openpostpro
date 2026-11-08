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

# Library: reads an SVG file into a list of shapes of the drawing. Use it with "import svgparse".
#
# The shapes are dictionaries, in millimeters, Y up, the lower left corner of the drawing at (0, 0):
#   {'type': 'line', 'p1': (x, y), 'p2': (x, y)}
#   {'type': 'circle', 'center': (x, y), 'radius': r}
#   {'type': 'ellipse', 'center': (x, y), 'major': full length, 'minor': full length, 'angle': radians of the major axis}
#   {'type': 'polyline', 'points': [(x, y), ...]}       a closed polyline repeats its first point
#   {'type': 'spline', 'segments': [{'type': 'line', 'point': (x, y)}, {'type': 'arc', 'point': ..., 'center': ..., 'radius': r, 'cw': bool}, ...]}
#   {'type': 'text', 'p1': (x, y), 'text': str, 'height': h, 'family': str, 'bold': bool, 'italic': bool}
# each one has a 'layer' (a name, '' for the current layer).
#
# What is read: svg, g (the layers of Inkscape, or the named groups of the top level), use, path (M L H V C S Q T A Z, absolute
# and relative), rect (rounded corners too), circle, ellipse, line, polyline, polygon, text and the transform attributes.
# Curves are cut into short segments (0.4 mm); the arcs of a path made of lines and arcs stay exact. The styles (colors, widths) are ignored.

import math
import re
import xml.etree.ElementTree as ET

PX = 25.4 / 96.0
UNITS = {'px': PX, 'pt': 25.4 / 72.0, 'pc': 25.4 / 6.0, 'mm': 1.0, 'cm': 10.0, 'in': 25.4, '': PX}
SKIP = ('defs', 'clippath', 'mask', 'metadata', 'style', 'title', 'desc', 'symbol', 'marker', 'pattern', 'lineargradient', 'radialgradient', 'filter', 'script', 'foreignobject', 'namedview')
SEGMENT = 0.4		# length of the segments of a curve, in millimeters
EPS = 1e-6


def local(tag):
	return tag.split('}')[-1].lower() if isinstance(tag, str) else ''


def numbers(text):
	return [float(v) for v in re.findall(r'[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?', text or '')]


# a length with its unit: (value, unit)
def length(text, default=0.0):
	m = re.match(r'\s*([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)\s*([a-zA-Z%]*)', text or '')
	if not m:
		return default, ''
	return float(m.group(1)), m.group(2).lower()


#######################################################
# matrices (a, b, c, d, e, f): x' = a x + c y + e, y' = b x + d y + f
#######################################################

IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


# the matrix which applies n then m
def multiply(m, n):
	return (m[0] * n[0] + m[2] * n[1], m[1] * n[0] + m[3] * n[1],
		m[0] * n[2] + m[2] * n[3], m[1] * n[2] + m[3] * n[3],
		m[0] * n[4] + m[2] * n[5] + m[4], m[1] * n[4] + m[3] * n[5] + m[5])


def apply(m, p):
	return (m[0] * p[0] + m[2] * p[1] + m[4], m[1] * p[0] + m[3] * p[1] + m[5])


def parse_transform(text):
	result = IDENTITY
	for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', text or ''):
		v = numbers(args)
		name = name.lower()
		if name == 'matrix' and len(v) == 6:
			m = tuple(v)
		elif name == 'translate' and v:
			m = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0.0)
		elif name == 'scale' and v:
			m = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
		elif name == 'rotate' and v:
			a = math.radians(v[0])
			c, s = math.cos(a), math.sin(a)
			m = (c, s, -s, c, 0, 0)
			if len(v) >= 3:
				m = multiply(multiply((1, 0, 0, 1, v[1], v[2]), m), (1, 0, 0, 1, -v[1], -v[2]))
		elif name == 'skewx' and v:
			m = (1, 0, math.tan(math.radians(v[0])), 1, 0, 0)
		elif name == 'skewy' and v:
			m = (1, math.tan(math.radians(v[0])), 0, 1, 0, 0)
		else:
			continue
		result = multiply(result, m)
	return result


def determinant(m):
	return m[0] * m[3] - m[1] * m[2]


# the scale of a rotation with a uniform scale (mirror allowed), None for any other matrix (non uniform scale, skew)
def similarity(m):
	s1 = math.hypot(m[0], m[1])
	s2 = math.hypot(m[2], m[3])
	if s1 < EPS or abs(s1 - s2) > 1e-6 * max(s1, 1.0):
		return None
	if abs(m[0] * m[2] + m[1] * m[3]) > 1e-6 * s1 * s2:
		return None
	return s1


#######################################################
# the path data
#######################################################

# subpaths: [{'start': (x, y), 'segs': [('L', p), ('C', p1, p2, p3), ('A', rx, ry, rotation, large, sweep, p)], 'closed': bool}]
def parse_path(d):
	tokens = re.findall(r'[MmLlHhVvCcSsQqTtAaZz]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?', d or '')
	subpaths = []
	current = None
	pos = (0.0, 0.0)
	start = pos
	last_control = None		# second control point of the last cubic, or control point of the last quadratic
	last_command = ''
	i = 0
	command = ''

	def number():
		nonlocal i
		v = float(tokens[i])
		i += 1
		return v

	def flag():
		# the flags of an arc can be written without separator: "a1 1 0 00.5 .5"
		nonlocal i
		t = tokens[i]
		if len(t) > 1 and t[0] in '01' and not t.startswith(('0.', '1.')):
			tokens[i] = t[1:]
			return t[0] == '1'
		i += 1
		return t[0] == '1'

	while i < len(tokens):
		if tokens[i].isalpha():
			command = tokens[i]
			i += 1
			if command in 'zZ':
				if current is not None:
					current['closed'] = True
					pos = start
					current = None		# a new subpath starts at the same point if there is a drawing command
				last_control = None
				last_command = 'z'
				continue
		elif command == '':
			break
		if command in 'zZ':
			break
		rel = command.islower()
		c = command.upper()
		ox, oy = pos if rel else (0.0, 0.0)
		try:
			if c == 'M':
				pos = (ox + number(), oy + number())
				start = pos
				current = {'start': pos, 'segs': [], 'closed': False}
				subpaths.append(current)
				command = 'l' if rel else 'L'		# the next pairs are lines
				last_control = None
				last_command = 'M'
				continue
			if current is None:
				current = {'start': pos, 'segs': [], 'closed': False}
				subpaths.append(current)
			if c == 'L':
				pos = (ox + number(), oy + number())
				current['segs'].append(('L', pos))
				last_control = None
			elif c == 'H':
				pos = (ox + number(), pos[1])
				current['segs'].append(('L', pos))
				last_control = None
			elif c == 'V':
				pos = (pos[0], oy + number())
				current['segs'].append(('L', pos))
				last_control = None
			elif c == 'C':
				p1 = (ox + number(), oy + number())
				p2 = (ox + number(), oy + number())
				p3 = (ox + number(), oy + number())
				current['segs'].append(('C', p1, p2, p3))
				last_control = ('C', p2)
				pos = p3
			elif c == 'S':
				p1 = (2 * pos[0] - last_control[1][0], 2 * pos[1] - last_control[1][1]) if last_control and last_control[0] == 'C' else pos
				p2 = (ox + number(), oy + number())
				p3 = (ox + number(), oy + number())
				current['segs'].append(('C', p1, p2, p3))
				last_control = ('C', p2)
				pos = p3
			elif c == 'Q' or c == 'T':
				if c == 'Q':
					q = (ox + number(), oy + number())
				else:
					q = (2 * pos[0] - last_control[1][0], 2 * pos[1] - last_control[1][1]) if last_control and last_control[0] == 'Q' else pos
				p3 = (ox + number(), oy + number())
				# a quadratic is a cubic
				p1 = (pos[0] + 2.0 / 3.0 * (q[0] - pos[0]), pos[1] + 2.0 / 3.0 * (q[1] - pos[1]))
				p2 = (p3[0] + 2.0 / 3.0 * (q[0] - p3[0]), p3[1] + 2.0 / 3.0 * (q[1] - p3[1]))
				current['segs'].append(('C', p1, p2, p3))
				last_control = ('Q', q)
				pos = p3
			elif c == 'A':
				rx, ry, rotation = number(), number(), number()
				large = flag()
				sweep = flag()
				pos = (ox + number(), oy + number())
				current['segs'].append(('A', rx, ry, rotation, large, sweep, pos))
				last_control = None
		except (IndexError, ValueError):
			break
	return [s for s in subpaths if s['segs']]


# center form of an SVG arc (appendix F.6.5 of the SVG specification): (cx, cy, rx, ry, start angle, sweep angle), None if it is a line
def arc_center(p0, rx, ry, rotation, large, sweep, p1):
	rx, ry = abs(rx), abs(ry)
	if rx < EPS or ry < EPS or (abs(p0[0] - p1[0]) < EPS and abs(p0[1] - p1[1]) < EPS):
		return None
	phi = math.radians(rotation)
	cp, sp = math.cos(phi), math.sin(phi)
	dx, dy = (p0[0] - p1[0]) / 2.0, (p0[1] - p1[1]) / 2.0
	x1 = cp * dx + sp * dy
	y1 = -sp * dx + cp * dy
	# the radii are enlarged if they are too small
	lam = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry)
	if lam > 1:
		rx *= math.sqrt(lam)
		ry *= math.sqrt(lam)
	num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1
	den = rx * rx * y1 * y1 + ry * ry * x1 * x1
	k = math.sqrt(max(0.0, num / den)) if den > 0 else 0.0
	if large == sweep:
		k = -k
	cx1 = k * rx * y1 / ry
	cy1 = -k * ry * x1 / rx
	cx = cp * cx1 - sp * cy1 + (p0[0] + p1[0]) / 2.0
	cy = sp * cx1 + cp * cy1 + (p0[1] + p1[1]) / 2.0
	a1 = math.atan2((y1 - cy1) / ry, (x1 - cx1) / rx)
	a2 = math.atan2((-y1 - cy1) / ry, (-x1 - cx1) / rx)
	delta = a2 - a1
	if sweep and delta < 0:
		delta += 2 * math.pi
	elif not sweep and delta > 0:
		delta -= 2 * math.pi
	return cx, cy, rx, ry, a1, delta


def arc_point(center, rx, ry, rotation, angle):
	phi = math.radians(rotation)
	x = rx * math.cos(angle)
	y = ry * math.sin(angle)
	return (center[0] + math.cos(phi) * x - math.sin(phi) * y, center[1] + math.sin(phi) * x + math.cos(phi) * y)


#######################################################
# from the path to the shapes
#######################################################

def bezier_points(p0, p1, p2, p3, scale):
	size = (math.hypot(p1[0] - p0[0], p1[1] - p0[1]) + math.hypot(p2[0] - p1[0], p2[1] - p1[1]) + math.hypot(p3[0] - p2[0], p3[1] - p2[1])) * scale
	n = max(8, min(120, int(math.ceil(size / SEGMENT))))
	points = []
	for k in range(1, n + 1):
		t = k / float(n)
		u = 1 - t
		points.append((u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
			u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]))
	return points


def arc_points(p0, rx, ry, rotation, large, sweep, p1, scale):
	arc = arc_center(p0, rx, ry, rotation, large, sweep, p1)
	if arc is None:
		return [p1]
	cx, cy, rx, ry, a1, delta = arc
	size = abs(delta) * max(rx, ry) * scale
	n = max(6, min(200, int(math.ceil(size / SEGMENT))))
	points = [arc_point((cx, cy), rx, ry, rotation, a1 + delta * k / float(n)) for k in range(1, n)]
	points.append(p1)
	return points


# the shapes of one subpath, in the final coordinates (matrix m)
def subpath_shapes(sub, m, layer):
	scale = math.sqrt(abs(determinant(m)))
	segs = sub['segs']
	closed = sub['closed']
	start = sub['start']
	has_curve = any(s[0] == 'C' for s in segs)
	arcs = [s for s in segs if s[0] == 'A']
	sim = similarity(m)

	# lines and arcs: a spline, the arcs stay exact. Elliptic arcs or a distorted drawing are cut into segments
	exact = not has_curve and arcs and sim is not None and all(abs(abs(s[1]) - abs(s[2])) < 1e-6 * max(abs(s[1]), 1.0) for s in arcs)
	if exact:
		segments = []
		p0 = start
		for s in segs:
			if s[0] == 'L':
				segments.append({'type': 'line', 'point': apply(m, p0)})
				p0 = s[1]
				continue
			arc = arc_center(p0, s[1], s[2], s[3], s[4], s[5], s[6])
			if arc is None:
				segments.append({'type': 'line', 'point': apply(m, p0)})
			else:
				cx, cy, rx, ry, a1, delta = arc
				a, b, c = apply(m, p0), apply(m, arc_point((cx, cy), rx, ry, s[3], a1 + delta / 2.0)), apply(m, s[6])
				cw = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]) < 0
				center = apply(m, (cx, cy))
				segments.append({'type': 'arc', 'point': a, 'center': center, 'radius': math.hypot(a[0] - center[0], a[1] - center[1]), 'cw': cw})
			p0 = s[6]
		segments.append({'type': 'line', 'point': apply(m, start if closed else p0)})
		if closed and (abs(p0[0] - start[0]) > EPS or abs(p0[1] - start[1]) > EPS):
			# the closing line
			segments.insert(len(segments) - 1, {'type': 'line', 'point': apply(m, p0)})
		return [{'type': 'spline', 'segments': segments, 'layer': layer}]

	points = [start]
	p0 = start
	for s in segs:
		if s[0] == 'L':
			points.append(s[1])
			p0 = s[1]
		elif s[0] == 'C':
			points += bezier_points(p0, s[1], s[2], s[3], scale)
			p0 = s[3]
		else:
			points += arc_points(p0, s[1], s[2], s[3], s[4], s[5], s[6], scale)
			p0 = s[6]
	points = [apply(m, p) for p in points]
	if closed and math.hypot(points[0][0] - points[-1][0], points[0][1] - points[-1][1]) > EPS:
		points.append(points[0])
	# the points which repeat the previous one are dropped
	clean = [points[0]]
	for p in points[1:]:
		if math.hypot(p[0] - clean[-1][0], p[1] - clean[-1][1]) > EPS:
			clean.append(p)
	if len(clean) == 2:
		return [{'type': 'line', 'p1': clean[0], 'p2': clean[1], 'layer': layer}]
	if len(clean) < 2:
		return []
	return [{'type': 'polyline', 'points': clean, 'layer': layer}]


def rect_path(x, y, w, h, rx, ry):
	if rx <= 0 and ry <= 0:
		return [{'start': (x, y), 'segs': [('L', (x + w, y)), ('L', (x + w, y + h)), ('L', (x, y + h))], 'closed': True}]
	if rx <= 0:
		rx = ry
	if ry <= 0:
		ry = rx
	rx = min(rx, w / 2.0)
	ry = min(ry, h / 2.0)
	segs = [('L', (x + w - rx, y)), ('A', rx, ry, 0, False, True, (x + w, y + ry)),
		('L', (x + w, y + h - ry)), ('A', rx, ry, 0, False, True, (x + w - rx, y + h)),
		('L', (x + rx, y + h)), ('A', rx, ry, 0, False, True, (x, y + h - ry)),
		('L', (x, y + ry)), ('A', rx, ry, 0, False, True, (x + rx, y))]
	return [{'start': (x + rx, y), 'segs': segs, 'closed': True}]


def ellipse_shapes(cx, cy, rx, ry, m, layer):
	if rx <= 0 or ry <= 0:
		return []
	sim = similarity(m)
	if sim is not None:
		center = apply(m, (cx, cy))
		if abs(rx - ry) < 1e-9 * max(rx, ry):
			return [{'type': 'circle', 'center': center, 'radius': rx * sim, 'layer': layer}]
		# the axes of the ellipse follow the matrix (a mirror does not change the direction of an axis)
		if rx >= ry:
			major, minor = 2 * rx * sim, 2 * ry * sim
			angle = math.atan2(m[1], m[0])
		else:
			major, minor = 2 * ry * sim, 2 * rx * sim
			angle = math.atan2(m[3], m[2])
		return [{'type': 'ellipse', 'center': center, 'major': major, 'minor': minor, 'angle': angle, 'layer': layer}]
	# distorted: a polyline
	path = [{'start': (cx + rx, cy), 'segs': [('A', rx, ry, 0, False, True, (cx, cy + ry)), ('A', rx, ry, 0, False, True, (cx - rx, cy)),
		('A', rx, ry, 0, False, True, (cx, cy - ry)), ('A', rx, ry, 0, False, True, (cx + rx, cy))], 'closed': True}]
	return subpath_shapes(path[0], m, layer)


#######################################################
# the document
#######################################################

def style_of(elem):
	style = {}
	for item in (elem.get('style') or '').split(';'):
		if ':' in item:
			k, v = item.split(':', 1)
			style[k.strip().lower()] = v.strip()
	return style


def attribute(elem, name, style=None):
	style = style if style is not None else style_of(elem)
	return style.get(name, elem.get(name))


class Reader:
	def __init__(self, root):
		self.root = root
		self.ids = {}
		for e in root.iter():
			if e.get('id'):
				self.ids[e.get('id')] = e
		self.shapes = []
		self.warnings = []

	def base_matrix(self, scale=1.0):
		width, wunit = length(self.root.get('width'), 0.0)
		height, hunit = length(self.root.get('height'), 0.0)
		box = numbers(self.root.get('viewBox') or self.root.get('viewbox'))
		if len(box) == 4 and box[2] > 0 and box[3] > 0:
			vx, vy, vw, vh = box
			if width > 0 and wunit != '%':
				k = width * UNITS.get(wunit, PX) / vw
			elif height > 0 and hunit != '%':
				k = height * UNITS.get(hunit, PX) / vh
			else:
				k = PX
		else:
			# no viewBox: one user unit is one pixel, the height is converted to user units
			vx = vy = 0.0
			k = PX
			vh = height * UNITS.get(hunit, PX) / PX if height > 0 and hunit != '%' else 0.0
		k *= scale
		# the origin is the lower left corner of the drawing, the Y axis goes up
		return (k, 0.0, 0.0, -k, -vx * k, (vy + vh) * k)

	def walk(self, elem, matrix, layer, depth=0, top=False):
		tag = local(elem.tag)
		if tag in SKIP or depth > 40:
			return
		style = style_of(elem)
		if style.get('display', elem.get('display')) == 'none' or style.get('visibility', elem.get('visibility')) == 'hidden':
			return
		m = multiply(matrix, parse_transform(elem.get('transform')))

		if tag in ('svg', 'g', 'a', 'switch'):
			name = layer
			label = elem.get('{http://www.inkscape.org/namespaces/inkscape}label')
			if elem.get('{http://www.inkscape.org/namespaces/inkscape}groupmode') == 'layer' and label:
				name = label
			elif top and tag == 'g' and elem.get('id') and not re.match(r'^(g|layer|group)?\d+$', elem.get('id')):
				name = elem.get('id')
			for child in elem:
				self.walk(child, m, name, depth + 1, top=(tag == 'svg' and depth == 0))
		elif tag == 'use':
			href = elem.get('{http://www.w3.org/1999/xlink}href') or elem.get('href') or ''
			target = self.ids.get(href.lstrip('#'))
			if target is not None:
				place = (1, 0, 0, 1, length(elem.get('x'))[0], length(elem.get('y'))[0])
				self.walk(target, multiply(m, place), layer, depth + 1)
		elif tag == 'path':
			for sub in parse_path(elem.get('d')):
				self.shapes += subpath_shapes(sub, m, layer)
		elif tag == 'rect':
			x, y = length(elem.get('x'))[0], length(elem.get('y'))[0]
			w, h = length(elem.get('width'))[0], length(elem.get('height'))[0]
			if w > 0 and h > 0:
				rx, ry = length(elem.get('rx'))[0], length(elem.get('ry'))[0]
				for sub in rect_path(x, y, w, h, rx, ry):
					self.shapes += subpath_shapes(sub, m, layer)
		elif tag == 'circle':
			r = length(elem.get('r'))[0]
			self.shapes += ellipse_shapes(length(elem.get('cx'))[0], length(elem.get('cy'))[0], r, r, m, layer)
		elif tag == 'ellipse':
			self.shapes += ellipse_shapes(length(elem.get('cx'))[0], length(elem.get('cy'))[0], length(elem.get('rx'))[0], length(elem.get('ry'))[0], m, layer)
		elif tag == 'line':
			p1 = apply(m, (length(elem.get('x1'))[0], length(elem.get('y1'))[0]))
			p2 = apply(m, (length(elem.get('x2'))[0], length(elem.get('y2'))[0]))
			if math.hypot(p1[0] - p2[0], p1[1] - p2[1]) > EPS:
				self.shapes.append({'type': 'line', 'p1': p1, 'p2': p2, 'layer': layer})
		elif tag in ('polyline', 'polygon'):
			v = numbers(elem.get('points'))
			points = [apply(m, (v[i], v[i + 1])) for i in range(0, len(v) - 1, 2)]
			if len(points) >= 2:
				if tag == 'polygon' and math.hypot(points[0][0] - points[-1][0], points[0][1] - points[-1][1]) > EPS:
					points.append(points[0])
				self.shapes.append({'type': 'polyline', 'points': points, 'layer': layer})
		elif tag == 'text':
			self.text(elem, m, layer, style)

	def text(self, elem, m, layer, style):
		content = ''.join(elem.itertext())
		content = re.sub(r'\s+', ' ', content).strip()
		if not content:
			return
		size, unit = length(attribute(elem, 'font-size', style), 16.0)
		if unit not in UNITS:
			unit = ''
		k = math.sqrt(abs(determinant(m)))
		family = (attribute(elem, 'font-family', style) or 'arial').split(',')[0].strip().strip('\'"') or 'arial'
		self.shapes.append({'type': 'text', 'p1': apply(m, (length(elem.get('x'))[0], length(elem.get('y'))[0])), 'text': content,
			'height': size * k, 'family': family, 'bold': (attribute(elem, 'font-weight', style) or '') in ('bold', '700', '800', '900'),
			'italic': (attribute(elem, 'font-style', style) or '') == 'italic', 'layer': layer})


# the shapes of an SVG text, scale: multiplies the size. Raises ValueError if the text is not an SVG
def parse(text, scale=1.0):
	try:
		root = ET.fromstring(text.encode('utf-8') if isinstance(text, str) else text)
	except ET.ParseError as e:
		raise ValueError(str(e))
	if local(root.tag) != 'svg':
		raise ValueError('not an SVG file')
	reader = Reader(root)
	reader.walk(root, reader.base_matrix(scale), '', top=False)
	return reader.shapes
