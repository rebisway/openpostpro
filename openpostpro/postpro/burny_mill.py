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

# Burny 2.5 mill post-processor
# Relative arc centers, dwell time given with F, no canned cycle cancel.
# The file adapts Burny2.5_mill.py of GGCad: only what differs from the generic post-processor is written here.

from generic_mill import Postpro as Generic


class Postpro(Generic):
	def __init__(self):
		super().__init__()
		# the name displayed in the menu
		self.en_us.update({'description': 'Burny 2.5'})
		self.fr_fr.update({'description': 'Burny 2.5'})
		self.es_es.update({'description': 'Burny 2.5'})

		self.relative = True		# IJK relative

	def use_safe_pos_before_tool_change(self):
		return True

	# the dwell time is given with F
	def pause(self, time):
		return self.code('G4') + 'F' + self.n(time)

	# the Burny does not know G80
	def cancel_canned_cycle(self):
		return ''
