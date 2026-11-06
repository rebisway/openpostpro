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

# Example of a script that modifies the selection: the radius of each selected circle is multiplied by a factor.
# The circles keep their id (and so their toolpaths), the whole change is undone in one step.

class Cadscript:
	def __init__(self):
		self.factor = 1.5

		self.en_us = {'description': 'Scale the selected circles', 'factor': 'Factor'}
		self.fr_fr = {'description': 'Agrandir les cercles sélectionnés', 'factor': 'Facteur'}

		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'examples'

	# set the reference to the cad core object, do not modify
	def set_core(self, value):
		self.core = value

	def get_properties_count(self):
		return 1

	def get_property_key(self, index):
		return 'factor'

	def get_property_description(self, index):
		return self.lang['factor']

	def get_property(self, index):
		return self.factor

	def set_property(self, index, val):
		self.factor = float(val)

	def run(self):
		for shape in self.core.selection:
			if shape['type'] == 'circle':
				x, y = shape['center']
				self.core.modify_circle(shape['id'], x, y, shape['radius'] * self.factor)
