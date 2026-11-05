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

# Example of a script working on the selection: a small circle is drawn at the middle of each selected line.

class Cadscript:
	def __init__(self):
		self.radius = 2.0

		self.en_us = {'description': 'Mark the middle of lines', 'radius': 'Radius'}
		self.fr_fr = {'description': 'Marquer le milieu des lignes', 'radius': 'Rayon'}

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

	# one property: the dialog box is displayed before the script runs
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
		# self.core.selection is the list of the selected shapes, each one is a dictionary
		# with at least 'id' and 'type'; the other keys depend on the type
		for shape in self.core.selection:
			if shape['type'] == 'line':
				x1, y1 = shape['p1']
				x2, y2 = shape['p2']
				self.core.circle((x1 + x2) / 2, (y1 + y2) / 2, self.radius)
