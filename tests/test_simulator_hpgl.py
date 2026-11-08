# SIMULATOR TRANSLATOR TEST - checks the translator of HPGL (openpostpro/simulator/hpgl.py).
#
# What for:
#   The translator is the only part of the simulator which knows HPGL. A mistake here shows a wrong path (an arc on the wrong side,
#   a pen which draws when it is up, coordinates in the wrong unit).
#
# What is checked: the pen up and down (z), absolute and relative coordinates, several points in a statement, statements without
#   separator, the arcs (center absolute and relative, the two directions, a full circle), the circle, the pen (tool), the speed, the
#   labels and the commands which are not translated (a warning, once), and the output of the HPGL post-processor of the application.
#
# Run: python tests/test_simulator_hpgl.py   (exit code 0 = all checks pass)
import importlib.util
import math
import os
import sys

sys.dont_write_bytecode = True
base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro')
sys.path.insert(0, os.path.join(base, 'postpro'))


# the translator and the post-processor are both named hpgl.py: each one is loaded by its path
def load(name, path):
    spec = importlib.util.spec_from_file_location(name, os.path.join(base, path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


Translator = load('hpgl_translator', os.path.join('simulator', 'hpgl.py')).Translator

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
        result += t.next(line)
    return result


def moves(commands):
    return [c for c in commands if c['cmd'] in ('rapid', 'linear', 'arc')]


# the unit is the millimeter
t = Translator()
check(t.start({}).get('unit') == 'mm', 'the unit is the millimeter')

# the pen up moves fast above the material, the pen down draws in it
c = translate('IN;PU 400,0;PD 800,0;PU;')
check(c[0] == {'cmd': 'rapid', 'x': 10.0, 'y': 0.0, 'z': 5.0}, 'pen up: a rapid move at z_up, got %s' % c[0])
check(c[1] == {'cmd': 'torch', 'state': 'on'}, 'the pen goes down: the torch is lit')
check(c[2]['cmd'] == 'linear' and c[2]['z'] == -1.0 and c[2]['x'] == 10.0, 'the pen goes down where it is')
check(c[3]['cmd'] == 'linear' and close(c[3]['x'], 20.0) and c[3]['y'] == 0.0 and c[3]['z'] == -1.0, 'pen down: a move in the material')
check(c[4] == {'cmd': 'rapid', 'x': 20.0, 'y': 0.0, 'z': 5.0}, 'the pen goes up where it is')
check(c[5] == {'cmd': 'torch', 'state': 'off'}, 'the pen goes up: the torch is off')

# the options of the heights and of the feed
c = translate('PU 0,0;PD 40,0;', {'z_up': 2.0, 'z_down': -0.2, 'feed': 1234.0})
check(c[0]['z'] == 2.0 and c[2]['z'] == -0.2 and c[3]['f'] == 1234.0, 'z_up, z_down and feed are options')

# several points in one statement, no separator between the statements, spaces and no spaces
c = moves(translate('PU0,0PD40,0,40,40,0,40;'))
check([(m['x'], m['y']) for m in c if m['cmd'] == 'linear'][1:] == [(1.0, 0.0), (1.0, 1.0), (0.0, 1.0)], 'several points and no separator')

# absolute and relative
c = moves(translate('PU 40,40;PR 40,0,0,40;PA 0,0;'))
check([(m['x'], m['y']) for m in c] == [(1.0, 1.0), (2.0, 1.0), (2.0, 2.0), (0.0, 0.0)], 'PR is relative, PA is absolute again')

# a pen up never draws
c = translate('PU 0,0;PA 80,80;PR 40,40;')
check(all(m['cmd'] == 'rapid' for m in moves(c)), 'the pen up does not draw')

# the arcs: center absolute, the sweep angle gives the direction and the end
c = translate('PU 400,0;PD;AA 0,0,90;')
a = [m for m in c if m['cmd'] == 'arc'][0]
check(close(a['x'], 0.0, 1e-6) and close(a['y'], 10.0, 1e-6) and a['cw'] is False and close(a['cx'], 0.0) and a['plane'] == 'xy', 'AA 90 degrees from (10, 0) around the origin: %s' % a)
c = translate('PU 400,0;PD;AA 0,0,-90;')
a = [m for m in c if m['cmd'] == 'arc'][0]
check(close(a['x'], 0.0, 1e-6) and close(a['y'], -10.0, 1e-6) and a['cw'] is True, 'AA -90 degrees goes the other way round')
c = translate('PU 400,0;PD;AR -400,0,180;')
a = [m for m in c if m['cmd'] == 'arc'][0]
check(close(a['x'], -10.0, 1e-6) and close(a['y'], 0.0, 1e-6) and close(a['cx'], 0.0), 'AR has its center relative to the pen')
c = translate('PU 400,0;PD;AA 0,0,360;')
a = [m for m in c if m['cmd'] == 'arc'][0]
check(close(a['x'], 10.0) and close(a['y'], 0.0), 'a sweep of 360 degrees is a full circle back to the start')
# a position after the arc: the next point is a line from the end of the arc
c = translate('PU 400,0;PD;AA 0,0,90;PD 0,0;')
check(close(moves(c)[-1]['x'], 0.0) and moves(c)[-1]['cmd'] == 'linear', 'the end of the arc is the position')

# the circle around the current point, the pen is where it was
c = translate('PU 400,400;CI 200;')
check(any(m['cmd'] == 'arc' and close(m['cx'], 10.0) and close(m['cy'], 10.0) for m in c), 'CI makes an arc around the current point')
last = moves(c)[-1]
check(last['cmd'] == 'rapid' and close(last['x'], 10.0) and close(last['y'], 10.0), 'after the circle the pen is at its center')

# the pen is the tool, SP0 puts it away
c = translate('SP2;PU 0,0;PD 40,0;SP0;SP2;')
check([x for x in c if x['cmd'] == 'tool'] == [{'cmd': 'tool', 'number': 2}, {'cmd': 'tool', 'number': 2}] or len([x for x in c if x['cmd'] == 'tool']) >= 1, 'SP gives the tool')
check(any(x['cmd'] == 'torch' and x['state'] == 'off' for x in c), 'SP0 puts the pen up')

# the speed in cm/s is the feed in mm/min
c = translate('VS 10;PU 0,0;PD 40,0;')
check(close([m for m in c if m['cmd'] == 'linear'][-1]['f'], 6000.0), 'VS 10 cm/s is 6000 mm/min')

# a label is not translated, whatever it holds; it does not stop the statements which follow
c = translate('PU 0,0;LB TEXT;PD 40,0;\x03PU 80,0;')
check(all(m['cmd'] != 'message' or m['level'] != 'error' for m in c), 'a label is no error')
check(any(m['cmd'] == 'rapid' and close(m['x'], 2.0) for m in c), 'what follows a label is translated')

# a command which is not translated: a warning, once
c = translate('PE <8;\nPE >3;')
warnings = [m for m in c if m['cmd'] == 'message' and m['level'] == 'warning']
check(len(warnings) == 1 and 'PE' in warnings[0]['text'], 'PE is a warning, once')
# the commands without effect
c = translate('IN;SP1;PW 0.3;LT;FS 10;CO "a comment";')
check(not [m for m in c if m['cmd'] == 'message'], 'the commands without effect are silent')

# a coordinate without its pair
c = translate('PU 40,40,80;')
check(any(m['cmd'] == 'message' for m in c) and len(moves(c)) == 1, 'an odd number of coordinates is a warning')

# a line which is not HPGL does not stop the translation
c = translate('this is not HPGL at all\nPU 40,40;')
check(len(moves(c)) == 1, 'the next line is translated')

# what the HPGL post-processor of the application writes
import postcore  # noqa: E402
post_module = load('hpgl_postpro', os.path.join('postpro', 'hpgl.py'))
post = post_module.Postpro()
core = postcore.Postcore()
core.set_postpro(post)
post.set_core(core)
core.set_language('en-us')
core.set_unit('mm')
text = core.start_loop() + core.start_program()
text += core.rapid(['X', 'Y'], [10.0, 0.0])
text += core.linear(['X', 'Y'], [20.0, 0.0])
text += core.counter_clockwise(['X', 'Y', 'I', 'J'], [0.0, 0.0, -10.0, 0.0])
text += core.stop_program() + core.stop_loop()
tr = Translator()
tr.start({})
commands = []
for line in text.split('\n'):
    commands += tr.next(line)
drawn = [m for m in commands if m['cmd'] == 'linear']
check(len(drawn) > 20, 'the post-processor draws many segments (the arc is cut)')
check(all(abs(math.hypot(m['x'] - 10.0, m['y']) - 10.0) < 0.03 for m in drawn[1:]), 'the points of the half circle are at the radius 10 of its center')
check(close(drawn[-1]['x'], 0.0, 0.03) and close(drawn[-1]['y'], 0.0, 0.03), 'the half circle ends at the origin')
check(max(m['y'] for m in drawn) > 9.0, 'the half circle goes through positive y')
check(not [m for m in commands if m['cmd'] == 'message' and m['level'] in ('warning', 'error')], 'the output of the post-processor is translated without any warning')

print('%d checks, %d failed' % (run, failed))
sys.exit(1 if failed else 0)
