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

# Num 1060 mill post-processor
# Spaces between words, X Y Z on every move, relative arc centers.
# The file adapts Num1060_mill.py of GGCad: only what differs from the generic post-processor is written here.

from generic_mill import Postpro as Generic


class Postpro(Generic):
	def __init__(self):
		super().__init__()
		# the name displayed in the menu
		self.en_us.update({'description': 'Num 1060'})
		self.fr_fr.update({'description': 'Num 1060'})
		self.es_es.update({'description': 'Num 1060'})

		# not condensed: a space between the words
		self.cond = False
		self.space = ' '
		self.relative = True		# IJK relative

	# the machine wants X Y Z on every move
	def pass_xy_to_cw_ccw(self):
		return True

	def pass_z_to_cw_ccw(self):
		return True

	def pass_xy_to_linear(self):
		return True

	def pass_z_to_linear(self):
		return True

	def use_safe_pos_before_tool_change(self):
		return True

	# the dwell time is given with F
	def pause(self, time):
		return self.code('G4') + 'F' + self.n(time)

	# no G64 (constant speed): the Num 1060 does not know it
	def start_program(self):
		unit = 'G20' if self.unit == '1' else 'G21'
		return self.code('G90') + self.code('G94') + self.code(unit)
