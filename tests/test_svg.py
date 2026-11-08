# SVG import (svgparse.py, svg_import.py) and export (svg_export.py), run against a fake engine.
import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import svgparse, svg_import, svg_export


def near(a, b, tol=1e-3):
    return abs(a - b) <= tol


def point_near(p, x, y, tol=1e-3):
    return near(p[0], x, tol) and near(p[1], y, tol)


HEAD = '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" width="100mm" height="50mm" viewBox="0 0 100 50">'

# ---- the size: millimeters, Y up, the lower left corner at (0, 0)
shapes = svgparse.parse(HEAD + '<line x1="10" y1="10" x2="30" y2="20"/><rect x="0" y="0" width="20" height="10"/></svg>')
line = [s for s in shapes if s['type'] == 'line'][0]
assert point_near(line['p1'], 10, 40) and point_near(line['p2'], 30, 30), line
rect = [s for s in shapes if s['type'] == 'polyline'][0]
assert len(rect['points']) == 5 and rect['points'][0] == rect['points'][-1], rect
assert point_near(rect['points'][0], 0, 50) and point_near(rect['points'][2], 20, 40), rect['points']

# ---- units: a viewBox in points
shapes = svgparse.parse('<svg xmlns="http://www.w3.org/2000/svg" width="72pt" height="36pt" viewBox="0 0 72 36"><line x1="0" y1="0" x2="72" y2="0"/></svg>')
assert near(shapes[0]['p2'][0] - shapes[0]['p1'][0], 25.4), shapes
# no size: pixels
shapes = svgparse.parse('<svg xmlns="http://www.w3.org/2000/svg" height="96px"><line x1="0" y1="0" x2="96" y2="0"/></svg>')
assert near(shapes[0]['p2'][0], 25.4) and near(shapes[0]['p1'][1], 25.4), shapes
# the scale of the script
shapes = svgparse.parse(HEAD + '<line x1="0" y1="0" x2="10" y2="0"/></svg>', 2.0)
assert near(shapes[0]['p2'][0], 20), shapes

# ---- circles, ellipses and the transforms
shapes = svgparse.parse(HEAD + '<circle cx="50" cy="25" r="5"/><g transform="translate(10 0) scale(2)"><circle cx="5" cy="5" r="1"/></g>'
    '<ellipse cx="20" cy="20" rx="8" ry="4"/><g transform="scale(1 2)"><circle cx="5" cy="5" r="1"/></g></svg>')
circles = [s for s in shapes if s['type'] == 'circle']
assert len(circles) == 2 and point_near(circles[0]['center'], 50, 25) and near(circles[0]['radius'], 5), circles
assert point_near(circles[1]['center'], 20, 40) and near(circles[1]['radius'], 2), circles[1]
ellipse = [s for s in shapes if s['type'] == 'ellipse'][0]
assert near(ellipse['major'], 16) and near(ellipse['minor'], 8) and near(ellipse['angle'], 0), ellipse
# a distorted circle is an ellipse cut into segments
distorted = [s for s in shapes if s['type'] == 'polyline']
assert len(distorted) == 1 and len(distorted[0]['points']) > 20, distorted

# ---- rotation
shapes = svgparse.parse(HEAD + '<g transform="rotate(90 50 25)"><line x1="40" y1="25" x2="60" y2="25"/></g></svg>')
assert point_near(shapes[0]['p1'], 50, 25 + 10) and point_near(shapes[0]['p2'], 50, 25 - 10), shapes[0]

# ---- paths: lines, relative moves, closed, H V
shapes = svgparse.parse(HEAD + '<path d="M 10 10 h 10 v 10 l -10 0 z"/><path d="m 50 10 l 10 0 l 0 10"/></svg>')
closed = shapes[0]
assert closed['type'] == 'polyline' and len(closed['points']) == 5 and closed['points'][0] == closed['points'][-1], closed
assert point_near(closed['points'][1], 20, 40) and point_near(closed['points'][2], 20, 30), closed['points']
opened = shapes[1]
assert len(opened['points']) == 3 and point_near(opened['points'][2], 60, 30), opened

# ---- curves are cut into segments and end on the end point
shapes = svgparse.parse(HEAD + '<path d="M 0 50 C 10 0 30 0 40 50"/><path d="M 0 0 Q 10 20 20 0"/></svg>')
cubic = shapes[0]['points']
assert len(cubic) > 10 and point_near(cubic[0], 0, 0) and point_near(cubic[-1], 40, 0), (len(cubic), cubic[0], cubic[-1])
# the middle of the cubic (t = 1/2): x = (0 + 3 * 10 + 3 * 30 + 40) / 8 = 20, y of the file = (50 + 3 * 0 + 3 * 0 + 50) / 8 = 12.5, drawing y = 50 - 12.5
mid = min(cubic, key=lambda p: abs(p[0] - 20))
assert near(mid[1], 37.5, 0.1), mid
quad = shapes[1]['points']
assert point_near(quad[-1], 20, 50), quad[-1]

# ---- arcs of a path made of lines and arcs stay exact: a spline
shapes = svgparse.parse(HEAD + '<path d="M 10 10 L 30 10 A 5 5 0 0 1 30 20 L 10 20 A 5 5 0 0 1 10 10 Z"/></svg>')
spline = shapes[0]
assert spline['type'] == 'spline', spline
arcs = [g for g in spline['segments'] if g['type'] == 'arc']
assert len(arcs) == 2 and near(arcs[0]['radius'], 5), arcs
assert point_near(arcs[0]['center'], 30, 35), arcs[0]['center']
assert point_near(spline['segments'][-1]['point'], 10, 40), spline['segments'][-1]
# the arcs turn the same way and the first one goes from (30, 40) to (30, 30), through the right: clockwise
assert arcs[0]['cw'] is True, arcs[0]
# an elliptic arc is cut into segments
shapes = svgparse.parse(HEAD + '<path d="M 10 10 A 10 5 0 0 1 30 10"/></svg>')
assert shapes[0]['type'] == 'polyline' and len(shapes[0]['points']) > 6, shapes[0]

# ---- the rounded rectangle
shapes = svgparse.parse(HEAD + '<rect x="10" y="10" width="30" height="20" rx="4"/></svg>')
assert shapes[0]['type'] == 'spline' and len([g for g in shapes[0]['segments'] if g['type'] == 'arc']) == 4, shapes[0]

# ---- polygons, layers, use, hidden shapes, text
shapes = svgparse.parse(HEAD + '<polygon points="0,0 10,0 10,10"/>'
    '<g inkscape:groupmode="layer" inkscape:label="Cut"><line x1="0" y1="0" x2="5" y2="5"/><g display="none"><line x1="0" y1="0" x2="9" y2="9"/></g></g>'
    '<defs><circle id="c" cx="0" cy="0" r="2"/></defs><use xlink:href="#c" x="10" y="10"/>'
    '<text x="5" y="20" font-size="4" font-family="Arial, sans-serif" font-weight="bold">Hello <tspan>world</tspan></text></svg>')
kinds = [s['type'] for s in shapes]
assert kinds == ['polyline', 'line', 'circle', 'text'], kinds
assert shapes[1]['layer'] == 'Cut' and shapes[0]['layer'] == '', shapes
assert point_near(shapes[2]['center'], 10, 40), shapes[2]
assert shapes[3]['text'] == 'Hello world' and shapes[3]['bold'] and shapes[3]['family'] == 'Arial' and near(shapes[3]['height'], 4), shapes[3]
assert point_near(shapes[3]['p1'], 5, 30), shapes[3]['p1']

# ---- an invalid file
for text in ('not xml', '<html/>'):
    try:
        svgparse.parse(text)
        assert False, 'must raise'
    except ValueError:
        pass


# ---- the script: shapes added at the mouse, layers
class Core:
    mouse_x = 100.0
    mouse_y = 200.0
    unit = 'mm'

    def __init__(self, selection=(), drawing=()):
        self.selection = list(selection)
        self.drawing = list(drawing)
        self.log_lines = []
        self.calls = []

    def __getattr__(self, name):
        return lambda *a: self.calls.append((name,) + a)

    def log(self, text):
        self.log_lines.append(text)


folder = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(folder, 'sample_test.svg')
try:
    with open(path, 'w') as f:
        f.write(HEAD + '<line x1="0" y1="50" x2="10" y2="50"/><g inkscape:groupmode="layer" inkscape:label="L2"><circle cx="5" cy="45" r="2"/></g></svg>')
    script = svg_import.Cadscript()
    script.set_language('en-us')
    script.set_property(0, path)
    script.set_property(1, 1)
    core = Core()
    script.set_core(core)
    script.run()
    names = [c[0] for c in core.calls]
    assert names == ['set_layer', 'line', 'set_layer', 'circle', 'set_layer'], names
    assert core.calls[1][1:] == (100.0, 200.0, 110.0, 200.0), core.calls[1]
    assert core.calls[3][1:] == (105.0, 205.0, 2.0), core.calls[3]
    assert core.calls[2][1] == 'L2' and core.calls[4][1] == '', core.calls
    assert core.log_lines == ['SVG imported: 2 shapes.'], core.log_lines
finally:
    if os.path.exists(path):
        os.remove(path)


# ---- the export
drawing = [
    {'id': 1, 'layer': 'Cut line', 'type': 'line', 'p1': (0.0, 0.0), 'p2': (10.0, 0.0)},
    {'id': 2, 'layer': 'Cut line', 'type': 'circle', 'center': (5.0, 5.0), 'radius': 2.0},
    {'id': 3, 'layer': 'Holes', 'type': 'arc', 'start': (10.0, 0.0), 'center': (0.0, 0.0), 'stop': (0.0, 10.0), 'cw': False, 'radius': 10.0},
    {'id': 4, 'layer': 'Holes', 'type': 'polyline', 'points': [(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 0.0)], 'mode': 'polyline', 'closed': True},
    {'id': 5, 'layer': 'Holes', 'type': 'spline', 'closed': True, 'segments': [
        {'type': 'line', 'point': (0.0, 0.0)},
        {'type': 'arc', 'point': (10.0, 0.0), 'center': (10.0, 5.0), 'radius': 5.0, 'cw': False},
        {'type': 'line', 'point': (10.0, 10.0)},
        {'type': 'line', 'point': (0.0, 0.0)}]},
    {'id': 6, 'layer': '', 'type': 'text', 'p1': (1.0, 1.0), 'text': 'a < b', 'height': 3.0, 'family': 'Arial', 'bold': False, 'italic': True},
    {'id': 7, 'layer': '', 'type': 'point', 'p1': (2.0, 2.0)},
    {'id': 8, 'layer': '', 'type': 'polyline', 'points': [(0.0, 0.0), (1.0, 2.0), (3.0, 3.0), (4.0, 0.0)], 'mode': 'catmull', 'closed': False},
    {'id': 9, 'layer': '', 'type': 'polyline', 'points': [(0.0, 0.0), (1.0, 2.0), (3.0, 3.0)], 'mode': 'bspline', 'closed': False},
]
exporter = svg_export.Cadscript()
exporter.set_language('en-us')
text = exporter.build(drawing)
assert text.startswith('<?xml') and '<svg ' in text and text.rstrip().endswith('</svg>'), text[:100]
assert 'inkscape:label="Cut line"' in text and 'inkscape:label="Holes"' in text, text
assert '&lt;' in text and 'font-style="italic"' in text, text
assert exporter.approximated == 1, exporter.approximated
# the exported file is read back by the importer: same shapes, same places
back = svgparse.parse(text)
by_type = {}
for s in back:
    by_type.setdefault(s['type'], []).append(s)
assert len(by_type['circle']) == 2, by_type.keys()		# the circle and the point
l = by_type['line'][0]
origin = l['p1']		# the point (0, 0) of the drawing, in the coordinates of the file read back (Y up again)
assert near(l['p2'][0] - origin[0], 10) and near(l['p2'][1], origin[1]), l
assert any(point_near(c['center'], origin[0] + 5, origin[1] + 5) and near(c['radius'], 2) for c in by_type['circle']), by_type['circle']
# the arc goes from (10, 0) to (0, 10) around (0, 0), counter clockwise: an exact arc of radius 10 in a spline
splines = [s for s in back if s['type'] == 'spline']
big = [g for s in splines for g in s['segments'] if g['type'] == 'arc' and near(g['radius'], 10)]
assert len(big) == 1, splines
assert point_near(big[0]['center'], origin[0], origin[1]) and big[0]['cw'] is False, big[0]
assert point_near(big[0]['point'], origin[0] + 10, origin[1]), big[0]
small = [g for s in splines for g in s['segments'] if g['type'] == 'arc' and near(g['radius'], 5)]
assert len(small) == 1 and small[0]['cw'] is False, small

# the same drawing in inches: 25.4 times bigger in the file
inch = exporter.build([{'id': 1, 'layer': '', 'type': 'line', 'p1': (0.0, 0.0), 'p2': (1.0, 0.0)}], 'inch')
back = svgparse.parse(inch)
assert near(back[0]['p2'][0] - back[0]['p1'][0], 25.4, 0.01), back

# the script: the selection, else the whole drawing; the file name comes from a dialog (replaced here)
out = os.path.join(folder, 'export_test.svg')
try:
    exporter = svg_export.Cadscript()
    exporter.set_language('en-us')
    exporter.ask_file = lambda: out
    core = Core(drawing=drawing[:2])
    exporter.set_core(core)
    exporter.run()
    with open(out) as f:
        written = f.read()
    assert written.count('<line') == 1 and written.count('<circle') == 1, written
    core = Core(selection=drawing[1:2], drawing=drawing)
    exporter.set_core(core)
    exporter.run()
    with open(out) as f:
        written = f.read()
    assert written.count('<line') == 0 and written.count('<circle') == 1, 'only the selection'
    # nothing at all: a message
    core = Core()
    exporter.set_core(core)
    exporter.run()
    assert core.calls and core.calls[0][0] == 'message', core.calls
finally:
    if os.path.exists(out):
        os.remove(out)
print('ok')
