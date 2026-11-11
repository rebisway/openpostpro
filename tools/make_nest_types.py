# Writes a sample document with one part of each type of a real nesting sheet, in the file given as argument:
#   python tools/make_nest_types.py tests/cpp/data/nest_types.opp
# A second argument, fr or en, names the layers and the groups in that language and leaves the texts out (the examples of the manual):
#   python tools/make_nest_types.py docs/examples/nesting.en.opp en
# The types: a plate with a label, a ring with a slit and a label, a disc with six holes, a ring sector with a row of holes,
# a fork and a slot with a hole at one end. One layer and one group (block) per part. Lines, arcs, circles and texts. Units: millimetre.
import math, sys

NAMES = {
    'fr': {'PL-01': 'Plaque', 'R-2': 'Anneau', 'Disque': 'Disque', 'Arc': 'Arc', 'Fourche': 'Fourche', 'Oblong': 'Oblong'},
    'en': {'PL-01': 'Plate', 'R-2': 'Ring', 'Disque': 'Disc', 'Arc': 'Arc', 'Fourche': 'Fork', 'Oblong': 'Slot'},
}
language = sys.argv[2] if len(sys.argv) > 2 else ''


SHAPES = {'en': ('Line', 'Arc', 'Circle', 'Text')}.get(language, ('Ligne', 'Arc', 'Cercle', 'Texte'))


def named(name):
    return NAMES[language][name] if language else name


LAYERS = [
    ('PL-01', '(0.3:0.75:0.85:1)'), ('R-2', '(1:0.9:0.45:1)'), ('Disque', '(0.93:0.4:0.7:1)'),
    ('Arc', '(0.45:0.7:0.95:1)'), ('Fourche', '(0.55:0.75:0.4:1)'), ('Oblong', '(1:0.7:0.3:1)'),
]


# a part is a list of shapes, in its own coordinates: ('line', x1, y1, x2, y2), ('arc', x1, y1, cx, cy, x2, y2, cw), ('circle', cx, cy, r),
# ('text', x, y, text, height). cw is 1 for a clockwise arc.
def polar(r, degrees):
    a = math.radians(degrees)
    return r * math.cos(a), r * math.sin(a)


def arc_points(shape, steps=48):
    _, x1, y1, cx, cy, x2, y2, cw = shape
    a1, a2 = math.atan2(y1 - cy, x1 - cx), math.atan2(y2 - cy, x2 - cx)
    sweep = (a1 - a2) % (2 * math.pi) * -1 if cw else (a2 - a1) % (2 * math.pi)
    r = math.hypot(x1 - cx, y1 - cy)
    return [(cx + r * math.cos(a1 + sweep * i / steps), cy + r * math.sin(a1 + sweep * i / steps)) for i in range(steps + 1)]


def bounds(shapes):
    points = []
    for s in shapes:
        if s[0] == 'line':
            points += [(s[1], s[2]), (s[3], s[4])]
        elif s[0] == 'arc':
            points += arc_points(s)
        elif s[0] == 'circle':
            points += [(s[1] - s[3], s[2] - s[3]), (s[1] + s[3], s[2] + s[3])]
    xs, ys = [p[0] for p in points], [p[1] for p in points]
    return min(xs), min(ys), max(xs), max(ys)


def rounded_rectangle(w, h, r):
    return [('line', r, 0, w - r, 0), ('arc', w - r, 0, w - r, r, w, r, 0), ('line', w, r, w, h - r), ('arc', w, h - r, w - r, h - r, w - r, h, 0),
            ('line', w - r, h, r, h), ('arc', r, h, r, h - r, 0, h - r, 0), ('line', 0, h - r, 0, r), ('arc', 0, r, r, r, r, 0, 0)]


def plate():
    return rounded_rectangle(87, 122, 4) + [('text', 52, 6, 'PL-01', 6)]


# a ring with a radial slit along +x: the outline goes round the outer circle, down the slit, back round the inner circle
def split_ring():
    big, small, half = 67.0, 44.0, 6.0
    to, ti = math.asin(half / big), math.asin(half / small)
    o1, o2 = (big * math.cos(to), big * math.sin(to)), (big * math.cos(to), -big * math.sin(to))
    i1, i2 = (small * math.cos(ti), small * math.sin(ti)), (small * math.cos(ti), -small * math.sin(ti))
    return [('arc', o1[0], o1[1], 0, 0, o2[0], o2[1], 0), ('line', o2[0], o2[1], i2[0], i2[1]),
            ('arc', i2[0], i2[1], 0, 0, i1[0], i1[1], 1), ('line', i1[0], i1[1], o1[0], o1[1]), ('text', -8, -57, 'R-2', 6)]


def disc():
    shapes = [('circle', 0, 0, 24), ('circle', 0, 0, 2.5)]
    for k in range(5):
        x, y = polar(13, 90 + 72 * k)
        shapes.append(('circle', x, y, 4.5))
    return shapes


# a sector of a ring, 140 degrees, with a row of small holes along its middle
def ring_sector():
    big, small, a1, a2 = 95.0, 75.0, 20.0, 160.0
    o1, o2, i1, i2 = polar(big, a1), polar(big, a2), polar(small, a1), polar(small, a2)
    shapes = [('arc', o1[0], o1[1], 0, 0, o2[0], o2[1], 0), ('line', o2[0], o2[1], i2[0], i2[1]),
              ('arc', i2[0], i2[1], 0, 0, i1[0], i1[1], 1), ('line', i1[0], i1[1], o1[0], o1[1])]
    for k in range(17):
        x, y = polar(85, a1 + (a2 - a1) * (k + 1) / 18)
        shapes.append(('circle', x, y, 2.2))
    return shapes


# a rod with a forked head
def fork():
    points = [(0, -4.5), (48, -4.5), (48, -12), (70, -12), (70, -4), (58, -4), (58, 4), (70, 4), (70, 12), (48, 12), (48, 4.5), (0, 4.5)]
    return [('line', points[i][0], points[i][1], points[(i + 1) % len(points)][0], points[(i + 1) % len(points)][1]) for i in range(len(points))]


# a slot with a hole in the middle of one of its ends
def slot():
    return [('line', 10, 0, 50, 0), ('arc', 50, 0, 50, 10, 50, 20, 0), ('line', 50, 20, 10, 20), ('arc', 10, 20, 10, 10, 10, 0, 0), ('circle', 10, 10, 4.5)]


PARTS = [('PL-01', plate), ('R-2', split_ring), ('Arc', ring_sector), ('Disque', disc), ('Fourche', fork), ('Oblong', slot)]
ROWS = [['PL-01', 'R-2', 'Arc'], ['Disque', 'Fourche', 'Oblong']]
GAP = 25.0


def number(v):
    return ('%.6f' % v).rstrip('0').rstrip('.') if v else '0'


lines = []
next_id = [10]


def emit(layer, block, shape, dx, dy):
    k = shape[0]
    i = next_id[0]
    next_id[0] += 1
    n = number
    if k == 'line':
        lines.append(';'.join(['103', str(i), layer, SHAPES[0] + '_%d' % i, n(shape[1] + dx), n(shape[2] + dy), n(shape[3] + dx), n(shape[4] + dy), block, '']))
    elif k == 'arc':
        lines.append(';'.join(['102', str(i), layer, SHAPES[1] + '_%d' % i, n(shape[1] + dx), n(shape[2] + dy), n(shape[3] + dx), n(shape[4] + dy),
                               n(shape[5] + dx), n(shape[6] + dy), str(shape[7]), block, '']))
    elif k == 'circle':
        lines.append(';'.join(['100', str(i), layer, SHAPES[2] + '_%d' % i, n(shape[1] + dx), n(shape[2] + dy), n(shape[3]), block, '']))
    elif k == 'text':
        lines.append(';'.join(['106', str(i), layer, SHAPES[3] + '_%d' % i, 'arial', shape[3], '0', '1', '%.6f' % shape[4], n(shape[1] + dx), n(shape[2] + dy),
                               '-1', '0', '0', '1', '0', '0', '0', '1', block, '']))


builders = dict(PARTS)
y = 10.0
for row in ROWS:
    x = 10.0
    row_height = 0.0
    for name in row:
        shapes = builders[name]()
        x0, y0, x1, y1 = bounds(shapes)
        for s in shapes:
            if language and s[0] == 'text':
                continue
            emit(named(name), named(name), s, x - x0, y - y0)
        x += x1 - x0 + GAP
        row_height = max(row_height, y1 - y0)
    y += row_height + GAP

head = ['{OPENPOSTPRO}', '0;version;0.1', '0;output;nest.nc', '0;unit;0', '{LAYERS}']
for i, (name, color) in enumerate(LAYERS):
    head.append('1;%d;%s;%s;1;0' % (i + 1, named(name), color))
head += ['{GROUPS}', '{SHAPES}']
with open(sys.argv[1], 'w', encoding='utf-8', newline='\n') as f:
    f.write('\n'.join(head + lines) + '\n')
print(len(lines), 'shapes written to', sys.argv[1])
