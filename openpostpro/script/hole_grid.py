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

# Grid of holes: circles of the given diameter, the first one at the position of the mouse.

class Cadscript:
	KEYS = ['columns', 'rows', 'pitch_x', 'pitch_y', 'diameter']
	INTEGERS = ['columns', 'rows']

	def __init__(self):
		self.columns = 4
		self.rows = 3
		self.pitch_x = 20.0
		self.pitch_y = 20.0
		self.diameter = 6.0

		self.en_us = {'description': 'Grid of holes', 'columns': 'Columns', 'rows': 'Rows', 'pitch_x': 'Pitch X', 'pitch_y': 'Pitch Y', 'diameter': 'Diameter'}
		self.fr_fr = {'description': 'Grille de trous', 'columns': 'Colonnes', 'rows': 'Lignes', 'pitch_x': 'Entraxe X', 'pitch_y': 'Entraxe Y', 'diameter': 'Diamètre'}

		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'CAD'

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
		r = self.diameter / 2
		for row in range(self.rows):
			for col in range(self.columns):
				self.core.circle(self.core.mouse_x + col * self.pitch_x, self.core.mouse_y + row * self.pitch_y, r)
