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

import os
import sys
import importlib.util

# Embedded Python does not find the Tcl/Tk libraries (init.tcl): locate them from the _tkinter module.
# Cadcore is imported before any user script, so scripts can import tkinter directly.
_spec = importlib.util.find_spec('_tkinter')
if _spec is not None and _spec.origin:
    _root = os.path.dirname(os.path.dirname(_spec.origin))
    for _var, _dir in (('TCL_LIBRARY', 'tcl8.6'), ('TK_LIBRARY', 'tk8.6')):
        _path = os.path.join(_root, 'tcl', _dir)
        if os.path.isdir(_path):
            os.environ[_var] = _path


class Cadcore:
    def __init__(self):
        self.mouse_x = 0.0
        self.mouse_y = 0.0
        self.unit = 'mm'
        self.selection = []
        self.output = ''

    def set_script(self, script):
        self.script = script

    def set_language(self, value):
        self.script.set_language(value)

    # unit of the document, 'mm' or 'inch'
    def set_unit(self, unit):
        self.unit = unit

    # shapes selected when the script is called: a list of dictionaries, see the CAD scripts guide
    def set_selection(self, selection):
        self.selection = selection

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

    # The other shapes. Coordinates are tuples (x, y), as in the selection. Each shape has its modify_ version
    # with the id of the shape as first argument: a shape keeps its type (a line stays a line).
    def point(self, x, y):
        self.output += self._point(x, y) + '\n'

    def ellipse(self, cx, cy, major, minor, start=0.0, stop=6.283185307179586, angle=0.0):
        self.output += self._ellipse(cx, cy, major, minor, start, stop, angle) + '\n'

    # points: list of (x, y); mode: 'polyline', 'bspline', 'natural_cubic', 'lagrange', 'catmull' or 'bezier'
    def polyline(self, points, mode='polyline'):
        self.output += self._polyline(points, mode) + '\n'

    # segments: list of dictionaries as in the selection: type ('line', 'arc' or 'circle'), point, and center, radius, cw for an arc
    def spline(self, segments):
        self.output += self._spline(segments) + '\n'

    def text(self, x, y, text, height=10.0, family='Arial', bold=False, italic=False, inter=0.0):
        self.output += self._text(x, y, text, height, family, bold, italic, inter) + '\n'

    def modify_point(self, id, x, y):
        self.output += 'M:' + str(id) + ':' + self._point(x, y) + '\n'

    def modify_ellipse(self, id, cx, cy, major, minor, start=0.0, stop=6.283185307179586, angle=0.0):
        self.output += 'M:' + str(id) + ':' + self._ellipse(cx, cy, major, minor, start, stop, angle) + '\n'

    def modify_polyline(self, id, points, mode='polyline'):
        self.output += 'M:' + str(id) + ':' + self._polyline(points, mode) + '\n'

    def modify_spline(self, id, segments):
        self.output += 'M:' + str(id) + ':' + self._spline(segments) + '\n'

    def modify_text(self, id, x, y, text, height=10.0, family='Arial', bold=False, italic=False, inter=0.0):
        self.output += 'M:' + str(id) + ':' + self._text(x, y, text, height, family, bold, italic, inter) + '\n'

    # text of the commands, one per shape
    def _point(self, x, y):
        return 'P:' + str(x) + ':' + str(y)

    def _ellipse(self, cx, cy, major, minor, start, stop, angle):
        return 'E:' + ':'.join(str(v) for v in (cx, cy, major, minor, start, stop, angle))

    def _polyline(self, points, mode):
        return 'PL:' + mode + ':' + ':'.join(str(c) for p in points for c in p)

    def _spline(self, segments):
        parts = []
        for s in segments:
            x, y = s['point']
            if s['type'] == 'line':
                parts.append('L:' + str(x) + ':' + str(y))
            else:
                cx, cy = s['center']
                parts.append(('A' if s['type'] == 'arc' else 'C') + ':' + ':'.join(str(v) for v in (x, y, cx, cy, s['radius'], 1 if s['cw'] else 0)))
        return 'S:' + ':'.join(parts)

    def _text(self, x, y, text, height, family, bold, italic, inter):
        # a text cannot contain a line break: it would cut the command
        return 'T:' + ':'.join(str(v) for v in (x, y, height, inter, 1 if bold else 0, 1 if italic else 0, family)) + ':' + str(text).replace('\n', ' ')

    # modify the parameters of a selected shape: a shape keeps its type (a line stays a line)
    def modify_line(self, id, x1, y1, x2, y2):
        self.output += 'M:' + str(id) + ':L:' + str(x1) + ':' + str(y1) + ':' + str(x2) + ':' + str(y2) + '\n'

    def modify_circle(self, id, x, y, r):
        self.output += 'M:' + str(id) + ':C:' + str(x) + ':' + str(y) + ':' + str(r) + '\n'

    def modify_arc(self, id, x1, y1, cx, cy, x2, y2, cw):
        clock = 0
        if cw:
            clock = 1
        self.output += 'M:' + str(id) + ':A:' + str(x1) + ':' + str(y1) + ':' + str(cx) + ':' + str(cy) + ':' + str(x2) + ':' + str(y2) + ':' + str(clock) + '\n'

    # delete a shape
    def remove(self, id):
        self.output += 'D:' + str(id) + '\n'

    def get_output(self):
        return self.output

    def append_output(self, val):
        self.output += val

    def get_description(self):
        return self.script.get_description()

    # false if the script manages its own dialog box (default: the properties dialog box is displayed)
    def use_properties(self):
        if hasattr(self.script, 'use_properties'):
            return self.script.use_properties()
        return True

    # name used to save the property value, it does not change with the language or the order (the index by default)
    def get_property_key(self, index):
        if hasattr(self.script, 'get_property_key'):
            return self.script.get_property_key(index)
        return str(index)
	
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
