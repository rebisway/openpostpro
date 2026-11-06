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

# Star: a closed polyline with the given number of branches, centered on the mouse, one branch pointing up.
# Diameter is the diameter of the circle through the tips. The inner points are the ones of the regular star
# polygon (the sides of a 5 branches star are the lines of a pentagram); with 3 or 4 branches they are at half the radius.

import math


class Cadscript:
	KEYS = ['branches', 'diameter']

	def __init__(self):
		self.branches = 5
		self.diameter = 100.0

		self.en_us = {'description': 'Star', 'branches': 'Branches', 'diameter': 'Diameter', 'invalid': 'A star needs at least 3 branches and a diameter above 0.'}
		self.fr_fr = {'description': 'Étoile', 'branches': 'Branches', 'diameter': 'Diamètre', 'invalid': 'Une étoile demande au moins 3 branches et un diamètre supérieur à 0.'}
		self.es_es = {'description': 'Estrella', 'branches': 'Puntas', 'diameter': 'Diámetro', 'invalid': 'Una estrella necesita al menos 3 puntas y un diámetro mayor que 0.'}

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
		if index == 0:
			self.branches = int(float(val))
		else:
			self.diameter = float(val)

	def run(self):
		n = self.branches
		if n < 3 or self.diameter <= 0:
			self.core.message(self.lang['invalid'])
			return
		outer = self.diameter / 2
		inner = outer * (math.cos(2 * math.pi / n) / math.cos(math.pi / n) if n >= 5 else 0.5)
		points = []
		for i in range(2 * n):
			r = outer if i % 2 == 0 else inner
			a = math.pi / 2 + math.pi * i / n
			points.append((self.core.mouse_x + r * math.cos(a), self.core.mouse_y + r * math.sin(a)))
		points.append(points[0])		# closed
		self.core.polyline(points, 'polyline')
