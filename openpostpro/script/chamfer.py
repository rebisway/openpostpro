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

# Chamfer: every corner of the selected polygons is cut by a straight line, at the given depth on each side.
# The code is in fillet.py: this script only changes the cut and the texts.

import fillet


class Cadscript(fillet.Cadscript):
	ROUNDED = False

	def texts(self):
		return {
			'en-us': {'description': 'Chamfer', 'size': 'Depth', 'nothing': 'Select lines, polylines or splines made of lines.', 'skipped': '%d corner(s) left sharp: they are too short for the depth.'},
			'fr-fr': {'description': 'Chanfrein', 'size': 'Profondeur', 'nothing': 'Sélectionnez des lignes, des polylignes ou des splines de lignes.', 'skipped': '%d coin(s) laissé(s) vif(s) : les côtés sont trop courts pour la profondeur.'}
		}
