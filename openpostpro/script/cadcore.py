# OpenPostPro - www.openpostpro.org
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

# Python 313 tkinter not loading bug
# Copy folders Python313\tcl\tcl8.6 and Python313\tcl\tk8.6 folder to Python313\lib


class Cadcore:
    def __init__(self):
        self.mouse_x = 0.0
        self.mouse_y = 0.0
        self.output = ''

    def set_script(self, script):
        self.script = script

    def set_language(self, value):
        self.script.set_language(value)

    def set_mouse(self, x, y):
        self.mouse_x = float(x)
        self.mouse_y = float(y)

    def line(self, x1, y1, x2, y2):
        self.output += 'L:' + str(x1) + ':' + str(y1) + ':' + str(x2) + ':' + str(y2) + '\n'

    def circle(self, x, y, r):
        self.output += 'C:' + str(x) + ':' + str(y) + ':' + str(r) + '\n'
    
    def arc(self, x1, y1, cx, cy, x2, y2, cw):
        clock = 0
        if cw:
            clock = 1
        self.output += 'A:' + str(x1) + ':' + str(y1) + ':' + str(cx) + ':' + str(cy) + ':' + str(x2) + ':' + str(y2) + ':' + str(clock) + '\n'

    def get_output(self):
        return self.output

    def append_output(self, val):
        self.output += val

    def get_description(self):
        return self.script.get_description()

    def use_properties(self):
        return self.script.use_properties()
	
    def get_properties_count(self):
        return self.script.get_properties_count()
	    
    def get_property_description(self, index):
        return self.script.get_property_description(index)

    def get_property(self, index):
        return self.script.get_property(index)
		
    def set_property(self, index, val):
        return self.script.set_property(index, val)

    def get_category(self):
        return self.script.get_category()

    def run(self):
        self.script.run()
        return self.output
