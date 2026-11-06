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

# Exterior involute spur gear, centered on the mouse, drawn as one closed polyline.
#
#  Modulus          module m of the gear
#  Teeth            number of teeth z
#  Pressure angle   angle of the flanks at the pitch circle (20 degrees is standard)
#  Pitch modifier   the pitch diameter is m * (z + modifier): moves the cutting circle (profile shift)
#  Outside modifier the tip diameter is the pitch diameter + m * (2 + modifier)
#  Dedendum         depth under the pitch circle, in modules (the root radius is rp - dedendum * m)
#  Spokes           lightening windows, see spokes.py: Offset is the spoke width and the hub radius

import math
import spokes
from geom import arc_point, rotate


def inv(a):
	return math.tan(a) - a


# the half thickness (angle) of a tooth at the radius r; base radius rb, pressure angle a, z teeth
def half_angle(r, rb, a, z):
	ar = math.acos(min(1.0, rb / r))
	return math.pi / (2 * z) + inv(a) - inv(ar)


# points of one tooth centered on the angle 0 then the root up to the next tooth, in radians, as (radius, angle)
def tooth(m, z, pressure, pitch_mod, outside_mod, dedendum, steps=8):
	a = math.radians(pressure)
	rp = m * (z + pitch_mod) / 2.0
	ra = rp + m * (1 + outside_mod / 2.0)
	rr = rp - dedendum * m
	rb = m * z / 2.0 * math.cos(a)
	if rr < 0.01 * m or ra <= rr:
		return None
	r0 = max(rb, rr)
	# the tip cannot be thinner than a point
	if half_angle(ra, rb, a, z) < 0:
		lo, hi = r0, ra
		for _ in range(60):
			mid = (lo + hi) / 2
			if half_angle(mid, rb, a, z) > 0:
				lo = mid
			else:
				hi = mid
		ra = lo
	if ra <= r0:
		return None

	flank = [(r0 + (ra - r0) * i / steps, 0.0) for i in range(steps + 1)]
	flank = [(r, half_angle(r, rb, a, z)) for r, _ in flank]
	return rr, rb, ra, flank


class Cadscript:
	KEYS = ['modulus', 'teeth', 'angle', 'primitive_diameter', 'outside_diameter', 'dedendum', 'use_spoke', 'spokes', 'offset']
	INTEGERS = ['teeth', 'spokes']
	BOOLS = ['use_spoke']

	def __init__(self):
		self.modulus = 5.0
		self.teeth = 20
		self.angle = 20.0
		self.primitive_diameter = 0.0
		self.outside_diameter = 0.0
		self.dedendum = 1.25
		self.use_spoke = False
		self.spokes = 3
		self.offset = 10.0

		self.en_us = {'description': 'Exterior involute spur gear', 'modulus': 'Modulus', 'teeth': 'Teeth', 'angle': 'Pressure angle (deg)',
			'primitive_diameter': 'Pitch diameter modifier', 'outside_diameter': 'Outside diameter modifier', 'dedendum': 'Dedendum (modules)',
			'use_spoke': 'Add spokes', 'spokes': 'Spokes', 'offset': 'Spoke offset', 'invalid': 'These dimensions do not give a gear: check the modulus, the teeth and the pressure angle.'}
		self.fr_fr = {'description': 'Roue dentée à denture droite extérieure', 'modulus': 'Module', 'teeth': 'Dents', 'angle': 'Angle de pression (deg)',
			'primitive_diameter': 'Modifieur diamètre primitif', 'outside_diameter': 'Modifieur diamètre extérieur', 'dedendum': 'Creux (modules)',
			'use_spoke': 'Ajouter des rayons', 'spokes': 'Nombre de rayons', 'offset': 'Offset des rayons', 'invalid': 'Ces dimensions ne donnent pas de roue : vérifiez le module, les dents et l\'angle de pression.'}
		self.es_es = {'description': 'Rueda dentada exterior', 'modulus': 'Módulo', 'teeth': 'Dientes', 'angle': 'Ángulo de presión (deg)',
			'primitive_diameter': 'Modificador diámetro primitivo', 'outside_diameter': 'Modificador diámetro exterior', 'dedendum': 'Pie del diente (módulos)',
			'use_spoke': 'Añadir radios', 'spokes': 'Radios', 'offset': 'Offset de los radios', 'invalid': 'Estas dimensiones no dan una rueda: compruebe el módulo, los dientes y el ángulo de presión.'}

		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'generation'

	# set the reference to the cad core object, do not modify
	def set_core(self, value):
		self.core = value

	def get_properties_count(self):
		return len(self.KEYS)

	def get_property_key(self, index):
		return self.KEYS[index]

	def get_property_description(self, index):
		return self.lang[self.KEYS[index]]

	def get_property(self, index):
		return getattr(self, self.KEYS[index])

	def set_property(self, index, val):
		key = self.KEYS[index]
		if key in self.BOOLS:
			setattr(self, key, val if isinstance(val, bool) else str(val).strip().lower() in ('1', 'true', 'yes'))
		elif key in self.INTEGERS:
			setattr(self, key, int(float(val)))
		else:
			setattr(self, key, float(val))

	# the outline of the gear around (0, 0): a closed list of points, and the root radius; None if invalid
	def outline(self):
		if self.teeth < 3 or self.modulus <= 0 or not 1 <= self.angle < 45:
			return None
		t = tooth(self.modulus, self.teeth, self.angle, self.primitive_diameter, self.outside_diameter, self.dedendum)
		if t is None:
			return None
		rr, rb, ra, flank = t
		z = self.teeth
		step = 2 * math.pi / z
		points = []
		for i in range(z):
			c = step * i
			right = [(r, c - h) for r, h in flank]
			left = [(r, c + h) for r, h in reversed(flank)]
			# radial line from the root to the base circle if the flank starts above the root
			if flank[0][0] > rr + 1e-9:
				right.insert(0, (rr, c - flank[0][1]))
				left.append((rr, c + flank[0][1]))
			points += right
			# tip
			points.append((ra, c))
			points += left
			# root up to the next tooth
			a_from = c + flank[0][1]
			a_to = c + step - flank[0][1]
			points += [(rr, a_from + (a_to - a_from) * k / 4.0) for k in range(1, 4)]
		xy = [(r * math.cos(a), r * math.sin(a)) for r, a in points]
		xy.append(xy[0])
		return xy, rr

	def run(self):
		o = self.outline()
		if o is None:
			self.core.message(self.lang['invalid'])
			return
		xy, rr = o
		cx, cy = self.core.mouse_x, self.core.mouse_y
		self.core.polyline([(cx + x, cy + y) for x, y in xy], 'polyline')
		if self.use_spoke:
			if not spokes.draw_spokes(self.core, cx, cy, self.offset, self.spokes, rr - self.offset):
				self.core.message(self.lang['invalid'])
