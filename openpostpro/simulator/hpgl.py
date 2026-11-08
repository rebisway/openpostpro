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

# Translator of the simulator: HPGL (plotters, vinyl cutters) to the simple commands of the simulator (see gcode.py for the contract).
#
# HPGL has no Z: the pen (or the blade) is up or down. The pen up moves at the height `z_up`, the pen down at the height `z_down`
# (options of the translator, in millimeters: the blade goes 1 mm into the material by default). The tool of the simulator is the
# pen: choose the shape and the diameter of the tool in the settings of the simulator. With a torch (plasma, water jet) the pen down
# is the torch lit: the commands torch on and off are sent when the pen goes down and up.
#
# What is translated:
#     IN, DF            initialize: pen up, absolute coordinates
#     SP n              select the pen n (SP0 puts the pen away: the pen goes up)
#     PU, PD [x,y ...]  pen up, pen down, and moves to the points
#     PA, PR [x,y ...]  absolute and relative coordinates (moves with the pen in its state)
#     AA, AR            arc with the center absolute or relative, and the sweep angle in degrees
#     CI r              circle around the current point
#     VS v              speed of the pen, in cm/s (the feed of the pen down moves)
# The other commands (labels, line types, pen thickness, scaling IP and SC, polyline encoded PE, ...) are ignored with a warning, once.
# The coordinates are in plotter units: 40 units per millimeter (0.025 mm) as the HPGL post-processor of the application writes them.

import math
import re

UNITS_PER_MM = 40.0
STATEMENT = re.compile(r'([A-Za-z]{2})([^A-Za-z]*)')
NUMBERS = re.compile(r'[-+]?(?:\d+\.?\d*|\.\d+)')


class Translator:
    def __init__(self):
        self.options = {}
        self.reset()

    # ------------------------------------------------------------------ the three methods of the contract
    def start(self, options):
        self.options = dict(options) if options else {}
        self.reset()
        return {'unit': 'mm', 'translator': 'hpgl'}

    def close(self):
        pass

    # the extensions (without the dot) of the files which this translator reads, so the simulator can choose it by itself
    def extensions(self):
        return ['plt', 'hpgl', 'hpg', 'hgl']

    def next(self, line):
        out = []
        try:
            for mnemonic, text in self.statements(line):
                self.statement(mnemonic.upper(), self.numbers(text), out)
        except Exception as e:           # an error in one line does not stop the translation
            out.append(self.message('error', 'line not translated: %s' % e))
        return out

    # ------------------------------------------------------------------ state
    def reset(self):
        self.units_per_mm = float(self.options.get('units_per_mm', UNITS_PER_MM))
        self.z_up = float(self.options.get('z_up', 5.0))
        self.z_down = float(self.options.get('z_down', -1.0))
        self.feed = float(self.options.get('feed', 3000.0))		# mm/min of the pen down moves, until a VS says another one
        self.x = self.y = 0.0			# the position, in plotter units
        self.pen_down = False
        self.absolute = True
        self.pen = None
        self.warned = set()

    @staticmethod
    def message(level, text):
        return {'cmd': 'message', 'level': level, 'text': text}

    def warn_once(self, out, key, text):
        if key not in self.warned:
            self.warned.add(key)
            out.append(self.message('warning', text))

    # ------------------------------------------------------------------ parsing
    @staticmethod
    def statements(line):
        # the labels (LB text, ended by the character 3) can have any character: they are dropped before the statements are found
        text = re.sub(r'LB[^\x03]*(\x03|$)', 'LB;', line, flags=re.IGNORECASE)
        # the comments (CO "text";) have letters too
        text = re.sub(r'CO\s*"[^"]*"', 'CO', text, flags=re.IGNORECASE)
        text = ''.join(c for c in text if c >= ' ')		# the controls (an escape sequence of a device, the end of line)
        return STATEMENT.findall(text)

    @staticmethod
    def numbers(text):
        return [float(v) for v in NUMBERS.findall(text)]

    # ------------------------------------------------------------------ the commands
    def mm(self, units):
        return units / self.units_per_mm

    def z(self):
        return self.z_down if self.pen_down else self.z_up

    def go(self, out, x, y):
        # a move to (x, y) in plotter units with the pen in its state
        self.x, self.y = x, y
        if self.pen_down:
            out.append({'cmd': 'linear', 'x': self.mm(x), 'y': self.mm(y), 'z': self.z_down, 'f': self.feed})
        else:
            out.append({'cmd': 'rapid', 'x': self.mm(x), 'y': self.mm(y), 'z': self.z_up})

    def move_pen(self, out, down):
        if down == self.pen_down:
            return
        self.pen_down = down
        if down:
            # the pen goes down where it is. For a torch (plasma, water jet) the pen down is the torch lit
            out.append({'cmd': 'torch', 'state': 'on'})
            out.append({'cmd': 'linear', 'x': self.mm(self.x), 'y': self.mm(self.y), 'z': self.z_down, 'f': self.feed})
        else:
            out.append({'cmd': 'rapid', 'x': self.mm(self.x), 'y': self.mm(self.y), 'z': self.z_up})
            out.append({'cmd': 'torch', 'state': 'off'})

    def points(self, out, values, name):
        if len(values) % 2:
            out.append(self.message('warning', '%s: a coordinate without its pair is ignored' % name))
            values = values[:-1]
        for i in range(0, len(values), 2):
            x, y = values[i], values[i + 1]
            if not self.absolute:
                x, y = self.x + x, self.y + y
            self.go(out, x, y)

    def arc(self, out, cx, cy, sweep):
        sx, sy = self.x, self.y
        radius = math.hypot(sx - cx, sy - cy)
        if radius <= 0.0 or sweep == 0.0:
            return
        sweep = max(-360.0, min(360.0, sweep))
        a0 = math.atan2(sy - cy, sx - cx)
        a1 = a0 + math.radians(sweep)
        ex, ey = cx + radius * math.cos(a1), cy + radius * math.sin(a1)
        if abs(sweep) >= 360.0:
            ex, ey = sx, sy			# a full circle comes back to the start
        if self.pen_down:
            out.append({'cmd': 'arc', 'x': self.mm(ex), 'y': self.mm(ey), 'z': self.z_down, 'cx': self.mm(cx), 'cy': self.mm(cy),
                        'cz': self.z_down, 'cw': sweep < 0, 'f': self.feed, 'plane': 'xy'})
        else:
            out.append({'cmd': 'rapid', 'x': self.mm(ex), 'y': self.mm(ey), 'z': self.z_up})
        self.x, self.y = ex, ey

    def statement(self, name, values, out):
        if name in ('IN', 'DF'):
            self.pen_down = False
            self.absolute = True
        elif name == 'SP':
            number = int(values[0]) if values else 0
            if number == 0:
                self.move_pen(out, False)
            elif number != self.pen:
                self.pen = number
                out.append({'cmd': 'tool', 'number': number})
        elif name == 'PU':
            self.move_pen(out, False)
            self.points(out, values, name)
        elif name == 'PD':
            self.move_pen(out, True)
            self.points(out, values, name)
        elif name == 'PA':
            self.absolute = True
            self.points(out, values, name)
        elif name == 'PR':
            self.absolute = False
            self.points(out, values, name)
        elif name in ('AA', 'AR'):
            if len(values) < 3:
                out.append(self.message('warning', '%s needs a center and an angle' % name))
                return
            cx, cy = values[0], values[1]
            if name == 'AR':
                cx, cy = self.x + cx, self.y + cy
            self.arc(out, cx, cy, values[2])
        elif name == 'CI':
            if not values or values[0] <= 0:
                return
            # a circle around the current point, it starts on the right of it, then the pen is back at the center
            cx, cy, radius = self.x, self.y, values[0]
            was_down = self.pen_down
            self.move_pen(out, False)
            self.go(out, cx + radius, cy)
            self.move_pen(out, True)
            self.arc(out, cx, cy, 360.0)
            self.move_pen(out, False)
            self.go(out, cx, cy)
            self.move_pen(out, was_down)
        elif name == 'VS':
            if values and values[0] > 0:
                self.feed = values[0] * 600.0		# cm/s to mm/min
        elif name in ('CO', 'PW', 'LT', 'FS', 'SI', 'SR', 'DI', 'DR', 'CS', 'SS', 'SA', 'NR', 'PG', 'AF', 'AH', 'OP', 'OI', 'OS', 'OE', 'ER'):
            pass		# the comment, the thickness, the line type, the force, the characters, the device: no effect on the path
        else:
            self.warn_once(out, name, '%s is not translated' % name)
