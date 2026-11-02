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

import sys

class Postcore:
	# class constructor (always present, leave it)
	def __init__(self):
		self.pos = 0
		self.inc = 0
		self.x = 0
		self.y = 0
		self.z = 0
		self.line_count = 0
		self.f = 0

	def get_line_count(self):
		return self.line_count

	def line(self, val):
		result = ''
		if len(val) > 0:
			if self.inc > 0 and val[0] != 'N':
				self.pos += self.inc
				result  = self.postpro.start_line(self.pos)
			result += val
			if len(result) > 0 and not result.endswith(self.postpro.stop_line()):
				result += self.postpro.stop_line()
				self.line_count = self.line_count + 1
		return result

	def get_current_position(self):
		return [self.x, self.y, self.z]

	def get_current_line(self):
		return self.pos

	def set_postpro(self, postpro):
		self.postpro = postpro

	def set_language(self, value):
		self.postpro.set_language(value)

	def get_description(self):
		return self.postpro.get_description()

	def get_category(self):
		return self.postpro.get_category()
		
	def get_extention(self):
		return self.postpro.get_extention()

	def set_safe_position(self, value):
		self.postpro.set_safe_position(value)

	def pass_xy_to_cw_ccw(self):
		return self.postpro.pass_xy_to_cw_ccw()
			
	def pass_z_to_cw_ccw(self):
		return self.postpro.pass_z_to_cw_ccw()
		
	def pass_xy_to_linear(self):
		return self.postpro.pass_xy_to_linear()
		
	def pass_z_to_linear(self):
		return self.postpro.pass_z_to_linear()
		
	def use_safe_pos_before_tool_change(self):
		return self.postpro.use_safe_pos_before_tool_change()
	
	def get_disable_z(self):
		return self.postpro.get_disable_z()
		
	# return true if arc center is relative to the destination point
	# or false if the center requested is absolute
	def ijk_relative(self):
		return self.postpro.ijk_relative()

	def get_properties_count(self):
		return self.postpro.get_properties_count()
	    
	def get_property_description(self, index):
		return self.postpro.get_property_description(index)

	def get_property(self, index):
		return self.postpro.get_property(index)
		
	def set_property(self, index, val):
		return self.postpro.set_property(index, val)

	def get_category(self):
		return self.postpro.get_category()

	def comment(self, val):
		output = ''
		if len(val) > 0:
			output = self.postpro.comment(val)
			if len(output) > 0:
				output += self.postpro.stop_line();
				self.line_count = self.line_count + 1
		return output

	def append(self, val):
		return self.line(val)

	def set_axes(self, axes, values):
		l = len(axes)
		i = 0
		result = ''
		while i < l:
			if axes[i] == 'X' or axes[i] == 'x':
				self.x = values[i]
			elif axes[i] == 'Y' or axes[i] == 'y':
				self.y = values[i]
			elif axes[i] == 'Z' or axes[i] == 'z':
				self.z = values[i]
			i = i + 1

	# called at program start
	def start_loop(self):
		self.x = self.y = self.z = 0

		#init line incrementation
		self.inc = self.postpro.incrementation()
		self.pos = self.inc

		output = self.postpro.start_output()
		if len(output) > 0:
			self.line_count = self.line_count + 1
		return output

	# called at program end
	def stop_loop(self):
		output = self.postpro.stop_output()
		if len(output) > 0:
			self.line_count = self.line_count + 1
		return output

	# called at program start
	def start_program(self):
		output  = self.line(self.postpro.start_program())
		output += self.line(self.postpro.initial_position())
		return output

	# called at program end
	def stop_program(self):
		output  = self.line(self.postpro.final_position())
		output += self.line(self.postpro.stop_program())
		return output

	# called before a new group  of tool paths is processed
	def start_group(self, name, tool, speed):
		output = ''
		if tool >= 0:
			output += self.line(self.postpro.tool(tool))
			if speed > 0:
				output += self.line(self.postpro.spindle_speed(speed))
		return output

	def stop_group(self):
		return ''

	# called before a new tool path is processed
	def start_toolpath(self):
		return self.postpro.start_toolpath()

	# called after stopping work process
	def stop_toolpath(self):
		return self.postpro.stop_toolpath()

	# called after first z positioning for a single tool path
	def start_single_path(self):
		return self.postpro.start_single_path()

	# called before z safe lifting for a single tool path
	def stop_single_path(self):
		return self.postpro.stop_single_path()

	# linear rapid movement
	def rapid(self, axes, values):
		self.set_axes(axes, values) #keep track of current position
		return self.line(self.postpro.rapid(axes, values))
	
	# linear working movement
	def linear(self, axes, values):
		self.set_axes(axes, values) #keep track of current position
		return self.line(self.postpro.linear(axes, values))

	# clockwise circular working movement
	def clockwise(self, axes, values):
		self.set_axes(axes, values) #keep track of current position
		return self.line(self.postpro.clockwise(axes, values))
		
	# counter clockwise circular working movement
	def counter_clockwise(self, axes, values):
		self.set_axes(axes, values) #keep track of current position
		return self.line(self.postpro.counter_clockwise(axes, values))

	# feed rate
	def feed(self, value):
		if self.f != value:
			self.f = value;
			return self.line(self.postpro.feed(value))
		return ''
		
	# start the spindle in clockwise direction
	def start_spindle_clockwise(self):
		return self.line(self.postpro.start_spindle_clockwise())
	
	# start the spindle in counter clockwise direction
	def start_spindle_counter_clockwise(self):
		return self.line(self.postpro.start_spindle_counter_clockwise())
	
	# stop the spindle
	def stop_spindle(self):
		return self.line(self.postpro.stop_spindle())
	
	# set the spindle speed
	def spindle_speed(self, speed):
		return self.line(self.postpro.spindle_speed(speed))

		# set left tool compensation
	def tool_left_radius_compensation(self):
		return self.line(self.postpro.tool_left_radius_compensation())
	
	# set right tool compensation
	def tool_right_radius_compensation(self):
		return self.line(self.postpro.tool_right_radius_compensation())
	
	# stop compensation
	def tool_cancel_radius_compensation(self):
		return self.line(self.postpro.tool_cancel_radius_compensation())
		
	# tool length compensation
	def tool_length_compensation(self, tool_number):
		return self.line(self.postpro.tool_length_compensation(tool_number))
	
	# stop compensation
	def tool_cancel_length_compensation(self):
		return self.line(self.postpro.tool_cancel_length_compensation())
	
	# send a drill order
	def drilling(self, axes, values, retract, pause):
		return self.line(self.postpro.drilling(axes, values, retract, pause))
	
	# send a peck drill order
	def pecking(self, axes, values, retract, delta):
		return self.line(self.postpro.pecking(axes, values, retract, delta))
	
	# send a tapping order
	def tapping(self, axes, values, retract):
		return self.line(self.postpro.tapping(axes, values, retract))
	
	# send a boring order
	def boring(self, axes, values, retract, pause):
		return self.line(self.postpro.boring(axes, values, retract, pause))
	
	# cancel canned cycle
	def cancel_canned_cycle(self):
		return self.line(self.postpro.cancel_canned_cycle())

	def pause(self, time):
		return self.line(self.postpro.pause(time))