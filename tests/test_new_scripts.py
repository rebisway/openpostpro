# Spokes, hexagon, gravity, exterior gear, airfoil file and potrace parser, run against a fake engine.
import math
import os, sys, tempfile
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import star, rectangle, spokes, hexagon, gravity, exterior_gear, airfoil_file, potrace


class Core:
    mouse_x = 0.0
    mouse_y = 0.0

    def __init__(self, selection=()):
        self.selection = list(selection)
        self.log = []

    def __getattr__(self, name):
        return lambda *a: self.log.append((name,) + a)

    def calls(self, name):
        return [l for l in self.log if l[0] == name]


def run(module, props, selection=(), mouse=(0.0, 0.0)):
    s = module.Cadscript()
    s.set_language('en-us')
    for i, v in enumerate(props):
        s.set_property(i, v)
    core = Core(selection)
    core.mouse_x, core.mouse_y = mouse
    s.set_core(core)
    s.run()
    return core


# star: 5 branches, tips on the circle, inner points on the pentagram, first tip up
core = run(star, [5, 100], mouse=(10, 10))
pts = core.calls('polyline')[0][1]
assert len(pts) == 11 and pts[0] == pts[-1]
assert abs(pts[0][0] - 10) < 1e-9 and abs(pts[0][1] - 60) < 1e-9
assert abs(math.hypot(pts[1][0] - 10, pts[1][1] - 10) - 50 * 0.381966) < 1e-4
assert run(star, [2, 100]).calls('message')

# rectangle: four lines from the mouse
core = run(rectangle, [30, 20], mouse=(1, 2))
assert [l[1:] for l in core.calls('line')] == [(1, 2, 31.0, 2), (31.0, 2, 31.0, 22.0), (31.0, 22.0, 1, 22.0), (1, 22.0, 1, 2)]

# spokes: 3 windows of 2 lines and 2 arcs, all points on the circles
core = run(spokes, [20, 3, 100])
assert len(core.calls('line')) == 6 and len(core.calls('arc')) == 6 and not core.calls('message')
for l in core.calls('line'):
    assert l[1:3] != l[3:5]
for a in core.calls('arc'):
    assert abs(math.hypot(a[1], a[2]) - math.hypot(a[5], a[6])) < 1e-9
assert run(spokes, [20, 1, 100]).calls('message')
assert run(spokes, [20, 3, 15]).calls('message')

# hexagon around the mouse
core = run(hexagon, [10], mouse=(5, 5))
pts = core.calls('polyline')[0][1]
assert len(pts) == 7 and pts[0] == pts[-1] and abs(pts[0][0] - 15) < 1e-9
assert all(abs(math.hypot(x - 5, y - 5) - 10) < 1e-9 for x, y in pts)

# gravity: square from 4 lines, a closed polyline triangle, a circle, a half disc made of a line and an arc
square = [{'type': 'line', 'p1': a, 'p2': b} for a, b in [((0, 0), (4, 0)), ((4, 2), (4, 0)), ((4, 2), (0, 2)), ((0, 0), (0, 2))]]
tri = {'type': 'polyline', 'points': [(0, 10), (6, 10), (0, 13)], 'closed': True, 'mode': 'polyline'}
circ = {'type': 'circle', 'center': (7, 7), 'radius': 1}
half = [{'type': 'line', 'p1': (-1, 20), 'p2': (1, 20)}, {'type': 'arc', 'start': (1, 20), 'center': (0, 20), 'stop': (-1, 20), 'cw': False, 'radius': 1}]
core = run(gravity, [], square + [tri, circ] + half)
pts = [(round(p[1], 3), round(p[2], 3)) for p in core.calls('point')]
assert (2.0, 1.0) in pts and (2.0, 11.0) in pts and (7, 7) in pts
assert any(abs(x) < 1e-6 and abs(y - 20 - 4 / (3 * math.pi)) < 0.01 for x, y in pts), pts
assert len(pts) == 4
assert run(gravity, [], [{'type': 'line', 'p1': (0, 0), 'p2': (1, 1)}]).calls('message')

# exterior gear
core = run(exterior_gear, [5, 20, 20, 0, 0, 1.25, False, 3, 10], mouse=(100, 0))
pts = core.calls('polyline')[0][1]
radii = [math.hypot(x - 100, y) for x, y in pts]
assert pts[0] == pts[-1]
assert abs(max(radii) - 55) < 1e-6 and abs(min(radii) - (50 - 6.25)) < 1e-6, (max(radii), min(radii))
assert not core.calls('line')
core = run(exterior_gear, [5, 20, 20, 0, 0, 1.25, True, 3, 5])
assert len(core.calls('line')) == 6 and not core.calls('message')
assert run(exterior_gear, [5, 2, 20, 0, 0, 1.25, False, 3, 10]).calls('message')
# the tooth is symmetric: the point set is the same after a half turn of teeth
pts = run(exterior_gear, [5, 20, 20, 0, 0, 1.25, False, 3, 10]).calls('polyline')[0][1]
n = (len(pts) - 1) // 20
a = 2 * math.pi / 20
assert all(abs(pts[i][0] * math.cos(a) - pts[i][1] * math.sin(a) - pts[i + n][0]) < 1e-6 and abs(pts[i][0] * math.sin(a) + pts[i][1] * math.cos(a) - pts[i + n][1]) < 1e-6 for i in range(n))

# airfoil: Selig then Lednicer
selig = "TEST\n1.0 0.0\n0.5 0.1\n0.0 0.0\n0.5 -0.1\n1.0 0.0\n"
lednicer = "TEST\n3. 3.\n\n0.0 0.0\n0.5 0.1\n1.0 0.0\n\n0.0 0.0\n0.5 -0.1\n1.0 0.0\n"
for text in (selig, lednicer):
    p = airfoil_file.parse(text)
    assert p is not None and len(p) in (5,) and (0.5, 0.1) in p and (0.5, -0.1) in p, p
assert airfoil_file.parse('nothing here') is None
with tempfile.TemporaryDirectory() as d:
    path = os.path.join(d, 'a.dat')
    open(path, 'w').write(selig)
    core = run(airfoil_file, [path, 200, False], mouse=(10, 20))
    pts = core.calls('polyline')[0][1]
    assert pts[0] == (210.0, 20.0) and (110.0, 40.0) in [(round(x, 6), round(y, 6)) for x, y in pts] and pts[-1] == pts[0]

# potrace svg
svg = '<svg width="100pt" height="50pt"><g transform="translate(0,50) scale(0.1,-0.1)"><path d="M100 100 l 300 0 l 0 300 z M0 0 c 0 100 100 100 100 0 z"/></g></svg>'
contours, w = potrace.parse_svg(svg)
assert w == 100 and len(contours) == 2
assert contours[0] == [(10.0, 10.0), (40.0, 10.0), (40.0, 40.0), (10.0, 10.0)], contours[0]
assert contours[1][0] == (0.0, 0.0) and contours[1][-1] == (0.0, 0.0) and len(contours[1]) == potrace.STEPS + 2
assert potrace.parse_svg('<g/>') is None
s = potrace.Cadscript(); s.set_language('en-us'); assert s.use_properties() is False and s.get_properties_count() == 0
print('ok')
