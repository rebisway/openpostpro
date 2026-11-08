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

# SVG import: the paths, rectangles, circles, ellipses, lines, polylines and texts of an SVG file are added to the drawing.
# The lower left corner of the SVG page is at the position of the mouse. The layers of Inkscape become layers of the drawing.
# The reading is done by svgparse.py. If the file name is empty or wrong, a file dialog (tkinter) is displayed.
#
#  Scale   multiplies the size of the drawing (1 = the size written in the file, in millimeters)

import os
import drawshapes
import svgparse


class Cadscript:
	KEYS = ['file', 'scale']

	def __init__(self):
		self.file = ''
		self.scale = 1.0

		self.en_us = {'description': 'SVG import', 'file': 'File (.svg)', 'scale': 'Scale', 'dialog': 'SVG file',
			'invalid': 'This file is not an SVG file: ', 'empty': 'No shape found in this file.', 'done': 'SVG imported: %d shapes.'}
		self.fr_fr = {'description': 'Import SVG', 'file': 'Fichier (.svg)', 'scale': 'Échelle', 'dialog': 'Fichier SVG',
			'invalid': 'Ce fichier n\'est pas un fichier SVG : ', 'empty': 'Aucune forme dans ce fichier.', 'done': 'SVG importé : %d formes.'}
		self.es_es = {'description': 'Importar SVG', 'file': 'Archivo (.svg)', 'scale': 'Escala', 'dialog': 'Archivo SVG',
			'invalid': 'Este archivo no es un archivo SVG: ', 'empty': 'No hay formas en este archivo.', 'done': 'SVG importado: %d formas.'}

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
			self.scale = float(val)

	def ask_file(self):
		try:
			import tkinter
			from tkinter import filedialog
			window = tkinter.Tk()
			window.withdraw()
			window.attributes('-topmost', True)
			name = filedialog.askopenfilename(parent=window, title=self.lang['dialog'], filetypes=[('SVG', '*.svg'), ('All', '*.*')])
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
			with open(path, 'rb') as f:
				shapes = svgparse.parse(f.read(), self.scale)
		except ValueError as e:
			self.core.message(self.lang['invalid'] + str(e))
			return
		if not shapes:
			self.core.message(self.lang['empty'])
			return
		# the shapes are in millimeters
		if self.core.unit == 'inch':
			shapes = drawshapes.scale(shapes, 1.0 / 25.4)
		count = drawshapes.draw(self.core, shapes, self.core.mouse_x, self.core.mouse_y)
		self.core.log(self.lang['done'] % count)
