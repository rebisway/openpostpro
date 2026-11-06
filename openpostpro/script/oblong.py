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

import tkinter as tk

class Cadscript:
    # class constructor (always present). If empty, add pass key word. Define the variables here.
	def __init__(self):
		self.radius = 10.0
		self.length = 100.0

    		# Dictionary to manage user interface strings
		self.en_us = {'description': 'Oblong', 'radius': 'Radius', 'length': 'Length'}
		self.fr_fr = {'description': 'Oblong', 'radius': 'Rayon', 'length': 'Longueur'}
		self.es_es = {'description': 'Oblongo', 'radius': 'Radio', 'length': 'Longitud'}

		# Langue used
		self.Language = {'en-us' : self.en_us, 'fr-fr' : self.fr_fr, 'es-es': self.es_es }
	
	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	#########################################
	# default methods of the post-processor #
	#########################################

	# return menu description : [Scripts][Category][Description]
	def get_description(self):
		return self.lang['description'] #"This is the default generic postprocessor for mill type machine"

	# return menu description : [Scripts][Category][Description]
	def get_category(self):
		return 'generation'

	# set the reference to the cad loop object, do not modify
	def set_core(self, value):
		self.core = value

	#############################################################################
	# the next methods manage the properties (parameters) of the post-processor #
	#############################################################################
	
	# return if we use default configuration box for properties, otherwise method 'configurate' will be called
	def use_properties(self):
		return False	# this script displays its own dialog box (tkinter)
	
	# return the number of properties (parameters)
	def get_properties_count(self):
		return 2
	
		# return the property description (the text displayed in the property box)
	def get_property_description(self, index):
		if index == 0:
			return self.lang['radius']
		elif index == 1:
			return self.lang['length']

	# return the property value
	def get_property(self, index):
		if index == 0:
			return self.radius
		elif index == 1:
			return self.length
		
	# set the property value
	def set_property(self, index, val):
		if index == 0:
			self.radius = float(val)
		elif index == 1:
			self.length = float(val)

	#############################################################################
	# the next methods runs the script #
	#############################################################################
	def configurate(self):
		pass
	
	def run(self):
		self.window = tk.Tk()
		self.window.wm_attributes('-toolwindow', 'true')
 
		frame=tk.Frame(self.window)

		frame1 = tk.Frame(frame)
		frame1.pack()

		label1 = tk.Label(
			frame1,
			text=self.get_property_description(0),
			justify=tk.LEFT,
			anchor="w"
		).grid(
			sticky="W", row=0, column=0
		)

		self.r = tk.StringVar()
		self.r.set(str(self.radius))
		radius =tk.Entry(
			frame1, 
			textvariable = self.r
			).grid(
				row = 0, column = 1
			)
 
		label2 = tk.Label(
			frame1,
			text=self.get_property_description(1),
			justify=tk.LEFT,
			anchor="w"
		).grid(
			sticky="W", row=1, column=0
			)

		self.l = tk.StringVar()
		self.l.set(str(self.length))
		length = tk.Entry(
			frame1, 
			textvariable = self.l
			)
		length.grid(
			row = 1, 
			column = 1
			)

		button = tk.Button(
			frame,
			text="Ok",
			command = self.window.quit
		)
		button.pack(pady=5)
		frame.pack(padx=5, pady=5)

		self.window.title(self.get_description())
		self.window.protocol("WM_DELETE_WINDOW", self.on_closing)
		self.center(self.window)
		self.window.mainloop()
		
		if self.window != 0:
			self.radius = float(self.r.get())
			self.length = float(self.l.get())
			self.compute()
			#self.core.append_output(str(self.radius) + '-' + str(self.length))

		self.on_closing()

	def on_closing(self):
		if self.window != 0:
			self.window.destroy()
			self.window = 0

	def center(self, window):
		window.update_idletasks()
		width = window.winfo_width()
		height = window.winfo_height()
		screen_width = window.winfo_screenwidth()
		screen_height = window.winfo_screenheight()
		x = (screen_width - width) // 2
		y = (screen_height - height) // 2
		window.geometry(f"{width}x{height}+{x}+{y}")

	def compute(self):
		
		#ARC1
		acx1 = -(self.length / 2) + self.radius + self.core.mouse_x
		acy1 = 0                                + self.core.mouse_y
		asx1 = acx1                             
		asy1 = -self.radius                     + self.core.mouse_y
		adx1 = acx1                             
		ady1 = +self.radius                     + self.core.mouse_y

		#ARC2
		acx2 = (self.length / 2) - self.radius  + self.core.mouse_x
		acy2 = 0                                + self.core.mouse_y
		asx2 = acx2                             
		asy2 = +self.radius                     + self.core.mouse_y
		adx2 = acx2                             
		ady2 = -self.radius                     + self.core.mouse_y

		#LINE1
		lsx1 = adx1
		lsy1 = ady1
		ldx1 = asx2
		ldy1 = asy2

		#LINE2
		lsx2 = adx2
		lsy2 = ady2
		ldx2 = asx1
		ldy2 = asy1

		self.core.arc(asx1, asy1, acx1, acy1, adx1, ady1, True)
		self.core.line(lsx1, lsy1, ldx1, ldy1)
		self.core.arc(asx2, asy2, acx2, acy2, adx2, ady2, True)
		self.core.line(lsx2, lsy2, ldx2, ldy2)
