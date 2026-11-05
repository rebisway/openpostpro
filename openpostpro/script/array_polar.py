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

# Polar array: the selected shapes are copied "Copies" times around the position of the mouse, each copy turned by the angle step.

import copyshape
import geom

class Cadscript:
	KEYS = ['count', 'step']
	INTEGERS = ['count']

	def __init__(self):
		self.count = 5
		self.step = 60.0

		self.en_us = {'description': 'Polar array of the selection', 'nothing': 'Select the shapes to copy.', 'count': 'Copies', 'step': 'Angle step (deg)'}
		self.fr_fr = {'description': 'Réseau polaire de la sélection', 'nothing': 'Sélectionnez les formes à copier.', 'count': 'Copies', 'step': 'Pas angulaire (deg)'}

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
		if len(self.core.selection) == 0:
			self.core.message(self.lang['nothing'])
			return
		center = (self.core.mouse_x, self.core.mouse_y)
		for i in range(1, self.count + 1):
			a = self.step * i
			for shape in self.core.selection:
				copyshape.copy_shape(self.core, shape, lambda p: geom.rotate(p, center, a), a)
