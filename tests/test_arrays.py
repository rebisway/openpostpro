# Arrays and hole grid scripts, run against a fake engine.
import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import array_linear, array_polar, hole_grid


class Core:
    mouse_x = 0.0
    mouse_y = 0.0

    def __init__(self, selection):
        self.selection = selection
        self.log = []

    def __getattr__(self, name):
        return lambda *a: self.log.append((name,) + a)

    def calls(self, name):
        return [l for l in self.log if l[0] == name]


def run(module, props, selection):
    s = module.Cadscript()
    s.set_language('en-us')
    for i, v in enumerate(props):
        s.set_property(i, v)
    core = Core(selection)
    s.set_core(core)
    s.run()
    return core


selection = [
    {'id': 1, 'layer': 'L', 'type': 'line', 'p1': (10, 0), 'p2': (20, 0)},
    {'id': 2, 'layer': '', 'type': 'circle', 'center': (5, 5), 'radius': 2},
    {'id': 3, 'layer': '', 'type': 'arc', 'start': (1, 0), 'center': (0, 0), 'stop': (0, 1), 'cw': False, 'radius': 1},
    {'id': 4, 'layer': '', 'type': 'point', 'p1': (1, 1)},
    {'id': 5, 'layer': '', 'type': 'ellipse', 'center': (0, 0), 'major': 3, 'minor': 1, 'start': 0, 'stop': 6.28, 'angle': 0.1},
    {'id': 6, 'layer': '', 'type': 'polyline', 'points': [(0, 0), (1, 1)], 'mode': 'polyline', 'closed': False},
    {'id': 7, 'layer': '', 'type': 'spline', 'closed': False, 'segments': [
        {'type': 'line', 'point': (0, 0)},
        {'type': 'arc', 'point': (1, 0), 'center': (1, 1), 'radius': 1, 'cw': True},
        {'type': 'line', 'point': (2, 2)}]},
    {'id': 8, 'layer': '', 'type': 'text', 'p1': (0, 0), 'text': 'a', 'family': 'Arial', 'height': 5, 'inter': 0, 'bold': False, 'italic': False},
]
shapes = len(selection)

# linear: 2 copies, step (50, 10)
core = run(array_linear, [2, 50, 10], selection)
assert core.calls('line')[0][1:] == (60.0, 10.0, 70.0, 10.0)
assert core.calls('line')[1][1:] == (110.0, 20.0, 120.0, 20.0)
assert len(core.calls('circle')) == 2 and len(core.calls('spline')) == 2
assert core.calls('set_layer')[0] == ('set_layer', 'L')

# polar: 3 copies of 90 degrees around the origin
core = run(array_polar, [3, 90], selection)
x1, y1, x2, y2 = core.calls('line')[0][1:]
assert abs(x1) < 1e-9 and abs(y1 - 10) < 1e-9 and abs(x2) < 1e-9 and abs(y2 - 20) < 1e-9
assert abs(core.calls('ellipse')[0][7] - (0.1 + math.pi / 2)) < 1e-9
assert len(core.calls('circle')) == 3

# no selection: a message, nothing drawn
for module in (array_linear, array_polar):
    core = run(module, [], [])
    assert [l[0] for l in core.log] == ['message']

# grid: 3 x 2 holes of diameter 4
core = run(hole_grid, [3, 2, 10, 10, 4], [])
circles = core.calls('circle')
assert len(circles) == 6 and circles[-1][1:] == (20.0, 10.0, 2.0)
print('ARRAYS OK')
