# Writes a sample document of loose parts for the nesting tests, in the file given as argument:
#   python tools/make_nest_parts.py tests/cpp/data/nest_parts.opp
# The parts imitate a real cutting sheet: big plates with holes, rounded rectangles with slots, rings, triangles,
# brackets and washers, so that small parts can be nested inside the holes of the big ones.
# Only lines, arcs and circles are used. Units: millimetre. One layer per part family, one group (block) per part.
import math, sys

shapes = []
next_id = [10]
layers = {}
block = ['']
counters = {}


# a new part: its shapes form a group (a block), named after the family of the part and a number
def new_part(family):
    counters[family] = counters.get(family, 0) + 1
    block[0] = '%s_%d' % (family, counters[family])


def add(kind, layer, name, *values):
    shapes.append('%d;%d;%s;%s_%d;%s;%s;' % (kind, next_id[0], layer, name, next_id[0], ';'.join(('%g' % v) for v in values), block[0]))
    next_id[0] += 1


def line(layer, x1, y1, x2, y2):
    add(103, layer, 'Ligne', x1, y1, x2, y2)


def circle(layer, cx, cy, r):
    add(100, layer, 'Cercle', cx, cy, r)


# arc from (x1, y1) to (x2, y2) around (cx, cy), cw = 1 when clockwise
def arc(layer, x1, y1, cx, cy, x2, y2, cw):
    add(102, layer, 'Arc', x1, y1, cx, cy, x2, y2, cw)


# rectangle with rounded corners, lower left corner at (x, y)
def round_rect(layer, x, y, w, h, r):
    line(layer, x + r, y, x + w - r, y)
    arc(layer, x + w - r, y, x + w - r, y + r, x + w, y + r, 0)
    line(layer, x + w, y + r, x + w, y + h - r)
    arc(layer, x + w, y + h - r, x + w - r, y + h - r, x + w - r, y + h, 0)
    line(layer, x + w - r, y + h, x + r, y + h)
    arc(layer, x + r, y + h, x + r, y + h - r, x, y + h - r, 0)
    line(layer, x, y + h - r, x, y + r)
    arc(layer, x, y + r, x + r, y + r, x + r, y, 0)


# slot (oblong hole) centred at (cx, cy), length l along X, width w
def slot(layer, cx, cy, l, w):
    r = w / 2
    a, b = cx - l / 2 + r, cx + l / 2 - r
    line(layer, a, cy - r, b, cy - r)
    arc(layer, b, cy - r, b, cy, b, cy + r, 0)
    line(layer, b, cy + r, a, cy + r)
    arc(layer, a, cy + r, a, cy, a, cy - r, 0)


def polygon(layer, points):
    new_part(layer)
    for i in range(len(points)):
        x1, y1 = points[i]
        x2, y2 = points[(i + 1) % len(points)]
        line(layer, x1, y1, x2, y2)


def plate_with_slots(layer, x, y, w, h, r, slots, holes):
    new_part(layer)
    round_rect(layer, x, y, w, h, r)
    for (sx, sy, sl, sw) in slots:
        slot(layer, x + sx, y + sy, sl, sw)
    for (hx, hy, hr) in holes:
        circle(layer, x + hx, y + hy, hr)


def ring(layer, cx, cy, ro, ri):
    new_part(layer)
    circle(layer, cx, cy, ro)
    circle(layer, cx, cy, ri)


def bracket(layer, x, y, a, b, t):
    polygon(layer, [(x, y), (x + a, y), (x + a, y + t), (x + t, y + t), (x + t, y + b), (x, y + b)])


def washer_family(layer, x, y, count, ro, ri, step):
    for i in range(count):
        ring(layer, x + i * step, y, ro, ri)


# big frame: a plate with a large rectangular window, where other parts can go
def frame(layer, x, y, w, h, r, window):
    new_part(layer)
    round_rect(layer, x, y, w, h, r)
    wx, wy, ww, wh = window
    round_rect(layer, x + wx, y + wy, ww, wh, 6)


def build():
    L = 'Plaques'
    frame(L, 0, 0, 260, 400, 12, (30, 60, 200, 300))
    frame(L, 280, 0, 260, 400, 12, (30, 60, 200, 300))
    ring('Bagues', 640, 80, 70, 52)
    ring('Bagues', 640, 80, 38, 26)
    ring('Bagues', 800, 80, 70, 52)
    ring('Bagues', 800, 80, 38, 26)
    plate_with_slots('Brides', 580, 180, 220, 70, 20, [(40, 35, 30, 14), (110, 35, 30, 14), (180, 35, 30, 14)], [(110, 35, 0)])
    plate_with_slots('Brides', 580, 270, 160, 120, 14, [(80, 30, 90, 16), (80, 90, 90, 16)], [(20, 60, 6), (140, 60, 6)])
    for i in range(3):
        bracket('Equerres', 830 + i * 70, 180, 60, 90, 18)
    for i in range(5):
        polygon('Triangles', [(830 + i * 60, 300), (880 + i * 60, 300), (830 + i * 60, 380)])
    washer_family('Rondelles', 580, 420, 8, 22, 12, 52)
    washer_family('Rondelles', 580, 480, 8, 14, 7, 36)
    plate_with_slots('Oblongs', 0, 420, 140, 40, 10, [(30, 20, 40, 14), (110, 20, 40, 14)], [])
    plate_with_slots('Oblongs', 160, 420, 140, 40, 10, [(30, 20, 40, 14), (110, 20, 40, 14)], [])
    for i in range(6):
        new_part('Disques')
        circle('Disques', 40 + i * 70, 520, 25)


build()
palette = {'Plaques': '(0.4:0.6:1:1)', 'Bagues': '(0.7:0.9:0.5:1)', 'Brides': '(0.7:0.5:0.9:1)', 'Equerres': '(1:1:0.6:1)',
           'Triangles': '(0.5:0.9:0.5:1)', 'Rondelles': '(0.9:0.4:0.9:1)', 'Oblongs': '(0.6:0.8:1:1)', 'Disques': '(1:0.7:0.2:1)'}
head = ['{OPENPOSTPRO}', '0;version;0.1', '0;output;nest.nc', '0;unit;0', '{LAYERS}']
for i, (name, color) in enumerate(palette.items()):
    head.append('1;%d;%s;%s;1;0' % (i + 1, name, color))
head.append('{GROUPS}')
head.append('{SHAPES}')
with open(sys.argv[1], 'w', encoding='utf-8', newline='\n') as f:
    f.write('\n'.join(head + shapes) + '\n')
print(len(shapes), 'shapes written to', sys.argv[1])
