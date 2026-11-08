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

# Mach3 mill post-processor
# Mach3 reads the .tap extension and absolute arc centers (G90.1).
# The file adapts Mach3_mill.py of GGCad: only what differs from the generic post-processor is written here.

from generic_mill import Postpro as Generic


class Postpro(Generic):
	def __init__(self):
		super().__init__()
		# the name displayed in the menu
		self.en_us.update({'description': 'Mach3'})
		self.fr_fr.update({'description': 'Mach3'})
		self.es_es.update({'description': 'Mach3'})

		self.ext = 'tap'
		self.relative = False		# IJK absolute

	def use_safe_pos_before_tool_change(self):
		return True

	# G90.1 / G91.1 tell Mach3 if the arc centers are absolute or incremental
	def start_program(self):
		unit = 'G20' if self.unit == '1' else 'G21'
		arcs = 'G91.1' if self.relative else 'G90.1'
		return self.code('G90') + self.code(arcs) + self.code('G64') + self.code('G94') + self.code(unit)
