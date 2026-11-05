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

class Postpro:
	# class constructor (always present). If empty, add pass key word. Define the variables here.
	def __init__(self):
		# incrementation of numbers at the begining of each line
		self.use_inc = False	# if false, the numbers are not append to lines
		self.increment = 5		# value of the increment for line number

		# clarity		
		self.use_comment = True # if true generates comments
		self.cond = True		# if true condensate the output by removing spaces
		
		# decimals
		self.decim = 6			# max number of decimales for real values
		
		# file
		self.ext = 'nc'         # file extention
		
		# Coordinates
		self.relative = True    # if true, 'I', 'J', 'K' coordinates will be relative to current coordinates
		self.unit = '0'	        # set the coordinates in millimeters or inches

		# Program
		self.pre_data = '%'		# first line written
		self.post_data = '%'	# last line written
		
		self.space = ''			# store a space character if not condensate mode
		self.command_line = ''
		self.last_r = ''
		self.last_q = ''
		self.last_code = ''		# last code used

		self.safe = 5.0			# safe z position
		self.xy_rapid_feedrate = 2000
		self.z_rapid_feedrate = 1000

		# Dictionary to manage user interface strings
		self.en_us = {'description': 'Generic Mill/Router', 'increment': 'Increment', 'use_inc': 'Numbering lines', 'cond': 'Condensed', 'decimales': 'Decimales number', 'ext': 'File extension', 'use_comment': 'Output comments', 'pre_data': 'File header', 'post_data': 'File footer', 'relative': 'IJK Relative', 'unit': 'Unit', 'millimeter': 'Millimeter', 'inch': 'Inch'}
		self.fr_fr = {'description': 'Mill/Router Générique', 'increment': 'Incrémentation', 'use_inc': 'Numéroter les lignes', 'cond': 'Condensé', 'decimales': 'Nombre de décimales', 'ext': 'Extension du fichier', 'use_comment': 'Générer les commentaires', 'pre_data': 'Début de fichier', 'post_data': 'Fin du fichier', 'relative': 'IJK Relatif', 'unit': 'Unité', 'millimeter': 'Millimètre', 'inch': 'Pouce'}
		self.es_es = {'description': 'Mill/Router Generico', 'increment': 'Incrementacion', 'use_inc': 'Numeracion de las lineas', 'cond': 'Condensado', 'decimales': 'Decimales', 'ext': 'Extencion del archivo', 'use_comment': 'Generar los comentarios', 'pre_data': 'Principio', 'post_data': 'Fin', 'relative': 'IJK Relativo', 'unit': 'Unidad', 'millimeter': 'Milímetro', 'inch': 'Pulgar'}

		# Langue used
		self.Language = {'en-us' : self.en_us, 'fr-fr' : self.fr_fr, 'es-es': self.es_es }
	
	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	# the unit of the document, 'mm' or 'inch', set by the application before the generation
	def set_unit(self, unit):
		self.unit = '1' if unit == 'inch' else '0'

	#########################################
	# default methods of the post-processor #
	#########################################

	# return menu description : [Post-processor][Select][Category][Description]
	def get_description(self):
		return self.lang['description'] #"This is the default generic postprocessor for mill type machine"

	# return menu category : menu [Post-processor][Select][Category][Description]
	def get_category(self):
		return "MILL"
		
	# return the output file extention
	def get_extention(self):
		return self.ext
	
	# return the maximum number of decimal digit
	def get_decimal(self):
		return self.decim

	#return the increment value for line numbering, -1 if there is no numbering
	def incrementation(self):
		if self.use_inc:
			return self.increment
		return -1
	
	# set the reference to the post core object, do not modify
	def set_core(self, value):
		self.core = value

	def set_safe_position(self, value):
		self.safe = value

	# return true if always pass X Y to G2 or G3
	def pass_xy_to_cw_ccw(self):
		return False
			
	# return true if always pass Z to G2 or G3
	def pass_z_to_cw_ccw(self):
		return False
		
	# return true if always pass X Y to G0 or G1
	def pass_xy_to_linear(self):
		return False	
		
	# return true if always pass Z to G0 or G1
	def pass_z_to_linear(self):
		return False	
		
	# return true if lift tool to safe position before changing tool
	def use_safe_pos_before_tool_change(self):
		return False	
	
	# return true if Z axe is used
	def get_disable_z(self):
		return False
		
	# return true if arc center is relative to the destination point
	# or false if the center requested is absolute
	def ijk_relative(self):
		return self.relative

	#############################################################################
	# the next methods manage the properties (parameters) of the post-processor #
	#############################################################################
	
	# return the number of properties (parameters)
	def get_properties_count(self):
		return 9
	
		# return the property description (the text displayed in the property box)
	def get_property_description(self, index):
		if index == 0:
			return self.lang['use_inc']
		elif index == 1:
			return self.lang['increment']
		elif index == 2:
			return self.lang['cond']
		elif index == 3:
			return self.lang['decimales']
		elif index == 4:
			return self.lang['ext']
		elif index == 5:
			return self.lang['use_comment']
		elif index == 6:
			return self.lang['pre_data']
		elif index == 7:
			return self.lang['post_data']
		elif index == 8:
			return self.lang['relative']

	# return the name used to save the property value, it does not change with the language or the order
	def get_property_key(self, index):
		return ['use_inc', 'increment', 'cond', 'decimales', 'ext', 'use_comment', 'pre_data', 'post_data', 'relative'][index]

	# return the property value
	def get_property(self, index):
		if index == 0:
			return self.use_inc
		elif index == 1:
			return self.increment
		elif index == 2:
			return self.cond
		elif index == 3:
			return self.decim
		elif index == 4:
			return self.ext
		elif index == 5:
			return self.use_comment
		elif index == 6:
			return self.pre_data
		elif index == 7:
			return self.post_data
		elif index == 8:
			return self.relative
		
	# set the property value
	def set_property(self, index, val):
		if index == 0:
			self.use_inc = val
		elif index == 1:
			self.increment = int(val)
		elif index == 2:
			self.cond = val
			if self.cond:
				self.space = ''
			else:
				self.space = ' '
		elif index == 3:
			self.decim = int(val)
		elif index == 4:
			self.ext = val
		elif index == 5:
			self.use_comment = val
		elif index == 6:
			self.pre_data = val
		elif index == 7:
			self.post_data = val
		elif index == 8:
			self.relative = val
	
	##############################################
	#              Helper definitions            #
	##############################################
	# output a number into a text following the number of digits specified, and removing trailing zeros
	def n(self, val):
		data = '%.' + str(self.decim) + 'f'
		dum = data % val
		dum = dum.rstrip('0')
		if dum.endswith('.'):
			dum = dum[:-1]
		return dum
	
	# check if code is equal to last code to avoid repeat
	def code(self, val):
		result = val
		if self.cond or val.startswith('G8') or val.startswith('G1') or val.startswith('G2') or val.startswith('G3'):
			if val == self.last_code:
				result = ''
			else: # if the code is changing, we will force again calls to R and Q parameters
				self.last_r = ''
				self.last_q = ''
		self.last_code = val
		if result != '':
			result = result + self.space
		return result
	
	def r(self, val):
		result = val
		if val == self.last_r:
			result = ''
		self.last_r = val
		return result
	
	def q(self, val):
		result = val
		if val == self.last_q:
			result = ''
		self.last_q = val
		return result
		
	# return the axes string
	# the variable 'self.space' is used for condensated mode (if not condensed, it contains a space character)
	def coords(self, axes, values):
		l = len(axes)
		i = 0
		result = ''
		while i < l:
			result = result + axes[i] + self.n(values[i]) + self.space
			i = i + 1
		return result.strip()
	
	##############################################
	# Folowing methods manages the actual output #
	# The result is returned as a string         #
	##############################################
	
	# called before any other methods
	# return the characters at the begining of the file, usually %, this is meanly used for RS232 communication
	def start_output(self):
		return self.pre_data + '\n'
	
	# called at the end of the whole process
	# return the characters to specify that end of file is reached, usually %, this is meanly used for RS232 communication
	def stop_output(self):
		return self.post_data

	# called as first program line
	def start_program(self):
		# G90 set absolute coordinates system
		# G64 set constant speed
		# G94 set feed rate in units per minutes mode
		unit = 'G21'
		if self.unit == '1':
			unit = 'G20'
		return self.code('G90') + self.code('G64') + self.code('G94') + self.code(unit)
	
	# called at the end of the program
	def stop_program(self):
		return 'M2'

	def feed(self, val):
		if self.last_feedrate != val:
			self.last_feedrate = val
			return 'F' + str(val)
		return ''
	
	# set the initial tool position
	def initial_position(self):
		output  = self.core.rapid(['Z'], [self.safe])
		output += self.core.rapid(['X', 'Y'], [0.0, 0.0])
		return output

	# set the final tool position
	def final_position(self):
		if self.core.z != self.safe:
			return self.core.rapid(['Z'], [self.safe])
		return ""
	
	# return a commented string
	def comment(self, data):
		if self.use_comment:
			return '(' + data + ')'
		else:
			return ''
	
	# return a new line
	def start_line(self, num):
		if self.use_inc:
			result = 'N' + str(num).zfill(5) + self.space
			return result
		else:
			return ''

	def stop_line(self):
		return '\n'

	# called before starting work process
	def start_toolpath(self):
		self.started = True
		return ''
	
	# called after stopping work process
	def stop_toolpath(self):
		self.started = False
		return ''

	# called after first z positioning for a single tool path
	def start_single_path(self):
		return ''

	# called before z safe lifting for a single tool path
	def stop_single_path(self):
		return ''
		
	# return the rapid movement string
	# axes array is the list of axes : X Y Z A B C
	# values is a list of double real values
	# both arrays have same size
	# not that the variable 'self.space' is used for condensated mode (if not condensed, it contains a space character)
	def rapid(self, axes, values):
		return self.code('G0') + self.coords( axes, values)
	
	# return the normal movement string
	# axes array is the list of axes : X Y Z A B C
	# values is a list of double real values
	# both arrays have same size
	def linear(self, axes, values):
		return self.code('G1') + self.coords( axes, values)
	
	# return the circular movement string
	# axes array is the list of axes : X Y Z I J K
	# values is a list of double real values
	# both arrays have same size
	def clockwise(self, axes, values):
		return self.code('G2') + self.coords( axes, values)
	
	# return the circular movement string
	# axes array is the list of axes : X Y Z I J K
	# values is a list of double real values
	# both arrays have same size
	def counter_clockwise(self, axes, values):
		return self.code('G3') + self.coords( axes, values)
	
	# return the pause command
	def pause(self, time):
		return self.code('G4') + 'P' + self.n(time)
	
	# tool change order
	def tool(self, number):
		if number == 0:
			return ''
		return 'M6' + self.space + 'T' + self.n(number)
		
	# feed rate
	def feed(self, speed):
		return 'F' + self.n(speed)
	
	# start the spindle in clockwise direction
	def start_spindle_clockwise(self):
		return 'M3'
	
	# start the spindle in counter clockwise direction
	def start_spindle_counter_clockwise(self):
		return 'M4'
	
	# stop the spindle
	def stop_spindle(self):
		return 'M5'
	
	# set the spindle speed
	def spindle_speed(self, speed):
		return 'S' + self.n(speed)
	
	# set left tool compensation
	def tool_left_radius_compensation(self):
		return self.code('G41')
	
	# set right tool compensation
	def tool_right_radius_compensation(self):
		return self.code('G42')
	
	# stop compensation
	def tool_cancel_radius_compensation(self):
		output = ''
		if self.core.z != self.safe:							# we make sure to lift to safe z
			output += self.core.rapid(['Z'], [self.safe])		# position before canceling compensation
		output += self.core.line(self.code('G40'))
		return output
	
	# tool length compensation
	def tool_length_compensation(self, tool_number):
		result = self.code('G43')
		if tool_number > 0:
			result += 'H' + str(tool_number)
		return result
	
	# stop compensation
	def tool_cancel_length_compensation(self):
		output = ''
		if self.core.z != self.safe:							# we make sure to lift to safe z
			output += self.core.rapid(['Z'], [self.safe])		# position before canceling compensation
		output += self.core.line(self.code('G49'))
		return output
	
	# send a drill order
	def drilling(self, axes, values, retract, pause):
		if pause == 0:
			gcode = 'G81'
		else:
			gcode = 'G82'
		result = self.code(gcode + self.space + 'G99') + self.coords( axes, values) + self.r( self.space + 'R' + self.n(retract) )
		if pause > 0:
			result = result + self.space + 'P' + self.n(pause)
		return result.strip()
	
	# send a peck drill order
	def pecking(self, axes, values, retract, delta):
		result = self.code('G83' + self.space + 'G99') + self.coords( axes, values) + self.r( self.space + 'R' + self.n(retract)) + self.q( self.space + 'Q' + self.n(delta))
		return result.strip()
	
	# send a tapping order
	def tapping(self, axes, values, retract):
		result = self.code('G84' + self.space + 'G99') + self.coords( axes, values) + self.r( self.space + 'R' + self.n(retract))
		return result.strip()
	
	# send a boring order
	def boring(self, axes, values, retract, pause):
		result = self.code('G85' + self.space + 'G99') + self.coords( axes, values) + self.r( self.space + 'R' + self.n(retract))
		if pause > 0:
			result = result + self.space + 'P' + self.n(pause)
		return result.strip()
	
	# cancel canned cycle
	def cancel_canned_cycle(self):
		return self.code('G80')
	
	# Turn mist on
	def mist(self):
		return 'M7'
	
	# turn flood on
	def flood(self):
		return 'M8'
	
	# turn all coolant off
	def stop_coolant(self):
		return 'M9'
	
	# XY Plane
	def plane_xy(self):
		return self.code('G17')
	
	# XZ Plane
	def plane_xz(self):
		return self.code('G18')
			
	# YZ Plane
	def plane_yz(self):
		return self.code('G19')