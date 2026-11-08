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

# Potrace: traces a bitmap (.bmp) into closed polylines, using the external program potrace (http://potrace.sourceforge.net).
# potrace writes an SVG file, which is read here: the curves are cut into short segments.
# Properties: the path of potrace (found in the PATH if empty), the bitmap (a file dialog is displayed if empty) and
# the width of the drawing. The lower left corner of the bitmap is at the mouse.

import os
import re
import shutil
import subprocess
import tempfile

STEPS = 12		# segments per curve


def bezier(p0, p1, p2, p3):
	pts = []
	for i in range(1, STEPS + 1):
		t = i / STEPS
		u = 1 - t
		pts.append((u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
			u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]))
	return pts


# closed polylines (lists of points) of the paths of a potrace SVG and the width of the image in svg units, in the
# units of the path data (y up). Returns (contours, width) or None.
def parse_svg(text):
	m = re.search(r'<svg[^>]*\swidth="([\d.]+)', text)
	t = re.search(r'scale\(\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\)', text)
	if not m:
		return None
	sx = abs(float(t.group(1))) if t else 1.0
	sy = abs(float(t.group(2))) if t else 1.0
	contours = []
	for d in re.findall(r'\sd="([^"]*)"', text):
		tokens = re.findall(r'[a-zA-Z]|[-+]?(?:\d+\.?\d*|\.\d+)(?:e[-+]?\d+)?', d)
		i = 0
		cmd = ''
		cur = (0.0, 0.0)
		start = cur
		pts = []

		def flush():
			if len(pts) > 2:
				contours.append(list(pts) + [pts[0]])

		def num(k):
			return float(tokens[i + k])

		while i < len(tokens):
			if tokens[i].isalpha():
				cmd = tokens[i]
				i += 1
				if cmd in 'zZ':
					flush()
					pts = []
					cur = start
				continue
			rel = cmd.islower()
			ox, oy = cur if rel else (0.0, 0.0)
			c = cmd.lower()
			if c == 'm':
				flush()
				cur = (ox + num(0), oy + num(1))
				start = cur
				pts = [cur]
				i += 2
				cmd = 'l' if rel else 'L'		# next pairs are lines
			elif c == 'l':
				cur = (ox + num(0), oy + num(1))
				pts.append(cur)
				i += 2
			elif c == 'h':
				cur = (ox + num(0), cur[1])
				pts.append(cur)
				i += 1
			elif c == 'v':
				cur = (cur[0], oy + num(0))
				pts.append(cur)
				i += 1
			elif c == 'c':
				p1 = (ox + num(0), oy + num(1))
				p2 = (ox + num(2), oy + num(3))
				p3 = (ox + num(4), oy + num(5))
				pts += bezier(cur, p1, p2, p3)
				cur = p3
				i += 6
			else:
				return None		# unknown command
		flush()
	if not contours:
		return None
	contours = [[(x * sx, y * sy) for x, y in c] for c in contours]
	return contours, float(m.group(1))


class Cadscript:
	def __init__(self):
		self.potrace = ''
		self.file = ''
		self.width = 100.0

		self.en_us = {'description': 'Import a bitmap (Potrace)', 'potrace': 'Potrace.exe path', 'file': 'Bitmap file (*.bmp)', 'width': 'Width of the drawing',
			'missing': 'Potrace was not found. Download it from http://potrace.sourceforge.net and set its path.', 'failed': 'Potrace could not trace this file.', 'dialog': 'Bitmap file', 'exe': 'Potrace program', 'nobitmap': 'Choose an existing bitmap file.', 'badwidth': 'The width must be greater than 0.', 'cancel': 'Cancel'}
		self.fr_fr = {'description': 'Importer une image (Potrace)', 'potrace': 'Chemin de Potrace.exe', 'file': 'Fichier image (*.bmp)', 'width': 'Largeur du dessin',
			'missing': 'Potrace est introuvable. Téléchargez-le sur http://potrace.sourceforge.net et indiquez son chemin.', 'failed': 'Potrace n\'a pas pu vectoriser ce fichier.', 'dialog': 'Fichier image', 'exe': 'Programme Potrace', 'nobitmap': 'Choisissez un fichier image existant.', 'badwidth': 'La largeur doit être supérieure à 0.', 'cancel': 'Annuler'}
		self.es_es = {'description': 'Importar una imagen (Potrace)', 'potrace': 'Ruta de Potrace.exe', 'file': 'Archivo de imagen (*.bmp)', 'width': 'Ancho del dibujo',
			'missing': 'No se encuentra Potrace. Descárguelo de http://potrace.sourceforge.net e indique su ruta.', 'failed': 'Potrace no pudo vectorizar este archivo.', 'dialog': 'Archivo de imagen', 'exe': 'Programa Potrace', 'nobitmap': 'Elija un archivo de imagen existente.', 'badwidth': 'El ancho debe ser mayor que 0.', 'cancel': 'Cancelar'}

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

	# the script displays its own dialog box (tkinter): no properties dialog box
	def use_properties(self):
		return False

	def get_properties_count(self):
		return 0

	def get_property_description(self, index):
		return ''

	def get_property(self, index):
		return None

	def set_property(self, index, val):
		pass

	# file where the path of potrace is kept
	def ini_path(self):
		return os.path.join(os.path.expanduser('~'), '.openpostpro', 'potrace.ini')

	def read_ini(self):
		try:
			with open(self.ini_path(), 'r', encoding='utf-8') as f:
				self.potrace = f.readline().strip()
		except OSError:
			pass
		if not (self.potrace and os.path.isfile(self.potrace)):
			self.potrace = shutil.which('potrace') or ''

	def write_ini(self):
		try:
			os.makedirs(os.path.dirname(self.ini_path()), exist_ok=True)
			with open(self.ini_path(), 'w', encoding='utf-8') as f:
				f.write(self.potrace)
		except OSError:
			pass

	def trace(self, exe, path):
		out = tempfile.NamedTemporaryFile(suffix='.svg', delete=False)
		out.close()
		try:
			flags = getattr(subprocess, 'CREATE_NO_WINDOW', 0)
			subprocess.run([exe, '-b', 'svg', path, '-o', out.name], check=True, capture_output=True, creationflags=flags)
			with open(out.name, 'r', errors='replace') as f:
				return f.read()
		except (OSError, subprocess.CalledProcessError):
			return None
		finally:
			try:
				os.remove(out.name)
			except OSError:
				pass

	# dialog box: returns False if cancelled. Ok is accepted when potrace and the bitmap exist.
	def ask(self):
		import tkinter as tk
		from tkinter import messagebox
		t = self.lang
		window = tk.Tk()
		window.title(t['description'])
		window.attributes('-topmost', True)
		exe = tk.StringVar(value=self.potrace)
		bmp = tk.StringVar(value=self.file)
		width = tk.StringVar(value=str(self.width))
		state = {'ok': False}

		def browse(var, title, types):
			name = self.core.open_file(title, types, var.get())
			if name:
				var.set(name)

		def validate():
			try:
				w = float(width.get().replace(',', '.'))
			except ValueError:
				w = 0.0
			if not os.path.isfile(exe.get()):
				messagebox.showwarning(t['description'], t['missing'], parent=window)
			elif not os.path.isfile(bmp.get()):
				messagebox.showwarning(t['description'], t['nobitmap'], parent=window)
			elif w <= 0:
				messagebox.showwarning(t['description'], t['badwidth'], parent=window)
			else:
				self.potrace, self.file, self.width = exe.get(), bmp.get(), w
				state['ok'] = True
				window.quit()

		frame = tk.Frame(window)
		frame.pack(padx=8, pady=8)
		tk.Label(frame, text=t['potrace'], anchor='w').grid(row=0, column=0, sticky='w')
		tk.Entry(frame, textvariable=exe, width=50).grid(row=0, column=1, padx=4)
		tk.Button(frame, text='...', command=lambda: browse(exe, t['exe'], [('Potrace', '*.exe'), ('All', '*.*')])).grid(row=0, column=2)
		tk.Label(frame, text=t['file'], anchor='w').grid(row=1, column=0, sticky='w')
		tk.Entry(frame, textvariable=bmp, width=50).grid(row=1, column=1, padx=4, pady=4)
		tk.Button(frame, text='...', command=lambda: browse(bmp, t['dialog'], [('Bitmap', '*.bmp'), ('All', '*.*')])).grid(row=1, column=2)
		tk.Label(frame, text=t['width'], anchor='w').grid(row=2, column=0, sticky='w')
		tk.Entry(frame, textvariable=width, width=12).grid(row=2, column=1, sticky='w', padx=4)
		buttons = tk.Frame(window)
		buttons.pack(pady=(0, 8))
		tk.Button(buttons, text='Ok', width=10, command=validate).pack(side='left', padx=4)
		tk.Button(buttons, text=t['cancel'], width=10, command=window.quit).pack(side='left', padx=4)
		window.protocol('WM_DELETE_WINDOW', window.quit)
		window.update_idletasks()
		window.geometry('+%d+%d' % ((window.winfo_screenwidth() - window.winfo_width()) // 2, (window.winfo_screenheight() - window.winfo_height()) // 2))
		window.mainloop()
		window.destroy()
		return state['ok']

	def run(self):
		self.read_ini()
		if not self.ask():
			return
		self.write_ini()
		text = self.trace(self.potrace, self.file)
		result = parse_svg(text) if text else None
		if result is None:
			self.core.message(self.lang['failed'])
			return
		contours, svg_width = result
		# the path data is in pt after the scale of the svg, like its width: one image width = svg_width
		k = self.width / svg_width
		for c in contours:
			self.core.polyline([(self.core.mouse_x + x * k, self.core.mouse_y + y * k) for x, y in c], 'polyline')
