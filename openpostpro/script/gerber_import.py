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

# Gerber and Excellon import: the pads, the tracks, the regions and the holes of a PCB file are added to the drawing.
# The origin of the file (0, 0) is at the position of the mouse. The reading is done by gerberparse.py, the drawing by drawshapes.py.
# If the file name is empty or wrong, a file dialog (tkinter) is displayed.
#
#  Tracks as outlines   yes: each track is a closed shape of its width (for isolation routing with an offset), no: its center line
#
# The shapes go into a layer which has the name of the file. The shapes of a clear polarity are in the layer "<name>_clear": they are not
# subtracted from the dark ones. The overlapping shapes are not merged: the union of the closed shapes is done in the application.

import os
import re
import drawshapes
import gerberparse


class Cadscript:
	KEYS = ['file', 'outline']

	def __init__(self):
		self.file = ''
		self.outline = True

		self.en_us = {'description': 'Gerber / Excellon import', 'file': 'File (.gbr, .drl...)', 'outline': 'Tracks as outlines', 'dialog': 'Gerber or Excellon file',
			'empty': 'No shape found in this file.', 'done': 'Gerber imported: %d shapes.', 'warning': 'Gerber: ', 'error': 'The file can not be read: '}
		self.fr_fr = {'description': 'Import Gerber / Excellon', 'file': 'Fichier (.gbr, .drl...)', 'outline': 'Pistes en contours', 'dialog': 'Fichier Gerber ou Excellon',
			'empty': 'Aucune forme dans ce fichier.', 'done': 'Gerber importé : %d formes.', 'warning': 'Gerber : ', 'error': 'Le fichier ne peut pas être lu : '}
		self.es_es = {'description': 'Importar Gerber / Excellon', 'file': 'Archivo (.gbr, .drl...)', 'outline': 'Pistas como contornos', 'dialog': 'Archivo Gerber o Excellon',
			'empty': 'No hay formas en este archivo.', 'done': 'Gerber importado: %d formas.', 'warning': 'Gerber: ', 'error': 'No se puede leer el archivo: '}

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
		if self.KEYS[index] == 'file':
			self.file = str(val)
		else:
			self.outline = val if isinstance(val, bool) else str(val).strip().lower() in ('1', 'true', 'yes')

	def ask_file(self):
		try:
			import tkinter
			from tkinter import filedialog
			window = tkinter.Tk()
			window.withdraw()
			window.attributes('-topmost', True)
			name = filedialog.askopenfilename(parent=window, title=self.lang['dialog'],
				filetypes=[('Gerber / Excellon', '*.gbr *.ger *.gtl *.gbl *.gto *.gbo *.gts *.gbs *.gko *.drl *.xln *.txt'), ('All', '*.*')])
			window.destroy()
			return name
		except Exception:
			return ''

	def run(self):
		path = self.file
		if not os.path.isfile(path):
			path = self.ask_file()
			if not path:
				return
		try:
			with open(path, 'r', errors='replace') as f:
				parsed = gerberparse.parse(f.read())
		except OSError as e:
			self.core.message(self.lang['error'] + str(e))
			return

		name = re.sub(r'[^A-Za-z0-9_.-]+', '_', os.path.splitext(os.path.basename(path))[0]) or 'Gerber'
		shapes = gerberparse.convert(parsed, self.outline, name)
		if not shapes:
			self.core.message(self.lang['empty'])
			return
		# the shapes are in millimeters
		k = 1.0 / 25.4 if self.core.unit == 'inch' else 1.0
		if k != 1.0:
			shapes = drawshapes.scale(shapes, k)
		count = drawshapes.draw(self.core, shapes, self.core.mouse_x, self.core.mouse_y)
		self.core.log(self.lang['done'] % count)
		for warning in sorted(set(parsed.warnings)):
			self.core.log(self.lang['warning'] + warning)

