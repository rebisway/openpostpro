# Writes the icon of the groups (block.png, 32 x 32, white on transparent) in the folder given as argument:
#   python tools/make_block_icons.py openpostpro/textures
# It is the usual picture of a group: two shapes inside a dashed frame with a handle on each corner.
import math, os, struct, sys, zlib

W = 32


def png(path, pixels):
    raw = b''.join(b'\x00' + bytes(pixels[y * W * 4:(y + 1) * W * 4]) for y in range(W))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, W, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(data)


px = [0] * (W * W * 4)


def put(x, y):
    if 0 <= x < W and 0 <= y < W:
        i = (y * W + x) * 4
        px[i:i + 4] = [255, 255, 255, 255]


def frame(x0, y0, x1, y1, dash):
    for x in range(x0, x1 + 1):
        if (x // dash) % 2 == 0:
            put(x, y0); put(x, y1)
    for y in range(y0, y1 + 1):
        if (y // dash) % 2 == 0:
            put(x0, y); put(x1, y)


def handle(cx, cy):
    for dx in range(-2, 3):
        for dy in range(-2, 3):
            put(cx + dx, cy + dy)


def rect(x0, y0, x1, y1):
    for x in range(x0, x1 + 1):
        put(x, y0); put(x, y1)
    for y in range(y0, y1 + 1):
        put(x0, y); put(x1, y)


def circle(cx, cy, r):
    for a in range(0, 360):
        put(int(round(cx + r * math.cos(math.radians(a)))), int(round(cy + r * math.sin(math.radians(a)))))


frame(4, 4, 27, 27, 2)
for cx, cy in ((4, 4), (27, 4), (4, 27), (27, 27)):
    handle(cx, cy)
rect(8, 14, 17, 23)
circle(20, 12, 5)
png(os.path.join(sys.argv[1], 'block.png'), px)
