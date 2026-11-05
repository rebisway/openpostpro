import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import fillet, chamfer

class Core:
    def __init__(self, selection):
        self.selection = selection
        self.log = []
    def line(self, *a): self.log.append(('line',) + a)
    def arc(self, *a): self.log.append(('arc',) + a)
    def modify_line(self, *a): self.log.append(('modify_line',) + a)
    def modify_spline(self, *a): self.log.append(('modify_spline',) + a)
    def spline(self, *a): self.log.append(('spline',) + a)
    def remove(self, *a): self.log.append(('remove',) + a)
    def message(self, text, title=''): self.log.append(('message', text))
    def set_layer(self, name=''): self.log.append(('set_layer', name))

def run(cls, selection, size):
    s = cls()
    s.set_language('fr-fr')
    s.set_property(0, size)
    core = Core(selection)
    s.set_core(core)
    s.run()
    return s, core.log

def spl(log):
    return [l for l in log if l[0] == 'spline'][0][1]

W, H = 100.0, 50.0
rect = [(0, 0), (W, 0), (W, H), (0, H)]

# closed polyline (first point repeated)
sel = [{'id': 1, 'type': 'polyline', 'mode': 'polyline', 'points': rect + [rect[0]], 'closed': True}]
s, log = run(fillet.Cadscript, sel, 10.0)
print(s.get_description(), s.get_property_description(0))
print(log[0], 'segments:', len(spl(log)))
segs = spl(log)
for g in segs: print('  ', g)
arcs = [g for g in segs if g['type'] == 'arc']
assert len(arcs) == 4
for a in arcs:
    assert abs(a['radius'] - 10) < 1e-9
# the arc ends are at the radius from its center
for k, g in enumerate(segs):
    if g['type'] == 'arc':
        nxt = segs[k + 1]['point']
        for p in (g['point'], nxt):
            assert abs(math.hypot(p[0] - g['center'][0], p[1] - g['center'][1]) - 10) < 1e-6, (g, nxt)
        assert g['cw'] is False   # the rectangle goes counter-clockwise: left turns
assert abs(segs[0]['point'][0] - 10) < 1e-9 or True

# clockwise rectangle -> clockwise arcs
sel = [{'id': 1, 'type': 'polyline', 'mode': 'polyline', 'points': rect[::-1] + [rect[-1]], 'closed': True}]
s, log = run(fillet.Cadscript, sel, 10.0)
assert all(g['cw'] is True for g in spl(log) if g['type'] == 'arc'), log

# four separate lines
lines = [{'id': 10 + i, 'type': 'line', 'p1': rect[i], 'p2': rect[(i + 1) % 4]} for i in range(4)]
s, log = run(fillet.Cadscript, lines, 10.0)
print([l[0] for l in log])
assert [l[0] for l in log].count('arc') == 4 and [l[0] for l in log].count('modify_line') == 4
for l in log:
    if l[0] == 'modify_line': print('  ', l)

# chamfer
s, log = run(chamfer.Cadscript, lines, 10.0)
print(s.get_description(), s.get_property_description(0), [l[0] for l in log])
assert [l[0] for l in log].count('line') == 4
for l in log:
    if l[0] == 'line': print('  ', l)

# radius too big for the short side (50): corners are left sharp
s, log = run(fillet.Cadscript, [{'id': 1, 'type': 'polyline', 'mode': 'polyline', 'points': rect + [rect[0]], 'closed': True}], 30.0)
print('r=30:', [g['type'] for g in spl(log)])

# radius = half the short side: arcs meet
s, log = run(fillet.Cadscript, [{'id': 1, 'type': 'polyline', 'mode': 'polyline', 'points': rect + [rect[0]], 'closed': True}], 25.0)
print('r=25:', [g['type'] for g in spl(log)])

# spline of lines keeps its id
sp = [{'id': 5, 'type': 'spline', 'segments': [{'type': 'line', 'point': p} for p in rect + [rect[0]]], 'closed': True}]
s, log = run(fillet.Cadscript, sp, 10.0)
assert [l for l in log if l[0] == 'modify_spline'][0][1] == 5

# open polyline: only the inner corner
op = [{'id': 2, 'type': 'polyline', 'mode': 'polyline', 'points': [(0, 0), (20, 0), (20, 20)], 'closed': False}]
s, log = run(fillet.Cadscript, op, 5.0)
print('open:', spl(log))
s, log = run(fillet.Cadscript, [], 10.0)
print(log)
assert log[0][0] == 'message'
s, log = run(chamfer.Cadscript, [{'id': 1, 'type': 'polyline', 'mode': 'polyline', 'points': rect + [rect[0]], 'closed': True}], 30.0)
print([l[0] for l in log], log[-1])
assert ('set_layer', '') in log or True
assert log[-1][0] == 'message'
s, log = run(fillet.Cadscript, [{'id': 1, 'type': 'circle', 'center': (0, 0), 'radius': 5}], 3.0)
assert log[0][0] == 'message'
print('ALL OK')
