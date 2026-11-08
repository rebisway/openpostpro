# Gerber and Excellon import (gerberparse.py, gerber_import.py), run against a fake engine.
import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import gerberparse, gerber_import, drawshapes


def near(a, b, tol=1e-6):
    return abs(a - b) <= tol


def pnear(p, q, tol=1e-6):
    return near(p[0], q[0], tol) and near(p[1], q[1], tol)


# every arc of a spline starts and ends on its circle, and the outline is closed
def check_spline(shape):
    segments = shape['segments']
    for i, g in enumerate(segments[:-1]):
        if g['type'] == 'arc':
            nxt = segments[i + 1]['point']
            assert near(math.dist(g['point'], g['center']), g['radius'], 1e-6), ('start of an arc is not on its circle', g)
            assert near(math.dist(nxt, g['center']), g['radius'], 1e-6), ('end of an arc is not on its circle', g, nxt)
    assert pnear(segments[0]['point'], segments[-1]['point']), 'the outline is closed'


GERBER = '''G04 test*
%FSLAX24Y24*%
%MOMM*%
%ADD10C,0.5*%
%ADD11R,2X1*%
%ADD12O,3X1*%
%ADD13P,2X6*%
%ADD14C,1.5,0.6*%
G01*
D10*
X10000Y10000D02*
X50000Y10000D01*
G75*
G03*
X50000Y30000I0J10000D01*
G01*
D11*
X80000Y80000D03*
D12*
X100000Y80000D03*
D13*
X120000Y80000D03*
D14*
X140000Y80000D03*
%LPC*%
D10*
X0Y0D03*
%LPD*%
G36*
X0Y0D02*
G01*
X0Y40000D01*
X40000Y40000D01*
X40000Y0D01*
X0Y0D01*
G37*
M02*
'''

parsed = gerberparse.parse(GERBER)
assert parsed.kind == 'gerber' and parsed.unit == 'mm', (parsed.kind, parsed.unit)
kinds = [o['kind'] for o in parsed.objects]
assert kinds == ['track', 'track', 'flash', 'flash', 'flash', 'flash', 'flash', 'region'], kinds
assert parsed.objects[0]['from'] == (1.0, 1.0) and parsed.objects[0]['segment']['to'] == (5.0, 1.0), parsed.objects[0]
arc = parsed.objects[1]['segment']
assert arc['type'] == 'arc' and arc['cw'] is False and pnear(arc['center'], (5.0, 2.0)), arc
assert parsed.objects[6]['clear'] and not parsed.objects[5]['clear']
assert not parsed.warnings, parsed.warnings

shapes = gerberparse.convert(parsed, True, 'top')
layers = sorted(set(s['layer'] for s in shapes))
assert layers == ['top', 'top_clear'], layers

# the straight track: a capsule of width 0.5 along (1, 1) - (5, 1)
capsule = shapes[0]
assert capsule['type'] == 'spline'
check_spline(capsule)
assert pnear(capsule['segments'][0]['point'], (1.0, 1.25)) and pnear(capsule['segments'][2]['point'], (5.0, 0.75)), capsule['segments']
assert [g['type'] for g in capsule['segments']] == ['line', 'arc', 'line', 'arc', 'line']
assert all(near(g['radius'], 0.25) for g in capsule['segments'] if g['type'] == 'arc')

# the track on an arc: the circles of radius 0.75 and 1.25 around (5, 2), and the half circles at the ends
arc_track = shapes[1]
check_spline(arc_track)
radii = sorted(round(g['radius'], 6) for g in arc_track['segments'] if g['type'] == 'arc')
assert radii == [0.25, 0.25, 0.75, 1.25], radii
big = [g for g in arc_track['segments'] if g['type'] == 'arc' and near(g['radius'], 1.25)][0]
assert big['cw'] is True and pnear(big['center'], (5.0, 2.0)), big

# the pads
rect = shapes[2]
assert rect['type'] == 'polyline' and pnear(rect['points'][0], (7.0, 7.5)) and pnear(rect['points'][2], (9.0, 8.5)), rect['points']
oblong = shapes[3]
assert oblong['type'] == 'spline'
check_spline(oblong)
assert any(near(g['radius'], 0.5) for g in oblong['segments'] if g['type'] == 'arc')
polygon = shapes[4]
assert polygon['type'] == 'polyline' and len(polygon['points']) == 7 and near(math.dist(polygon['points'][0], (12.0, 8.0)), 1.0), polygon
pad = [s for s in shapes if s['type'] == 'circle' and pnear(s['center'], (14.0, 8.0))]
assert len(pad) == 2 and sorted(c['radius'] for c in pad) == [0.3, 0.75], pad		# the pad and its hole
clear = [s for s in shapes if s['layer'] == 'top_clear']
assert len(clear) == 1 and clear[0]['type'] == 'circle', clear
region = [s for s in shapes if s['type'] == 'polyline' and s['points'][0] == (0.0, 0.0)][0]
assert len(region['points']) == 5 and pnear(region['points'][2], (4.0, 4.0)), region

# the center lines
centers = gerberparse.convert(parsed, False, 'top')
assert centers[0] == {'type': 'line', 'p1': (1.0, 1.0), 'p2': (5.0, 1.0), 'layer': 'top'}, centers[0]
assert centers[1]['type'] == 'arc' and centers[1]['cw'] is False and pnear(centers[1]['center'], (5.0, 2.0)), centers[1]

# inches, numbers in the other format (trailing zeros omitted), single quadrant arcs, an arc with an aperture of a rectangle
inch = gerberparse.parse('%FSTAX23Y23*%\n%MOIN*%\n%ADD10C,0.01*%\nD10*\nX1Y1D02*\nX2Y1D01*\nG74*\nG02*\nX3Y0I0J1D01*\nM02*\n')
assert inch.unit == 'inch'
tracks = [o for o in inch.objects if o['kind'] == 'track']
assert len(tracks) == 2, inch.objects
# "X1" with trailing zeros omitted in the format 2.3 is 100 / 1000 * 100: the digits are padded to 5: 10000 -> 10.000
assert pnear(tracks[0]['from'], (10.0, 10.0)) and pnear(tracks[0]['segment']['to'], (20.0, 10.0)), tracks[0]
# single quadrant: the offsets have no sign, the center is the one which makes an arc of at most 90 degrees: from (20, 10) to (30, 0), clockwise, offset (0, 10)
seg = tracks[1]['segment']
assert seg['type'] == 'arc' and seg['cw'] is True and pnear(seg['center'], (20.0, 0.0)), seg
in_mm = gerberparse.convert(inch, False, 'x')
assert near(in_mm[0]['p2'][0] - in_mm[0]['p1'][0], 254.0), in_mm[0]

# a macro which is not defined is reported, a track without aperture too
warn = gerberparse.parse("""%FSLAX24Y24*%
%MOMM*%
%ADD10NOPE,1*%
D10*
X0Y0D03*
D11*
X0Y0D01*
M02*
""")
assert len(warn.warnings) == 3 and not warn.objects, warn.warnings

# macros: variables and arithmetic ($3 = $1 x 2), a circle and a center line, with a rotation
macro = gerberparse.parse("""%FSLAX24Y24*%
%MOMM*%
%AMBAR*0 a comment*
$3=$1x2*
1,1,$3,0,0*
21,1,$1,$2/2,1,0,45*%
%ADD10BAR,1X4*%
D10*
X100000Y200000D03*
M02*
""")
assert not macro.warnings, macro.warnings
made = gerberparse.convert(macro)
assert made[0]['type'] == 'circle' and pnear(made[0]['center'], (10.0, 20.0)) and near(made[0]['radius'], 1.0), made[0]
# the center line: 1 x 2 (width $1 = 1, height $2/2 = 2) centered on (1, 0), turned by 45 degrees around the origin
bar = made[1]
assert bar['type'] == 'polyline' and len(bar['points']) == 5, bar
cx = sum(p[0] for p in bar['points'][:4]) / 4.0 - 10.0
cy = sum(p[1] for p in bar['points'][:4]) / 4.0 - 20.0
assert near(cx, math.cos(math.radians(45)), 1e-6) and near(cy, math.sin(math.radians(45)), 1e-6), (cx, cy)
assert near(math.dist(bar['points'][0], bar['points'][1]), 1.0, 1e-6) and near(math.dist(bar['points'][1], bar['points'][2]), 2.0, 1e-6), bar

# the RoundRect macro of KiCad: one rounded rectangle, 2 x 1.3 with a radius of 0.25
kicad = """%FSLAX46Y46*%
%MOMM*%
%AMRoundRect*
0 Rectangle with rounded corners*
0 $1 Rounding radius*
4,1,4,$2,$3,$4,$5,$6,$7,$8,$9,$2,$3,0*
1,1,$1+$1,$2,$3*
1,1,$1+$1,$4,$5*
1,1,$1+$1,$6,$7*
1,1,$1+$1,$8,$9*
20,1,$1+$1,$2,$3,$4,$5,0*
20,1,$1+$1,$4,$5,$6,$7,0*
20,1,$1+$1,$6,$7,$8,$9,0*
20,1,$1+$1,$8,$9,$2,$3,0*%
%ADD10RoundRect,0.250000X-1.000000X-0.650000X1.000000X-0.650000X1.000000X0.650000X-1.000000X0.650000X0*%
D10*
X10000000Y5000000D03*
M02*
"""
parsed_kicad = gerberparse.parse(kicad)
assert not parsed_kicad.warnings, parsed_kicad.warnings
pad = gerberparse.convert(parsed_kicad)
assert len(pad) == 1 and pad[0]['type'] == 'spline', pad
check_spline(pad[0])
arcs = [g for g in pad[0]['segments'] if g['type'] == 'arc']
assert len(arcs) == 4 and all(near(g['radius'], 0.25) and g['cw'] is False for g in arcs), arcs
xs = [g['point'][0] for g in pad[0]['segments']]
ys = [g['point'][1] for g in pad[0]['segments']]
# the pad is at (10, 5): 2 + 0.5 wide, 1.3 + 0.5 high
assert near(min(xs), 10 - 1.25) and near(max(xs), 10 + 1.25) and near(min(ys), 5 - 0.9) and near(max(ys), 5 + 0.9), (min(xs), max(xs), min(ys), max(ys))

# a rectangular aperture on a track: the hull of the two positions
rect_track = gerberparse.parse('%FSLAX24Y24*%\n%MOMM*%\n%ADD10R,1X0.5*%\nD10*\nX0Y0D02*\nX30000Y0D01*\nM02*\n')
hull = gerberparse.convert(rect_track)[0]
xs = [p[0] for p in hull['points']]
ys = [p[1] for p in hull['points']]
assert near(min(xs), -0.5) and near(max(xs), 3.5) and near(min(ys), -0.25) and near(max(ys), 0.25), hull

# ---- Excellon
EXCELLON = '''M48
; comment
METRIC,TZ
T01C0.8
T02C1.0
%
T01
X10500Y-20300
X30000Y40000
T02
X5000Y5000G85X15000Y5000
M30
'''
drill = gerberparse.parse(EXCELLON)
assert drill.kind == 'excellon' and drill.unit == 'mm', (drill.kind, drill.unit)
assert [o['kind'] for o in drill.objects] == ['hole', 'hole', 'slot'], drill.objects
assert pnear(drill.objects[0]['at'], (10.5, -20.3)) and near(drill.objects[0]['d'], 0.8), drill.objects[0]
shapes = gerberparse.convert(drill, True, 'drill')
assert shapes[0] == {'type': 'circle', 'center': (10.5, -20.3), 'radius': 0.4, 'layer': 'drill'}, shapes[0]
assert shapes[2]['type'] == 'spline'
check_spline(shapes[2])
# explicit decimal points and inches
inch_drill = gerberparse.parse('M48\nINCH\nT1C0.0315\n%\nT1\nX1.5Y2.25\nM30\n')
assert inch_drill.unit == 'inch' and pnear(inch_drill.objects[0]['at'], (1.5, 2.25)), inch_drill.objects
assert near(gerberparse.convert(inch_drill)[0]['center'][0], 38.1), gerberparse.convert(inch_drill)[0]
# leading zeros (trailing zeros omitted): X0105 in the format 3.3 is 010.500
lz = gerberparse.parse('M48\nMETRIC,LZ\nT1C1\n%\nT1\nX0105Y002\nM30\n')
assert pnear(lz.objects[0]['at'], (10.5, 2.0)), lz.objects[0]


# ---- the script
class Core:
    mouse_x = 100.0
    mouse_y = 200.0
    unit = 'mm'

    def __init__(self):
        self.calls = []
        self.logs = []

    def __getattr__(self, name):
        return lambda *a: self.calls.append((name,) + a)

    def log(self, text):
        self.logs.append(text)


folder = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(folder, 'top copper.gbr')
try:
    with open(path, 'w') as f:
        f.write('%FSLAX24Y24*%\n%MOMM*%\n%ADD10C,0.5*%\nD10*\nX10000Y10000D02*\nX50000Y10000D01*\nX60000Y60000D03*\nM02*\n')
    script = gerber_import.Cadscript()
    script.set_language('en-us')
    script.set_property(0, path)
    script.set_property(1, False)		# center lines
    core = Core()
    script.set_core(core)
    script.run()
    names = [c[0] for c in core.calls]
    assert names == ['set_layer', 'line', 'circle', 'set_layer'], names
    assert core.calls[0][1] == 'top_copper', core.calls[0]
    assert core.calls[1][1:] == (101.0, 201.0, 105.0, 201.0), core.calls[1]
    assert core.calls[2][1:] == (106.0, 206.0, 0.25), core.calls[2]
    assert core.logs == ['Gerber imported: 2 shapes.'], core.logs

    # in inches
    core = Core()
    core.unit = 'inch'
    script.set_core(core)
    script.run()
    assert near(core.calls[1][3] - core.calls[1][1], 4.0 / 25.4, 1e-6), core.calls[1]
finally:
    if os.path.exists(path):
        os.remove(path)
print('ok')
