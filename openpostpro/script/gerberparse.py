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

# Library: reads Gerber (RS-274X) and Excellon drill files into shapes of the drawing. Use it with "import gerberparse".
#
# parse(text) gives the objects of the file, convert(parsed, outline) gives the shapes, in the format of svgparse.py:
#   {'type': 'line', 'p1': (x, y), 'p2': (x, y)}, {'type': 'circle', 'center': (x, y), 'radius': r}, {'type': 'arc', 'start': ..., 'center': ..., 'stop': ..., 'cw': bool},
#   {'type': 'polyline', 'points': [...]}, {'type': 'spline', 'segments': [...]}
# each one has a 'layer'. The coordinates are in millimeters.
#
# Gerber: the apertures C, R, O and P; the flashes (D03), the tracks (D01, straight and arcs in single or multiple quadrant mode),
# the regions (G36/G37), the polarity (shapes of a clear polarity go to the layer "<name>_clear": they are not subtracted),
# the units and the number format, the macro apertures (AM: circle, vector line, center line, outline and polygon primitives, with their
# variables and arithmetic; the RoundRect macro of KiCad becomes one rounded rectangle, the other macros one shape per primitive).
# Excellon: the tools, the holes and the slots (G85), metric or inch, with or without a decimal point.

import math
import re

EPS = 1e-9


#######################################################
# Gerber                                              #
#######################################################

class Parsed:
	def __init__(self):
		self.kind = 'gerber'
		self.unit = 'mm'
		self.objects = []		# dictionaries: 'kind' (flash, track, region, hole, slot), 'clear', and the geometry
		self.warnings = []


# a coordinate written in the file: sign, digits, number of integer and decimal digits, and where the zeros were omitted
def decode(text, integer, decimal, zeros):
	negative = text.startswith('-')
	digits = text.lstrip('+-')
	if '.' in digits:
		value = float(digits)
	else:
		if zeros == 'T':
			digits = digits.ljust(integer + decimal, '0')
		value = int(digits or '0') / float(10 ** decimal)
	return -value if negative else value


# center of an arc from p0 to p1 given by the offsets i, j. In single quadrant mode the offsets are unsigned: the signs which give the right radius and an arc of at most 90 degrees are used
def arc_center(p0, p1, i, j, cw, single):
	if not single:
		return (p0[0] + i, p0[1] + j)
	best = None
	for sx in (1, -1):
		for sy in (1, -1):
			c = (p0[0] + sx * abs(i), p0[1] + sy * abs(j))
			r0 = math.hypot(p0[0] - c[0], p0[1] - c[1])
			r1 = math.hypot(p1[0] - c[0], p1[1] - c[1])
			sweep = sweep_angle(p0, p1, c, cw)
			error = abs(r0 - r1) + (0 if sweep <= math.pi / 2 + 1e-6 else 1.0)
			if best is None or error < best[0]:
				best = (error, c)
	return best[1]


# angle swept from p0 to p1 around c, always positive; a full turn if the points are the same
def sweep_angle(p0, p1, c, cw):
	a0 = math.atan2(p0[1] - c[1], p0[0] - c[0])
	a1 = math.atan2(p1[1] - c[1], p1[0] - c[0])
	sweep = (a0 - a1) if cw else (a1 - a0)
	while sweep < 0:
		sweep += 2 * math.pi
	if sweep < 1e-9 and math.hypot(p0[0] - p1[0], p0[1] - p1[1]) < 1e-9:
		sweep = 2 * math.pi
	return sweep


# value of an expression of a macro: numbers, $n variables, + - x / and parentheses
def evaluate(expression, variables):
	text = expression.strip().replace('x', '*').replace('X', '*')
	text = re.sub(r'\$(\d+)', lambda m: '(%r)' % variables.get(int(m.group(1)), 0.0), text)
	if not re.match(r'^[\d.eE+\-*/() ]*$', text) or not text.strip():
		return 0.0
	try:
		return float(eval(text, {'__builtins__': {}}, {}))
	except (SyntaxError, ZeroDivisionError, TypeError, ValueError):
		return 0.0


def rotate_point(p, degrees):
	a = math.radians(degrees)
	return (p[0] * math.cos(a) - p[1] * math.sin(a), p[0] * math.sin(a) + p[1] * math.cos(a))


# the primitives of a macro for the parameters of an aperture: dictionaries with their points in the local coordinates
# ('circle': center and radius, 'polygon': points, closed). Returns (primitives, warnings)
def macro_primitives(name, body, params):
	variables = {i + 1: v for i, v in enumerate(params)}
	primitives = []
	warnings = []
	for word in body:
		word = word.strip()
		if not word or word.startswith('0 ') or word == '0':
			continue
		m = re.match(r'^\$(\d+)\s*=(.*)$', word)
		if m:
			variables[int(m.group(1))] = evaluate(m.group(2), variables)
			continue
		values = [evaluate(v, variables) for v in word.split(',')]
		code = int(values[0])
		exposure = values[1] if len(values) > 1 else 1.0
		if exposure == 0:
			warnings.append('macro %s: a primitive of a clear exposure is ignored' % name)
			continue
		try:
			if code == 1:
				rotation = values[5] if len(values) > 5 else 0.0
				primitives.append({'kind': 'circle', 'center': rotate_point((values[3], values[4]), rotation), 'radius': values[2] / 2.0})
			elif code == 20:
				rotation = values[7] if len(values) > 7 else 0.0
				p0, p1 = (values[3], values[4]), (values[5], values[6])
				d = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
				h = values[2] / 2.0
				if d > EPS:
					nx, ny = -(p1[1] - p0[1]) / d * h, (p1[0] - p0[0]) / d * h
					corners = [(p0[0] + nx, p0[1] + ny), (p1[0] + nx, p1[1] + ny), (p1[0] - nx, p1[1] - ny), (p0[0] - nx, p0[1] - ny)]
					points = [rotate_point(c, rotation) for c in corners]
					primitives.append({'kind': 'polygon', 'points': points + [points[0]]})
			elif code == 21:
				rotation = values[6] if len(values) > 6 else 0.0
				w, h, cx, cy = values[2] / 2.0, values[3] / 2.0, values[4], values[5]
				points = [rotate_point(c, rotation) for c in ((cx - w, cy - h), (cx + w, cy - h), (cx + w, cy + h), (cx - w, cy + h))]
				primitives.append({'kind': 'polygon', 'points': points + [points[0]]})
			elif code == 4:
				count = int(values[2]) + 1
				rotation = values[3 + 2 * count] if len(values) > 3 + 2 * count else 0.0
				points = [rotate_point((values[3 + 2 * k], values[4 + 2 * k]), rotation) for k in range(count)]
				if points[0] != points[-1]:
					points.append(points[0])
				primitives.append({'kind': 'polygon', 'points': points})
			elif code == 5:
				count = max(3, int(values[2]))
				rotation = values[6] if len(values) > 6 else 0.0
				cx, cy, r = values[3], values[4], values[5] / 2.0
				points = [(cx + r * math.cos(math.radians(rotation) + 2 * math.pi * k / count), cy + r * math.sin(math.radians(rotation) + 2 * math.pi * k / count)) for k in range(count)]
				primitives.append({'kind': 'polygon', 'points': points + [points[0]]})
			else:
				warnings.append('macro %s: primitive %d not read' % (name, code))
		except IndexError:
			warnings.append('macro %s: a primitive has too few parameters' % name)
	return primitives, warnings


# the rounded rectangle of the RoundRect macro of KiCad: the polygon of the four corners, enlarged by the radius
def rounded_polygon(corners, radius):
	points = list(corners[:-1]) if corners[0] == corners[-1] else list(corners)
	area = sum(points[i][0] * points[(i + 1) % len(points)][1] - points[(i + 1) % len(points)][0] * points[i][1] for i in range(len(points)))
	if area < 0:
		points.reverse()
	n = len(points)
	normals = []
	for i in range(n):
		a, b = points[i], points[(i + 1) % n]
		d = math.hypot(b[0] - a[0], b[1] - a[1])
		normals.append((((b[1] - a[1]) / d, -(b[0] - a[0]) / d) if d > EPS else (0.0, 0.0)))
	segments = []
	for i in range(n):
		p, q = points[i], points[(i + 1) % n]
		ni, nj = normals[i], normals[(i + 1) % n]
		segments.append({'type': 'line', 'point': (p[0] + radius * ni[0], p[1] + radius * ni[1])})
		segments.append({'type': 'arc', 'point': (q[0] + radius * ni[0], q[1] + radius * ni[1]), 'center': q, 'radius': radius, 'cw': False})
	segments.append({'type': 'line', 'point': segments[0]['point']})
	return segments


def parse_aperture(text, parsed):
	m = re.match(r'ADD(\d+)([A-Za-z_][\w.$]*),?(.*)', text)
	if not m:
		return None
	code, kind, rest = int(m.group(1)), m.group(2), m.group(3)
	values = [float(v) for v in re.findall(r'[-+]?(?:\d+\.?\d*|\.\d+)', rest)]
	if kind == 'C' and values:
		return code, {'shape': 'C', 'd': values[0], 'hole': values[1] if len(values) > 1 else 0.0}
	if kind in ('R', 'O') and len(values) >= 2:
		return code, {'shape': kind, 'w': values[0], 'h': values[1], 'hole': values[2] if len(values) > 2 else 0.0}
	if kind == 'P' and len(values) >= 2:
		return code, {'shape': 'P', 'd': values[0], 'n': int(values[1]), 'rotation': values[2] if len(values) > 2 else 0.0, 'hole': values[3] if len(values) > 3 else 0.0}
	return code, {'shape': 'macro', 'name': kind, 'params': values}


def parse_gerber(text):
	parsed = Parsed()
	integer, decimal, zeros = 2, 4, 'L'
	incremental = False
	apertures = {}
	aperture = None
	x = y = 0.0
	mode = 'lin'
	single = False		# G74: single quadrant
	region = None		# the contours of the region being drawn
	contour = None
	clear = False
	macros = {}
	macro_block = None

	def end_contour():
		nonlocal contour
		if contour is not None and len(contour['segments']) > 0:
			region.append(contour)
		contour = None

	# the words: parameters between % and data blocks between *
	words = []
	i = 0
	n = len(text)
	while i < n:
		if text[i] == '%':
			j = text.find('%', i + 1)
			j = n if j < 0 else j
			for w in text[i + 1:j].split('*'):
				w = w.strip()
				if w:
					words.append(('param', w, i))
			i = j + 1
		elif text[i] in ' \r\n\t':
			i += 1
		else:
			j = text.find('*', i)
			j = n if j < 0 else j
			w = text[i:j].strip()
			if w:
				words.append(('data', w, i))
			i = j + 1

	for kind, word, block in words:
		if kind == 'param':
			if macro_block is not None and block == macro_block[0]:
				macro_block[1].append(word)
				continue
			if word.startswith('FS'):
				m = re.match(r'FS([LT]?)([AI]?)X(\d)(\d)Y(\d)(\d)', word)
				if m:
					zeros = m.group(1) or 'L'
					incremental = m.group(2) == 'I'
					integer, decimal = int(m.group(3)), int(m.group(4))
			elif word.startswith('MO'):
				parsed.unit = 'inch' if word[2:4] == 'IN' else 'mm'
			elif word.startswith('AD'):
				a = parse_aperture(word, parsed)
				if a:
					ap = a[1]
					if ap['shape'] == 'macro' and ap['name'] in macros:
						ap['prims'], notes = macro_primitives(ap['name'], macros[ap['name']], ap['params'])
						parsed.warnings.extend(notes)
					apertures[a[0]] = ap
			elif word.startswith('AM'):
				macros[word[2:]] = []
				macro_block = (block, macros[word[2:]])
			elif word.startswith('LP'):
				clear = word[2:3] == 'C'
			continue

		if word.startswith('G04'):
			continue
		if word.startswith('M02') or word.startswith('M00'):
			break
		numbers = dict()
		gcodes = []
		dcode = None
		for letter, value in re.findall(r'([GXYIJDM])([-+]?[\d.]+)', word):
			if letter == 'G':
				gcodes.append(int(float(value)))
			elif letter == 'D':
				dcode = int(float(value))
			elif letter in 'XYIJ':
				numbers[letter] = value

		for g in gcodes:
			if g == 1:
				mode = 'lin'
			elif g == 2:
				mode = 'cw'
			elif g == 3:
				mode = 'ccw'
			elif g == 74:
				single = True
			elif g == 75:
				single = False
			elif g == 36:
				region = []
				contour = None
			elif g == 37:
				end_contour()
				if region:
					parsed.objects.append({'kind': 'region', 'contours': region, 'clear': clear})
				region = None
				contour = None

		# a block without D code uses the last operation, which is a draw in a region
		if dcode is not None and dcode >= 10:
			aperture = apertures.get(dcode)
			if aperture is None:
				parsed.warnings.append('aperture D%d is not defined' % dcode)
			continue
		if 'X' not in numbers and 'Y' not in numbers and 'I' not in numbers and dcode is None:
			continue

		nx, ny = x, y
		if 'X' in numbers:
			v = decode(numbers['X'], integer, decimal, zeros)
			nx = x + v if incremental else v
		if 'Y' in numbers:
			v = decode(numbers['Y'], integer, decimal, zeros)
			ny = y + v if incremental else v
		i_off = decode(numbers['I'], integer, decimal, zeros) if 'I' in numbers else 0.0
		j_off = decode(numbers['J'], integer, decimal, zeros) if 'J' in numbers else 0.0

		if dcode == 2:
			if region is not None:
				end_contour()
				contour = {'start': (nx, ny), 'segments': []}
		elif dcode == 3:
			if aperture is None:
				parsed.warnings.append('flash without aperture')
			elif aperture['shape'] == 'macro' and 'prims' not in aperture:
				parsed.warnings.append('macro aperture %s not defined' % aperture['name'])
			else:
				parsed.objects.append({'kind': 'flash', 'at': (nx, ny), 'aperture': aperture, 'clear': clear})
		elif dcode == 1 or (dcode is None and ('X' in numbers or 'Y' in numbers)):
			segment = {'type': 'line', 'to': (nx, ny)}
			if mode != 'lin':
				cw = mode == 'cw'
				segment = {'type': 'arc', 'to': (nx, ny), 'center': arc_center((x, y), (nx, ny), i_off, j_off, cw, single), 'cw': cw}
			if region is not None:
				if contour is None:
					contour = {'start': (x, y), 'segments': []}
				contour['segments'].append(segment)
			elif aperture is None:
				parsed.warnings.append('draw without aperture')
			elif aperture['shape'] == 'macro':
				parsed.warnings.append('a track with the macro aperture %s is not allowed' % aperture['name'])
			else:
				parsed.objects.append({'kind': 'track', 'from': (x, y), 'segment': segment, 'aperture': aperture, 'clear': clear})
		x, y = nx, ny

	return parsed


#######################################################
# Excellon                                            #
#######################################################

def parse_excellon(text):
	parsed = Parsed()
	parsed.kind = 'excellon'
	parsed.unit = 'inch'
	integer, decimal = 2, 4
	zeros = 'L'		# leading zeros kept, the trailing ones are omitted
	tools = {}
	tool = None
	header = True
	absolute = True
	x = y = 0.0
	metric_default = False
	explicit_format = False

	def value(token):
		if '.' in token:
			return float(token)
		return decode(token, integer, decimal, 'T' if zeros == 'L' else 'L')

	for line in text.splitlines():
		line = line.strip()
		if not line or line.startswith(';'):
			if line.startswith(';') and re.search(r'FORMAT\s*=\s*\{?(\d):(\d)', line):
				m = re.search(r'FORMAT\s*=\s*\{?(\d):(\d)', line)
				integer, decimal = int(m.group(1)), int(m.group(2))
				explicit_format = True
			continue
		if line.startswith('M48'):
			header = True
			continue
		if line == '%' or line.startswith('M95'):
			header = False
			continue
		if line.startswith('METRIC') or line.startswith('INCH'):
			parsed.unit = 'mm' if line.startswith('METRIC') else 'inch'
			if not explicit_format:
				integer, decimal = (3, 3) if parsed.unit == 'mm' else (2, 4)
			if ',LZ' in line:
				zeros = 'L'
			elif ',TZ' in line:
				zeros = 'T'
			continue
		m = re.match(r'(?:(\d)\.(\d+))?', line)
		if line.startswith('M71') or line.startswith('M72'):
			parsed.unit = 'mm' if line.startswith('M71') else 'inch'
			if not explicit_format:
				integer, decimal = (3, 3) if parsed.unit == 'mm' else (2, 4)
			continue
		if line.startswith('G90'):
			absolute = True
			continue
		if line.startswith('G91'):
			absolute = False
			continue
		m = re.match(r'T(\d+)(?:.*?C([\d.]+))?', line)
		if m:
			number = int(m.group(1))
			if m.group(2) is not None:
				tools[number] = float(m.group(2))
			else:
				tool = number if number != 0 else None
			continue
		if line.startswith('M30') or line.startswith('M00'):
			break
		# holes and slots: X..Y.. or X..Y..G85X..Y..
		m = re.match(r'X([-+]?[\d.]+)?Y([-+]?[\d.]+)?(?:G85X([-+]?[\d.]+)Y([-+]?[\d.]+))?', line)
		if m and not header:
			nx = value(m.group(1)) if m.group(1) else x
			ny = value(m.group(2)) if m.group(2) else y
			if not absolute:
				nx, ny = x + nx, y + ny
			if tool is None or tool not in tools:
				parsed.warnings.append('hole without tool')
			elif m.group(3):
				parsed.objects.append({'kind': 'slot', 'from': (nx, ny), 'to': (value(m.group(3)), value(m.group(4))), 'd': tools[tool], 'clear': False})
			else:
				parsed.objects.append({'kind': 'hole', 'at': (nx, ny), 'd': tools[tool], 'clear': False})
			x, y = nx, ny
	return parsed


# the file is an Excellon file if it has the header M48
def parse(text):
	if re.search(r'^\s*M48', text, re.MULTILINE) and not re.search(r'%FS', text):
		return parse_excellon(text)
	return parse_gerber(text)


#######################################################
# the shapes                                          #
#######################################################

def regular_polygon(center, diameter, n, rotation):
	r = diameter / 2.0
	points = [(center[0] + r * math.cos(math.radians(rotation) + 2 * math.pi * k / n), center[1] + r * math.sin(math.radians(rotation) + 2 * math.pi * k / n)) for k in range(n)]
	return points + [points[0]]


# a rectangle with half circles at the ends of its long sides
def obround(center, w, h):
	if abs(w - h) < EPS:
		return [{'type': 'circle', 'center': center, 'radius': w / 2.0}]
	cx, cy = center
	if w > h:
		r = h / 2.0
		a, b = (cx - w / 2.0 + r, cy), (cx + w / 2.0 - r, cy)
		return [{'type': 'spline', 'segments': [
			{'type': 'line', 'point': (a[0], cy - r)}, {'type': 'arc', 'point': (b[0], cy - r), 'center': b, 'radius': r, 'cw': False},
			{'type': 'line', 'point': (b[0], cy + r)}, {'type': 'arc', 'point': (a[0], cy + r), 'center': a, 'radius': r, 'cw': False},
			{'type': 'line', 'point': (a[0], cy - r)}]}]
	r = w / 2.0
	a, b = (cx, cy - h / 2.0 + r), (cx, cy + h / 2.0 - r)
	return [{'type': 'spline', 'segments': [
		{'type': 'line', 'point': (cx + r, a[1])}, {'type': 'arc', 'point': (cx + r, b[1]), 'center': b, 'radius': r, 'cw': False},
		{'type': 'line', 'point': (cx - r, b[1])}, {'type': 'arc', 'point': (cx - r, a[1]), 'center': a, 'radius': r, 'cw': False},
		{'type': 'line', 'point': (cx + r, a[1])}]}]


def rectangle(center, w, h):
	cx, cy = center
	return [{'type': 'polyline', 'points': [(cx - w / 2.0, cy - h / 2.0), (cx + w / 2.0, cy - h / 2.0), (cx + w / 2.0, cy + h / 2.0), (cx - w / 2.0, cy + h / 2.0), (cx - w / 2.0, cy - h / 2.0)]}]


def macro_shapes(center, ap):
	shapes = []
	prims = ap['prims']
	# the macro of KiCad: a polygon of four corners, four circles and four rectangles, which together make a rounded rectangle
	if ap['name'] == 'RoundRect' and len(ap['params']) >= 9 and prims and prims[0]['kind'] == 'polygon':
		corners = [(p[0] + center[0], p[1] + center[1]) for p in prims[0]['points']]
		r = ap['params'][0]
		if r <= EPS:
			return [{'type': 'polyline', 'points': corners}]
		return [{'type': 'spline', 'segments': rounded_polygon(corners, r)}]
	for p in prims:
		if p['kind'] == 'circle':
			shapes.append({'type': 'circle', 'center': (p['center'][0] + center[0], p['center'][1] + center[1]), 'radius': p['radius']})
		else:
			shapes.append({'type': 'polyline', 'points': [(q[0] + center[0], q[1] + center[1]) for q in p['points']]})
	return shapes


def flash_shapes(center, ap):
	shape = ap['shape']
	if shape == 'macro':
		return macro_shapes(center, ap)
	if shape == 'C':
		shapes = [{'type': 'circle', 'center': center, 'radius': ap['d'] / 2.0}]
	elif shape == 'R':
		shapes = rectangle(center, ap['w'], ap['h'])
	elif shape == 'O':
		shapes = obround(center, ap['w'], ap['h'])
	else:
		shapes = [{'type': 'polyline', 'points': regular_polygon(center, ap['d'], max(3, ap['n']), ap['rotation'])}]
	if ap.get('hole', 0) > 0:
		shapes.append({'type': 'circle', 'center': center, 'radius': ap['hole'] / 2.0})
	return shapes


# a straight track of the given width: a rectangle with half circles at its ends, the outline goes clockwise
def capsule(p0, p1, width):
	h = width / 2.0
	dx, dy = p1[0] - p0[0], p1[1] - p0[1]
	d = math.hypot(dx, dy)
	if d < EPS:
		return [{'type': 'circle', 'center': p0, 'radius': h}]
	ux, uy = dx / d, dy / d
	nx, ny = -uy, ux
	a = (p0[0] + nx * h, p0[1] + ny * h)
	b = (p1[0] + nx * h, p1[1] + ny * h)
	c = (p1[0] - nx * h, p1[1] - ny * h)
	e = (p0[0] - nx * h, p0[1] - ny * h)
	return [{'type': 'spline', 'segments': [
		{'type': 'line', 'point': a}, {'type': 'arc', 'point': b, 'center': p1, 'radius': h, 'cw': True},
		{'type': 'line', 'point': c}, {'type': 'arc', 'point': e, 'center': p0, 'radius': h, 'cw': True},
		{'type': 'line', 'point': a}]}]


# a track on an arc: two arcs around the center and a half circle at each end
def arc_capsule(p0, p1, center, cw, width):
	h = width / 2.0
	r = math.hypot(p0[0] - center[0], p0[1] - center[1])
	if r <= h + EPS:
		return None
	a0 = math.atan2(p0[1] - center[1], p0[0] - center[0])
	a1 = math.atan2(p1[1] - center[1], p1[0] - center[0])

	def at(radius, angle):
		return (center[0] + radius * math.cos(angle), center[1] + radius * math.sin(angle))

	# going counter clockwise, the left side of the track is the inner one; clockwise, the outer one
	left, right = (r - h, r + h) if not cw else (r + h, r - h)
	s_l, e_l = at(left, a0), at(left, a1)
	s_r, e_r = at(right, a0), at(right, a1)
	return [{'type': 'spline', 'segments': [
		{'type': 'arc', 'point': s_l, 'center': center, 'radius': left, 'cw': cw},
		{'type': 'arc', 'point': e_l, 'center': p1, 'radius': h, 'cw': True},
		{'type': 'arc', 'point': e_r, 'center': center, 'radius': right, 'cw': not cw},
		{'type': 'arc', 'point': s_r, 'center': p0, 'radius': h, 'cw': True},
		{'type': 'line', 'point': s_l}]}]


def hull(points):
	points = sorted(set(points))
	if len(points) <= 2:
		return points

	def cross(o, a, b):
		return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

	lower, upper = [], []
	for p in points:
		while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
			lower.pop()
		lower.append(p)
	for p in reversed(points):
		while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
			upper.pop()
		upper.append(p)
	return lower[:-1] + upper[:-1]


def track_shapes(obj, outline):
	ap = obj['aperture']
	p0, seg = obj['from'], obj['segment']
	p1 = seg['to']
	if not outline:
		if seg['type'] == 'line':
			return [{'type': 'line', 'p1': p0, 'p2': p1}] if math.hypot(p1[0] - p0[0], p1[1] - p0[1]) > EPS else []
		return [{'type': 'arc', 'start': p0, 'center': seg['center'], 'stop': p1, 'cw': seg['cw']}]
	if ap['shape'] in ('C', 'O'):
		width = ap['d'] if ap['shape'] == 'C' else min(ap['w'], ap['h'])
		if seg['type'] == 'line':
			return capsule(p0, p1, width)
		shapes = arc_capsule(p0, p1, seg['center'], seg['cw'], width)
		if shapes is not None:
			return shapes
		return [{'type': 'arc', 'start': p0, 'center': seg['center'], 'stop': p1, 'cw': seg['cw']}]
	# a rectangular aperture moved along a straight line: the hull of its two positions
	w, h = (ap['w'], ap['h']) if ap['shape'] == 'R' else (ap['d'], ap['d'])
	corners = []
	for p in (p0, p1):
		for sx in (-1, 1):
			for sy in (-1, 1):
				corners.append((p[0] + sx * w / 2.0, p[1] + sy * h / 2.0))
	points = hull(corners)
	return [{'type': 'polyline', 'points': points + [points[0]]}] if len(points) >= 3 else []


def region_shapes(obj):
	shapes = []
	for contour in obj['contours']:
		segments = contour['segments']
		start = contour['start']
		if all(s['type'] == 'line' for s in segments):
			points = [start] + [s['to'] for s in segments]
			if math.hypot(points[0][0] - points[-1][0], points[0][1] - points[-1][1]) > EPS:
				points.append(points[0])
			if len(points) >= 4:
				shapes.append({'type': 'polyline', 'points': points})
			continue
		out = []
		p0 = start
		for s in segments:
			if s['type'] == 'line':
				out.append({'type': 'line', 'point': p0})
			else:
				out.append({'type': 'arc', 'point': p0, 'center': s['center'], 'radius': math.hypot(p0[0] - s['center'][0], p0[1] - s['center'][1]), 'cw': s['cw']})
			p0 = s['to']
		if math.hypot(p0[0] - start[0], p0[1] - start[1]) > EPS:
			out.append({'type': 'line', 'point': p0})
		out.append({'type': 'line', 'point': start})
		shapes.append({'type': 'spline', 'segments': out})
	return shapes


# the shapes of a parsed file, in millimeters; outline: the tracks are closed shapes of their width, otherwise their center line
def convert(parsed, outline=True, name='Gerber'):
	k = 25.4 if parsed.unit == 'inch' else 1.0
	shapes = []

	def scaled(shape):
		def f(p):
			return (p[0] * k, p[1] * k)

		out = dict(shape)
		for key in ('p1', 'p2', 'center', 'start', 'stop'):
			if key in out:
				out[key] = f(out[key])
		if 'radius' in out:
			out['radius'] = out['radius'] * k
		if 'points' in out:
			out['points'] = [f(p) for p in out['points']]
		if 'segments' in out:
			segments = []
			for g in out['segments']:
				c = dict(g)
				c['point'] = f(g['point'])
				if 'center' in g:
					c['center'] = f(g['center'])
					c['radius'] = g['radius'] * k
				segments.append(c)
			out['segments'] = segments
		return out

	for obj in parsed.objects:
		kind = obj['kind']
		if kind == 'flash':
			made = flash_shapes(obj['at'], obj['aperture'])
		elif kind == 'track':
			made = track_shapes(obj, outline)
		elif kind == 'region':
			made = region_shapes(obj)
		elif kind == 'hole':
			made = [{'type': 'circle', 'center': obj['at'], 'radius': obj['d'] / 2.0}]
		elif kind == 'slot':
			made = capsule(obj['from'], obj['to'], obj['d']) if outline else [{'type': 'line', 'p1': obj['from'], 'p2': obj['to']}]
		else:
			made = []
		layer = name + '_clear' if obj['clear'] else name
		for s in made:
			s = scaled(s)
			s['layer'] = layer
			shapes.append(s)
	return shapes
