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


# HPGL post-processor, for plotters and vinyl cutters. The coordinates are integers in plotter units
# (1 unit = 0.025 mm). The arcs are cut into short segments: HPGL has no arc move in this output.
# Z, feed rate, spindle and canned cycles do not exist in HPGL and are ignored.

import math

UNITS_PER_MM = 1.0 / 0.025
UNITS_PER_INCH = 1.0 / 0.0009842
CHORD = 0.01			# maximum distance (mm) between an arc and its segments


class Postpro:
	def __init__(self):
		self.cur_x = 0.0		# current position, in plotter units
		self.cur_y = 0.0
		self.factor = UNITS_PER_MM
		self.pen_down = False

		self.en_us = {'description': 'HPGL'}
		self.fr_fr = {'description': 'HPGL'}
		self.es_es = {'description': 'HPGL'}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)	# unknown language: English

	# the unit of the document, 'mm' or 'inch', set by the application before the generation
	def set_unit(self, unit):
		self.factor = UNITS_PER_INCH if unit == 'inch' else UNITS_PER_MM

	def set_core(self, value):
		self.core = value

	def set_safe_position(self, value):
		pass

	#########################################
	# default methods of the post-processor #
	#########################################

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'OTHER'

	def get_extention(self):
		return 'plt'

	def get_decimal(self):
		return 0

	def incrementation(self):
		return -1

	def pass_xy_to_cw_ccw(self):
		return True

	def pass_z_to_cw_ccw(self):
		return False

	def pass_xy_to_linear(self):
		return False

	def pass_z_to_linear(self):
		return False

	def use_safe_pos_before_tool_change(self):
		return False

	def get_disable_z(self):
		return True

	# the center of an arc is given relative to its start point
	def ijk_relative(self):
		return True

	def get_properties_count(self):
		return 0

	def get_property_description(self, index):
		return ''

	def get_property_key(self, index):
		return str(index)

	def get_property(self, index):
		return ''

	def set_property(self, index, val):
		pass

	##############################################
	# Folowing methods manages the actual output #
	# The result is returned as a string         #
	##############################################

	def start_output(self):
		return 'IN;\n'

	def stop_output(self):
		return 'PU;\n'

	def start_program(self):
		return ''

	def stop_program(self):
		return ''

	def initial_position(self):
		return ''

	def final_position(self):
		return ''

	def comment(self, data):
		return 'CO "' + data.replace('"', "'") + '";'

	def start_line(self, num):
		return ''

	def stop_line(self):
		return '\n'

	def start_toolpath(self):
		return ''

	def stop_toolpath(self):
		return ''

	def start_single_path(self):
		return ''

	def stop_single_path(self):
		return ''

	# X Y of the move, the missing one is the current position
	def destination(self, axes, values):
		x, y = self.cur_x, self.cur_y
		for axis, value in zip(axes, values):
			if axis == 'X':
				x = value * self.factor
			elif axis == 'Y':
				y = value * self.factor
		return x, y

	# 'PU' (pen up) or 'PD' (pen down) with the position (plotter units), nothing if the position does not change
	def move(self, command, x, y):
		ux, uy = int(round(x)), int(round(y))
		cx, cy = int(round(self.cur_x)), int(round(self.cur_y))
		self.cur_x, self.cur_y = x, y
		if ux == cx and uy == cy:
			return ''
		return command + ' ' + str(ux) + ',' + str(uy) + ';'

	def rapid(self, axes, values):
		x, y = self.destination(axes, values)
		return self.move('PU', x, y)

	def linear(self, axes, values):
		x, y = self.destination(axes, values)
		return self.move('PD', x, y)

	# the arc is cut into segments: the core already holds the end point, the start point is the current position
	def arc(self, axes, values, cw):
		x, y = self.destination(axes, values)
		i = j = 0.0
		for axis, value in zip(axes, values):
			if axis == 'I':
				i = value * self.factor
			elif axis == 'J':
				j = value * self.factor
		sx, sy = self.cur_x, self.cur_y
		cx, cy = sx + i, sy + j
		radius = math.hypot(sx - cx, sy - cy)
		if radius == 0:
			return self.move('PD', x, y)

		a0 = math.atan2(sy - cy, sx - cx)
		a1 = math.atan2(y - cy, x - cx)
		sweep = a1 - a0
		if cw:
			if sweep >= 0:
				sweep -= 2 * math.pi
		elif sweep <= 0:
			sweep += 2 * math.pi
		# full circle: same start and end point
		if abs(x - sx) < 0.5 and abs(y - sy) < 0.5:
			sweep = -2 * math.pi if cw else 2 * math.pi

		# the chord error (CHORD mm) gives the step angle
		tolerance = CHORD * self.factor
		step = 2 * math.acos(max(0.0, 1 - tolerance / radius)) if radius > tolerance else math.pi / 2
		count = max(2, int(math.ceil(abs(sweep) / max(step, 0.01))))

		output = ''
		for k in range(1, count):
			a = a0 + sweep * k / count
			line = self.move('PD', cx + radius * math.cos(a), cy + radius * math.sin(a))
			if line != '':
				output += self.core.line(line)
		last = self.move('PD', x, y)
		return last if output == '' else output + last

	def clockwise(self, axes, values):
		return self.arc(axes, values, True)

	def counter_clockwise(self, axes, values):
		return self.arc(axes, values, False)

	def pause(self, time):
		return ''

	# the tool number is the pen number
	def tool(self, number):
		if number == 0:
			return ''
		return 'SP' + str(int(number)) + ';'

	def feed(self, speed):
		return ''

	def start_spindle_clockwise(self):
		return ''

	def start_spindle_counter_clockwise(self):
		return ''

	def stop_spindle(self):
		return ''

	def spindle_speed(self, speed):
		return ''

	def tool_left_radius_compensation(self):
		return ''

	def tool_right_radius_compensation(self):
		return ''

	def tool_cancel_radius_compensation(self):
		return ''

	def tool_length_compensation(self, tool_number):
		return ''

	def tool_cancel_length_compensation(self):
		return ''

	def drilling(self, axes, values, retract, pause):
		return ''

	def pecking(self, axes, values, retract, delta):
		return ''

	def tapping(self, axes, values, retract):
		return ''

	def boring(self, axes, values, retract, pause):
		return ''

	def cancel_canned_cycle(self):
		return ''

	def mist(self):
		return ''

	def flood(self):
		return ''

	def stop_coolant(self):
		return ''
