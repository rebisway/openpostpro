# SIMULATOR TRANSLATOR TEST - checks the translator of generic G-code (openpostpro/simulator/gcode.py).
#
# What for:
#   The translator is the only part of the simulator which knows the G-code. A mistake here shows a wrong path in the simulator
#   (an arc on the wrong side, a feed in inches taken as millimeters, a canned cycle which does not go down) and it looks like a
#   mistake of the program. The simulator trusts the commands it gets.
#
# What is checked: the moves and their modes (G0 G1 G2 G3), absolute and relative distances, units (inches), arcs with a center and
#   with a radius, the three planes, the feed rate, the spindle, the coolant, the tools, the stops, the dwell, the canned cycles
#   (G81 G82 G83 with G98 / G99), the comments and the messages, the unknown codes (a warning, once) and an error in one line.
#
# Run: python tests/test_simulator_gcode.py   (exit code 0 = all checks pass)
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'simulator'))
from gcode import Translator  # noqa: E402

failed = 0
run = 0


def check(condition, message):
    global failed, run
    run += 1
    if not condition:
        failed += 1
        print('  FAIL: ' + message)


def close(a, b, tolerance=1e-6):
    return abs(a - b) <= tolerance


def translate(lines, options=None):
    t = Translator()
    t.start(options or {})
    result = []
    for line in lines.split('\n'):
        result.extend(t.next(line))
    t.close()
    return result


def kinds(commands):
    return [c['cmd'] for c in commands]


print('moves and modes')
c = translate('G0 X10 Y20 Z5\nG1 X20 F300\nY30\nG0 Z10')
check(kinds(c) == ['rapid', 'linear', 'linear', 'rapid'], 'the modal motion keeps its mode: %s' % kinds(c))
check(c[0]['x'] == 10 and c[0]['y'] == 20 and c[0]['z'] == 5, 'a rapid has its three coordinates')
check(c[1]['x'] == 20 and c[1]['y'] == 20 and c[1]['z'] == 5 and c[1]['f'] == 300, 'the missing coordinates keep their value, the feed is in the move')
check(c[2]['y'] == 30 and c[2]['f'] == 300, 'the feed is modal')
check(c[3]['z'] == 10 and c[3]['x'] == 20, 'a rapid after a feed move')

print('relative distances and inches')
c = translate('G91\nG1 X5 F100\nX5\nG90\nX0')
check([m['x'] for m in c] == [5, 10, 0], 'G91 adds, G90 goes back to absolute: %s' % [m['x'] for m in c])
c = translate('G20\nG1 X1 Y2 F10')
check(close(c[0]['x'], 25.4) and close(c[0]['y'], 50.8) and close(c[0]['f'], 254.0), 'inches give millimeters, feed included')
c = translate('G20 G1 X1 F10\nG21\nG1 X1')
check(close(c[0]['x'], 25.4) and close(c[1]['x'], 1.0), 'G21 goes back to millimeters')

print('arcs')
c = translate('G0 X10 Y0\nG3 X0 Y10 I-10 J0 F200')
a = c[1]
check(a['cmd'] == 'arc' and not a['cw'], 'G3 is counter-clockwise')
check(close(a['cx'], 0) and close(a['cy'], 0), 'the center is relative to the start: %s,%s' % (a['cx'], a['cy']))
c = translate('G0 X0 Y10\nG2 X10 Y0 I0 J-10 F200')
check(c[1]['cw'] and close(c[1]['cx'], 0) and close(c[1]['cy'], 0), 'G2 is clockwise')
c = translate('G0 X10 Y0\nG3 X0 Y10 R10 F200')
check(close(c[1]['cx'], 0) and close(c[1]['cy'], 0), 'a radius R gives the small arc: center %s,%s' % (c[1]['cx'], c[1]['cy']))
c = translate('G0 X10 Y0\nG2 X0 Y10 R10 F200')
check(close(c[1]['cx'], 10) and close(c[1]['cy'], 10), 'G2 with R: the center is on the other side: %s,%s' % (c[1]['cx'], c[1]['cy']))
c = translate('G0 X10 Y0\nG3 X0 Y10 R-10 F200')
check(close(c[1]['cx'], 10) and close(c[1]['cy'], 10), 'a negative R gives the large arc: %s,%s' % (c[1]['cx'], c[1]['cy']))
c = translate('G0 X10 Y0\nG3 X10 Y0 I-10 J0 F200')
check(c[1]['x'] == 10 and c[1]['y'] == 0 and close(c[1]['cx'], 0), 'a full circle has the same start and end')
c = translate('G0 X10 Y0\nG3 X0 Y10 Z-2 I-10 J0 F200')
check(c[1]['z'] == -2, 'a helix: Z changes along the arc')
c = translate('G91.1\nG90.1\nG0 X10 Y0\nG3 X0 Y10 I0 J0 F200')
check(close(c[1]['cx'], 0) and close(c[1]['cy'], 0), 'G90.1: the center is absolute')
c = translate('G18\nG0 X10 Z0\nG3 X0 Z10 I-10 K0 F100')
check(c[1]['plane'] == 'xz' and close(c[1]['cx'], 0) and close(c[1]['cz'], 0), 'plane XZ: the center uses I and K: %s' % c[1])
c = translate('G19\nG0 Y10 Z0\nG3 Y0 Z10 J-10 K0 F100')
check(c[1]['plane'] == 'yz' and close(c[1]['cy'], 0) and close(c[1]['cz'], 0), 'plane YZ: the center uses J and K: %s' % c[1])
c = translate('G0 X0 Y0\nG2 X5 Y5 F100')
check(c[-1]['cmd'] == 'message' and c[-1]['level'] == 'error', 'an arc without center nor radius is an error')
c = translate('G0 X0 Y0\nG2 X50 Y0 R5 F100')
check(c[-1]['cmd'] == 'message' and c[-1]['level'] == 'error', 'a radius too small is an error')

print('machine commands')
c = translate('T2 M6\nS12000 M3\nM8\nM9\nM5\nM0\nM1\nM30')
check(kinds(c) == ['tool', 'spindle', 'coolant', 'coolant', 'spindle', 'stop', 'stop', 'stop'], 'M codes: %s' % kinds(c))
check(c[0]['number'] == 2, 'the tool number of T2 M6')
check(c[1]['state'] == 'cw' and c[1]['rpm'] == 12000, 'M3 with the speed S of the same line')
check(c[2]['state'] == 'flood' and c[3]['state'] == 'off', 'M8 and M9')
check(c[4]['state'] == 'off', 'M5')
check(not c[5]['optional'] and c[6]['optional'] and not c[5]['end'], 'M0 stops, M1 is optional')
check(c[7]['end'], 'M30 ends the program')
c = translate('M4 S500\nS800')
check(c[0]['state'] == 'ccw' and c[1]['state'] == 'ccw' and c[1]['rpm'] == 800, 'a new speed with the spindle on is sent')
c = translate('G4 P2.5\nG4 X1')
check(c[0]['cmd'] == 'dwell' and c[0]['seconds'] == 2.5 and c[1]['seconds'] == 1, 'dwell with P and with X')
c = translate('G4 P500', {'dwell_p_ms': True})
check(c[0]['seconds'] == 0.5, 'P in milliseconds when the option says so')

print('comments and text')
c = translate('(a comment)\n; another\nG0 X1 ; moves\n(MSG, hello operator)\n%\n/G0 X99\nN10 G0 X2')
check(kinds(c) == ['rapid', 'message', 'rapid'], 'comments are ignored, MSG is shown, block delete and the program delimiter are skipped: %s' % kinds(c))
check(c[1]['level'] == 'info' and c[1]['text'] == 'hello operator', 'the message of MSG')
check(c[0]['x'] == 1 and c[2]['x'] == 2, 'a comment on a line of code, a line number')
c = translate('g1 x5 f10\nG1X6Y7')
check(c[0]['x'] == 5 and c[1]['x'] == 6 and c[1]['y'] == 7, 'lower case and no space between the words')

print('canned cycles')
c = translate('G0 Z10\nG98 G81 X5 Y5 Z-3 R2 F100')
check(kinds(c[1:]) == ['rapid', 'rapid', 'linear', 'rapid'], 'G81: to the hole, to R, down, up: %s' % kinds(c[1:]))
check(c[1]['z'] == 10 and c[2]['z'] == 2 and c[3]['z'] == -3 and c[4]['z'] == 10, 'G98 comes back to the initial height: %s' % [m['z'] for m in c[1:]])
c = translate('G0 Z10\nG99 G81 X5 Y5 Z-3 R2 F100')
check(c[-1]['z'] == 2, 'G99 stays at the R plane')
c = translate('G0 Z10\nG99 G81 X5 Y5 Z-3 R2 F100\nX15\nG80\nG0 Z10')
check([m['x'] for m in c if m['cmd'] == 'linear'] == [5, 15], 'the cycle is modal: a second hole with only X')
c = translate('G0 Z10\nG99 G82 X0 Y0 Z-1 R2 P1.5 F100')
check('dwell' in kinds(c) and [m for m in c if m['cmd'] == 'dwell'][0]['seconds'] == 1.5, 'G82 waits at the bottom')
c = translate('G0 Z10\nG99 G83 X0 Y0 Z-9 R2 Q4 F100')
bottoms = [m['z'] for m in c if m['cmd'] == 'linear']
check(bottoms == [-2, -6, -9], 'G83 goes down by pecks of Q: %s' % bottoms)

print('warnings and errors')
c = translate('G93\nG93\nG41 D1\nG0 X1\nG92 X0\nM99\nM99')
levels = [(m['level'], m['text'][:12]) for m in c if m['cmd'] == 'message']
check(len([m for m in c if m['cmd'] == 'message' and 'inverse' in m['text']]) == 1, 'a warning is given once')
check(len([m for m in c if m['cmd'] == 'message' and 'G41' in m['text']]) == 1, 'the cutter compensation is said to be not simulated, once')
check(len([m for m in c if m['cmd'] == 'message' and 'M99' in m['text']]) == 1, 'an unknown M code, once')
c = translate('G1 X1')
check(any(m['cmd'] == 'message' and 'feed' in m['text'] for m in c) and c[-1]['f'] == 1.0, 'a feed move with no F is reported')
t = Translator()
t.start({})
check(isinstance(t.start({'a': 1}), dict) and t.start({})['unit'] == 'mm', 'start returns the information')
t.next('G1 X1 F10')
t.start({})
check(t.next('G1 Y2 F10')[0]['x'] == 0, 'start resets the position')
check(isinstance(t.next('G1 X'), list) and isinstance(t.next('@@@ ###'), list), 'a broken line does not raise')

print('a program as the post-processor writes it')
program = '''G21 G90
T1 M6
S10000 M3
G0 X0 Y0 Z5
G0 X10 Y10
G1 Z-1 F200
G1 X20 F800
G2 X30 Y20 I0 J10
G1 X30 Y40
G0 Z5
M5
M30'''
c = translate(program)
check(kinds(c) == ['tool', 'spindle', 'rapid', 'rapid', 'linear', 'linear', 'arc', 'linear', 'rapid', 'spindle', 'stop'], 'the commands of the program: %s' % kinds(c))
check(c[6]['cmd'] == 'arc' and close(c[6]['cx'], 20) and close(c[6]['cy'], 20), 'the arc of the program')
check(close(math.hypot(c[6]['x'] - c[6]['cx'], c[6]['y'] - c[6]['cy']), math.hypot(20 - c[6]['cx'], 10 - c[6]['cy'])), 'the arc has the same radius at its start and its end')

print('the example translator of the guide (tests/sim/points_translator.py)')
import importlib.util
spec = importlib.util.spec_from_file_location('points_translator', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sim', 'points_translator.py'))
points = importlib.util.module_from_spec(spec)
spec.loader.exec_module(points)
example = points.Translator()
check(example.start({})['unit'] == 'mm', 'the example gives the unit')
rows = [example.next(l) for l in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sim', 'points.txt')).read().splitlines()]
commands = [c for row in rows for c in row]
check(commands[0]['cmd'] == 'rapid' and commands[0]['z'] == 5, 'the first point is a rapid move')
check(commands[1]['cmd'] == 'linear' and commands[1]['f'] == 200 and commands[2]['f'] == 600, 'the feed of a point, and the default one')
check([c['level'] for c in commands if c['cmd'] == 'message'] == ['error', 'warning'], 'a bad line is an error, a short one a warning')
check(len([c for c in commands if c['cmd'] != 'message']) == 8, 'the other points are moves')
example.close()

print('%d checks, %d failed' % (run, failed))
sys.exit(1 if failed else 0)
