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

# Spokes: the windows of a wheel. The ring between the hub (radius = Offset) and the Radius is cut into "Count" windows
# separated by spokes of width Offset. Each window is two lines and two arcs, centered on the mouse.
# exterior_gear.py uses draw_spokes() to lighten a gear.

import math


# draws the windows around (cx, cy), returns False if the dimensions do not fit
def draw_spokes(core, cx, cy, offset, count, radius):
	hub = offset				# radius of the hub
	h = offset / 2.0			# half width of a spoke
	if count < 2 or offset <= 0 or radius <= hub or hub <= h:
		return False
	t_in = math.sqrt(hub * hub - h * h)
	t_out = math.sqrt(radius * radius - h * h)
	for k in range(count):
		a1 = 2 * math.pi * k / count
		a2 = 2 * math.pi * (k + 1) / count
		u1, n1 = (math.cos(a1), math.sin(a1)), (-math.sin(a1), math.cos(a1))
		u2, n2 = (math.cos(a2), math.sin(a2)), (-math.sin(a2), math.cos(a2))
		# side along the spoke k (to its left), then side along the spoke k + 1 (to its right)
		p1 = (cx + t_in * u1[0] + h * n1[0], cy + t_in * u1[1] + h * n1[1])
		p2 = (cx + t_out * u1[0] + h * n1[0], cy + t_out * u1[1] + h * n1[1])
		p3 = (cx + t_out * u2[0] - h * n2[0], cy + t_out * u2[1] - h * n2[1])
		p4 = (cx + t_in * u2[0] - h * n2[0], cy + t_in * u2[1] - h * n2[1])
		# the window must have a width: the two sides must not cross
		if (a2 - a1) / 2 <= math.asin(h / hub):
			return False
		core.line(p1[0], p1[1], p2[0], p2[1])
		core.arc(p2[0], p2[1], cx, cy, p3[0], p3[1], False)
		core.line(p3[0], p3[1], p4[0], p4[1])
		core.arc(p4[0], p4[1], cx, cy, p1[0], p1[1], True)
	return True


class Cadscript:
	KEYS = ['offset', 'count', 'radius']
	INTEGERS = ['count']

	def __init__(self):
		self.offset = 20.0
		self.count = 3
		self.radius = 100.0

		self.en_us = {'description': 'Wheel spokes', 'offset': 'Offset (hub radius and spoke width)', 'count': 'Count', 'radius': 'Outer radius', 'nofit': 'The spokes do not fit: check the offset, the count and the radius.'}
		self.fr_fr = {'description': 'Rayons de roue', 'offset': 'Offset (rayon du moyeu et largeur des rayons)', 'count': 'Nombre', 'radius': 'Rayon extérieur', 'nofit': 'Les rayons ne tiennent pas : vérifiez l\'offset, le nombre et le rayon.'}
		self.es_es = {'description': 'Radios de rueda', 'offset': 'Offset (radio del buje y ancho de los radios)', 'count': 'Número', 'radius': 'Radio exterior', 'nofit': 'Los radios no caben: compruebe el offset, el número y el radio.'}

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
		setattr(self, key, int(float(val)) if key in self.INTEGERS else float(val))

	def run(self):
		if not draw_spokes(self.core, self.core.mouse_x, self.core.mouse_y, self.offset, self.count, self.radius):
			self.core.message(self.lang['nofit'])
