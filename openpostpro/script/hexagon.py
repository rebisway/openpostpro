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

# Hexagon: a closed regular polygon with 6 sides, centered on the mouse. Radius is the distance from the center to a corner.

import geom


class Cadscript:
	def __init__(self):
		self.radius = 50.0

		self.en_us = {'description': 'Hexagon', 'radius': 'Radius'}
		self.fr_fr = {'description': 'Hexagone', 'radius': 'Rayon'}
		self.es_es = {'description': 'Hexágono', 'radius': 'Radio'}

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
		return 1

	def get_property_key(self, index):
		return 'radius'

	def get_property_description(self, index):
		return self.lang['radius']

	def get_property(self, index):
		return self.radius

	def set_property(self, index, val):
		self.radius = float(val)

	def run(self):
		center = (self.core.mouse_x, self.core.mouse_y)
		first = (center[0] + self.radius, center[1])
		points = [geom.rotate(first, center, 60 * i) for i in range(6)]
		points.append(points[0])		# closed
		self.core.polyline(points, 'polyline')
