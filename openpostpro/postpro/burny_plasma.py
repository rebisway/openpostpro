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

# Burny 2.5 plasma post-processor
# Every arc gives X Y I J, dwell time given with F.
# The file adapts Burny2.5_plasma.py of GGCad: only what differs from the generic post-processor is written here.

from generic_plasma import Postpro as Generic


class Postpro(Generic):
	def __init__(self):
		super().__init__()
		# the name displayed in the menu
		self.en_us.update({'description': 'Burny 2.5'})
		self.fr_fr.update({'description': 'Burny 2.5'})
		self.es_es.update({'description': 'Burny 2.5'})

		self.last_ij = [0.0, 0.0]

	# the dwell time is given with F
	def pause(self, time):
		return self.code('G4') + 'F' + self.n(time)

	# X Y I J on every arc: a missing X, Y is the current position, a missing I, J is the previous value
	# (zero when the centers are relative)
	def arc_coords(self, axes, values):
		data = {'X': self.core.x, 'Y': self.core.y, 'I': 0.0 if self.relative else self.last_ij[0], 'J': 0.0 if self.relative else self.last_ij[1]}
		for axis, value in zip(axes, values):
			if axis in data:
				data[axis] = value
		self.last_ij = [data['I'], data['J']]
		return ''.join(axis + self.n(data[axis]) + self.space for axis in ('X', 'Y', 'I', 'J')).strip()

	def clockwise(self, axes, values):
		return self.code('G2') + self.arc_coords(axes, values)

	def counter_clockwise(self, axes, values):
		return self.code('G3') + self.arc_coords(axes, values)
