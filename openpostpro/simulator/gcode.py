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

# Translator of the simulator: generic G-code (milling, router, plasma) to the simple commands of the simulator.
#
# The application calls the three methods of the class Translator:
#     start(options)   resets the state, returns a dictionary of information ('unit' = the unit of the program)
#     next(line)       translates ONE line of the program, returns the list of the commands of the line
#     close()          releases what has to be
# The translator only translates. Play, pause, step and rewind belong to the simulator.
#
# A command is a dictionary with 'cmd' and its parameters. Coordinates are absolute, in millimeters, and complete (x, y, z):
#     rapid    x y z
#     linear   x y z f                   f in mm/min
#     arc      x y z cx cy cz cw f plane the center is absolute, plane is 'xy', 'xz' or 'yz'
#     dwell    seconds
#     tool     number
#     spindle  state ('cw', 'ccw', 'off') rpm
#     coolant  state ('flood', 'mist', 'off')
#     stop     optional end              M0 and M1 (optional), M2 and M30 (end)
#     message  level ('info', 'warning', 'error') text
# A translator for another machine or format has the same three methods and sends the same commands.

import math
import re

MM_PER_INCH = 25.4
WORD = re.compile(r'([A-Za-z])\s*([-+]?(?:\d+\.?\d*|\.\d+))')


class Translator:
    def __init__(self):
        self.options = {}
        self.reset()

    # ------------------------------------------------------------------ the three methods of the contract
    def start(self, options):
        self.options = dict(options) if options else {}
        self.reset()
        return {'unit': 'mm', 'translator': 'gcode'}

    def close(self):
        pass

    # the extensions (without the dot) of the files which this translator reads, so the simulator can choose it by itself
    def extensions(self):
        return ['nc', 'gcode', 'tap', 'ngc', 'cnc', 'txt']

    def next(self, line):
        out = []
        try:
            words, messages = self.parse(line)
            for m in messages:
                out.append(self.message('info', m))
            if words:
                self.block(words, out)
        except Exception as e:           # an error in one line does not stop the translation
            out.append(self.message('error', 'line not translated: %s' % e))
        return out

    # ------------------------------------------------------------------ state
    def reset(self):
        self.x = self.y = self.z = 0.0            # the position in millimeters
        self.unit = 1.0                            # millimeters per unit of the program
        self.absolute = True                       # G90 / G91
        self.arc_relative = True                   # G91.1 (the centers are relative to the start, default) / G90.1
        self.plane = 'xy'
        self.motion = 0                            # modal motion: 0 1 2 3 or the canned cycle number (81..89, 73)
        self.feed = 0.0                            # in mm/min (or mm per revolution with G95)
        self.feed_per_rev = False
        self.rpm = 0.0
        self.tool = 0
        self.spindle = 'off'
        self.coolant = 'off'
        self.retract_initial = True                # G98 / G99
        self.warned = set()
        # canned cycle
        self.cycle_r = None
        self.cycle_z = None
        self.cycle_q = 0.0
        self.cycle_p = 0.0

    @staticmethod
    def message(level, text):
        return {'cmd': 'message', 'level': level, 'text': text}

    def warn_once(self, out, key, text, level='warning'):
        if key not in self.warned:
            self.warned.add(key)
            out.append(self.message(level, text))

    # ------------------------------------------------------------------ parsing
    def parse(self, line):
        text = line
        # comments: (...) and ; to the end of the line. A message comment (MSG, ...) is shown
        messages = re.findall(r'\(\s*MSG\s*,\s*(.*?)\)', text, re.IGNORECASE)
        text = re.sub(r'\([^)]*\)', ' ', text)
        text = text.split(';')[0]
        text = text.strip()
        if text.startswith('%') or text.startswith('/'):      # program delimiter, block delete
            return [], messages
        words = [(m.group(1).upper(), float(m.group(2))) for m in WORD.finditer(text)]
        return words, messages

    # ------------------------------------------------------------------ one block
    def block(self, words, out):
        values = {}
        gcodes = []
        mcodes = []
        for letter, value in words:
            if letter == 'G':
                gcodes.append(value)
            elif letter == 'M':
                mcodes.append(int(round(value)))
            elif letter == 'N':
                pass
            else:
                values[letter] = value

        motion_g = None
        dwell = False
        for g in gcodes:
            code = int(round(g * 10))            # 90.1 is 901
            if code in (0, 10, 20, 30):
                motion_g = int(code / 10)
            elif code in (810, 820, 830, 730, 840, 850, 860, 890):
                motion_g = int(code / 10)
            elif code == 800:
                # the end of a canned cycle: the next motion needs its own G word
                self.cycle_r = None
                self.cycle_z = None
                self.motion = 1
            elif code == 40:
                dwell = True
            elif code == 170:
                self.plane = 'xy'
            elif code == 180:
                self.plane = 'xz'
            elif code == 190:
                self.plane = 'yz'
            elif code == 200:
                self.unit = MM_PER_INCH
            elif code == 210:
                self.unit = 1.0
            elif code == 900:
                self.absolute = True
            elif code == 910:
                self.absolute = False
            elif code == 901:
                self.arc_relative = False
            elif code == 911:
                self.arc_relative = True
            elif code == 940:
                self.feed_per_rev = False
            elif code == 950:
                self.feed_per_rev = True
            elif code == 930:
                self.warn_once(out, 'g93', 'inverse time feed (G93) is not simulated: the feed is taken as mm/min')
            elif code == 980:
                self.retract_initial = True
            elif code == 990:
                self.retract_initial = False
            elif code in (410, 420):
                self.warn_once(out, 'g41', 'cutter radius compensation (G41/G42) is not simulated: the programmed path is shown', 'info')
            elif code in (400, 490, 430, 440, 540, 550, 560, 570, 580, 590, 640, 610, 530):
                pass                                          # accepted, with no effect on the shape of the path
            elif code in (280, 300):
                self.warn_once(out, 'g28', 'return to the reference position (G28/G30) is not simulated')
            elif code == 920:
                self.warn_once(out, 'g92', 'coordinate system offset (G92) is not simulated')
            else:
                self.warn_once(out, 'g%g' % g, 'G%g is not known by the translator' % g)

        # the feed rate and the speed of the spindle, with the units of the block
        if 'F' in values:
            self.feed = values['F'] * self.unit
        if 'S' in values:
            self.rpm = values['S']
            if self.spindle != 'off':
                out.append({'cmd': 'spindle', 'state': self.spindle, 'rpm': self.rpm})

        # M codes, in the order of the program
        for m in mcodes:
            if m in (3, 4):
                self.spindle = 'cw' if m == 3 else 'ccw'
                out.append({'cmd': 'spindle', 'state': self.spindle, 'rpm': self.rpm})
            elif m == 5:
                self.spindle = 'off'
                out.append({'cmd': 'spindle', 'state': 'off', 'rpm': 0.0})
            elif m == 6:
                if 'T' in values:
                    self.tool = int(values['T'])
                out.append({'cmd': 'tool', 'number': self.tool})
            elif m == 7:
                self.coolant = 'mist'
                out.append({'cmd': 'coolant', 'state': 'mist'})
            elif m == 8:
                self.coolant = 'flood'
                out.append({'cmd': 'coolant', 'state': 'flood'})
            elif m == 9:
                self.coolant = 'off'
                out.append({'cmd': 'coolant', 'state': 'off'})
            elif m == 0:
                out.append({'cmd': 'stop', 'optional': False, 'end': False})
            elif m == 1:
                out.append({'cmd': 'stop', 'optional': True, 'end': False})
            elif m in (2, 30):
                out.append({'cmd': 'stop', 'optional': False, 'end': True})
            elif m in (48, 49, 50, 51, 53):
                pass
            else:
                self.warn_once(out, 'm%d' % m, 'M%d is not known by the translator' % m)
        if 'T' in values and 6 not in mcodes:
            self.tool = int(values['T'])               # a tool is selected, it is changed by M6

        if dwell:
            seconds = 0.0
            if 'X' in values:
                seconds = values['X']
            elif 'P' in values:
                # P is in seconds (LinuxCNC, GRBL); some controllers use milliseconds
                seconds = values['P'] / (1000.0 if self.options.get('dwell_p_ms') else 1.0)
            out.append({'cmd': 'dwell', 'seconds': seconds})
            return

        if motion_g is not None:
            self.motion = motion_g

        has_axis = any(a in values for a in 'XYZ')
        if not has_axis and not any(a in values for a in 'IJKR') and motion_g is None:
            return
        if self.motion in (0, 1, 2, 3) and not has_axis and not (self.motion in (2, 3)):
            return

        # the target, absolute in millimeters
        target = [self.x, self.y, self.z]
        for i, a in enumerate('XYZ'):
            if a in values:
                v = values[a] * self.unit
                target[i] = v if self.absolute else target[i] + v

        if self.motion == 0:
            self.go(out, 'rapid', target)
        elif self.motion == 1:
            if self.feed <= 0:
                self.warn_once(out, 'nofeed', 'a linear move with no feed rate (F) is not defined: the feed is taken as 1 mm/min', 'warning')
                self.feed = 1.0
            self.go(out, 'linear', target)
        elif self.motion in (2, 3):
            self.arc(out, values, target)
        else:
            self.cycle(out, values, target)

    # ------------------------------------------------------------------ moves
    def feed_mm(self):
        if self.feed_per_rev and self.rpm > 0:
            return self.feed * self.rpm
        return self.feed

    def go(self, out, kind, target):
        if kind == 'rapid':
            out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': target[2]})
        else:
            out.append({'cmd': 'linear', 'x': target[0], 'y': target[1], 'z': target[2], 'f': self.feed_mm()})
        self.x, self.y, self.z = target

    def arc(self, out, values, target):
        cw = self.motion == 2
        a, b, c = {'xy': (0, 1, 2), 'xz': (2, 0, 1), 'yz': (1, 2, 0)}[self.plane]   # the two axes of the plane, the third one
        start = [self.x, self.y, self.z]
        offsets = {'xy': ('I', 'J', 'K'), 'xz': ('K', 'I', 'J'), 'yz': ('J', 'K', 'I')}[self.plane]
        center = [0.0, 0.0, 0.0]

        if 'R' in values:
            r = values['R'] * self.unit
            dx, dy = target[a] - start[a], target[b] - start[b]
            d = math.hypot(dx, dy)
            if d < 1e-9:
                out.append(self.message('error', 'an arc with a radius (R) needs an end point different from its start'))
                return
            h2 = r * r - d * d / 4.0
            if h2 < -1e-6:
                out.append(self.message('error', 'the radius of the arc (R) is too small for its end point'))
                return
            h = math.sqrt(max(h2, 0.0))
            # the center is on the perpendicular of the chord; the sign gives the small or the large arc
            mx, my = (start[a] + target[a]) / 2.0, (start[b] + target[b]) / 2.0
            ux, uy = -dy / d, dx / d
            sign = -1.0 if cw else 1.0
            if r < 0:
                sign = -sign
            center[a] = mx + sign * ux * h
            center[b] = my + sign * uy * h
        else:
            ia, ib, ic = offsets
            offa = values.get(ia, 0.0) * self.unit
            offb = values.get(ib, 0.0) * self.unit
            if self.arc_relative:
                center[a] = start[a] + offa
                center[b] = start[b] + offb
            else:
                center[a] = offa
                center[b] = offb
            if not (ia in values or ib in values):
                out.append(self.message('error', 'an arc needs its center (I J K) or its radius (R)'))
                return
        center[c] = start[c]

        if self.feed <= 0:
            self.warn_once(out, 'nofeed', 'an arc with no feed rate (F) is not defined: the feed is taken as 1 mm/min', 'warning')
            self.feed = 1.0
        out.append({'cmd': 'arc', 'x': target[0], 'y': target[1], 'z': target[2], 'cx': center[0], 'cy': center[1], 'cz': center[2],
                    'cw': cw, 'f': self.feed_mm(), 'plane': self.plane})
        self.x, self.y, self.z = target

    # ------------------------------------------------------------------ canned cycles, developed in moves
    def cycle(self, out, values, target):
        if self.plane != 'xy':
            self.warn_once(out, 'cycleplane', 'the canned cycles are simulated in the XY plane only')
        if 'R' in values:
            self.cycle_r = values['R'] * self.unit if self.absolute else self.z + values['R'] * self.unit
        if 'Z' in values:
            self.cycle_z = target[2]
        if 'Q' in values:
            self.cycle_q = abs(values['Q'] * self.unit)
        if 'P' in values:
            self.cycle_p = values['P'] / (1000.0 if self.options.get('dwell_p_ms') else 1.0)
        if self.cycle_r is None or self.cycle_z is None:
            out.append(self.message('error', 'a canned cycle needs R and Z'))
            return
        if not any(a in values for a in 'XY'):
            return                                           # no hole: only the setting of the cycle

        initial = self.z
        r_level = self.cycle_r
        z_bottom = self.cycle_z
        feed = self.feed_mm() if self.feed > 0 else 1.0

        # G98 goes back to the initial height (if it is above the R plane), G99 stops at the R plane
        out_z = max(initial, r_level) if self.retract_initial else r_level
        # to the hole at the highest of the two heights, then to the R plane
        out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': max(initial, r_level)})
        if r_level < initial:
            out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': r_level})

        code = self.motion
        if code in (83, 73) and self.cycle_q > 0:
            depth = r_level
            while depth > z_bottom + 1e-9:
                nxt = max(depth - self.cycle_q, z_bottom)
                out.append({'cmd': 'linear', 'x': target[0], 'y': target[1], 'z': nxt, 'f': feed})
                if nxt > z_bottom + 1e-9:
                    # chip breaking (G73) lifts a little, deep drilling (G83) goes out of the hole and comes back
                    if code == 83:
                        out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': r_level})
                        out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': nxt + 0.5})
                    else:
                        out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': nxt + 0.5})
                depth = nxt
        else:
            out.append({'cmd': 'linear', 'x': target[0], 'y': target[1], 'z': z_bottom, 'f': feed})

        if code in (82, 89) and self.cycle_p > 0:
            out.append({'cmd': 'dwell', 'seconds': self.cycle_p})
        if code in (85, 86, 89):
            # the tool comes out of the hole at the feed rate
            out.append({'cmd': 'linear', 'x': target[0], 'y': target[1], 'z': r_level, 'f': feed})
            if out_z > r_level:
                out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': out_z})
        else:
            out.append({'cmd': 'rapid', 'x': target[0], 'y': target[1], 'z': out_z})
        self.x, self.y, self.z = target[0], target[1], out_z
