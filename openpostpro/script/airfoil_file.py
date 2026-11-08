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

# Airfoil from a coordinate file (Selig or Lednicer .dat). The profile is scaled to the chord, its leading edge is
# at the mouse and its chord lies on the X axis. If the file is empty or missing, a file dialog (tkinter) is displayed.
#
# Selig:    a title line, then the points from the trailing edge along the upper side to the leading edge and back
#           along the lower side.
# Lednicer: a title line, a line with the number of points of each side, then the upper side and the lower side,
#           both from the leading edge to the trailing edge.

import os


def numbers(line):
	try:
		return [float(v) for v in line.replace(',', ' ').split()]
	except ValueError:
		return None


# list of (x, y) of the profile from a text, None if nothing can be read
def parse(text):
	rows = []
	for line in text.splitlines():
		v = numbers(line)
		if v is not None and len(v) == 2:
			rows.append((v[0], v[1]))
		elif v is None or len(v) == 0:
			if rows:
				rows.append(None)		# blank or text line after data: keep a mark
	rows = [r for r in rows]
	if not rows:
		return None
	clean = [r for r in rows if r is not None]
	if clean and clean[0][0] > 1.5 and clean[0][1] > 1.5:
		# Lednicer: the first line is the number of points of each side
		n1, n2 = int(clean[0][0]), int(clean[0][1])
		data = clean[1:]
		if len(data) < n1 + n2:
			return None
		upper = data[:n1]
		lower = data[n1:n1 + n2]
		points = upper[::-1] + lower[1:]
	else:
		points = clean
	return points if len(points) >= 3 else None


class Cadscript:
	KEYS = ['file', 'chord', 'smooth']

	def __init__(self):
		self.file = ''
		self.chord = 100.0
		self.smooth = True

		self.en_us = {'description': 'Airfoil from a file', 'file': 'File (.dat)', 'chord': 'Chord', 'smooth': 'Smooth curve',
			'invalid': 'This file does not contain airfoil coordinates.', 'dialog': 'Airfoil file'}
		self.fr_fr = {'description': 'Profil d\'aile depuis un fichier', 'file': 'Fichier (.dat)', 'chord': 'Corde', 'smooth': 'Courbe lissée',
			'invalid': 'Ce fichier ne contient pas de coordonnées de profil.', 'dialog': 'Fichier de profil'}
		self.es_es = {'description': 'Perfil alar desde un archivo', 'file': 'Archivo (.dat)', 'chord': 'Cuerda', 'smooth': 'Curva suave',
			'invalid': 'Este archivo no contiene coordenadas de perfil.', 'dialog': 'Archivo de perfil'}

		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'import'

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
		if key == 'file':
			self.file = str(val)
		elif key == 'smooth':
			self.smooth = val if isinstance(val, bool) else str(val).strip().lower() in ('1', 'true', 'yes')
		else:
			self.chord = float(val)

	# the property "file" has a button which opens the file dialog of the application
	def get_property_file(self, index):
		return ('open', 'Airfoil', '*.dat;*.txt') if index == 0 else None

	def ask_file(self):
		return self.core.open_file(self.lang['dialog'], [('Airfoil', '*.dat;*.txt'), ('All', '*.*')])

	def run(self):
		path = self.file
		if not os.path.isfile(path):
			path = self.ask_file()
			if not path:
				return
		with open(path, 'r', errors='replace') as f:
			points = parse(f.read())
		if points is None:
			self.core.message(self.lang['invalid'])
			return
		x0 = min(p[0] for p in points)
		width = max(p[0] for p in points) - x0
		if width <= 0:
			self.core.message(self.lang['invalid'])
			return
		k = self.chord / width
		pts = [(self.core.mouse_x + (x - x0) * k, self.core.mouse_y + y * k) for x, y in points]
		if pts[0] != pts[-1]:
			pts.append(pts[0])		# closed: the trailing edge is joined
		self.core.polyline(pts, 'catmull' if self.smooth else 'polyline')
