# Generates the icon (icon_16..256.png, openpostpro.ico) and the splash image (splash.png) of OpenPostPro, in the folder given as argument:
#   python tools/make_icons.py openpostpro/textures
# Concentric toolpaths of a pocket around the tool. Only the standard library is used.
import math, struct, zlib, sys, os

def png(path_or_none, w, h, rgba):
    raw = b''.join(b'\x00' + bytes(rgba[y*w*4:(y+1)*w*4]) for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    if path_or_none:
        open(path_or_none, 'wb').write(data)
    return data

def sd_round_rect(px, py, hx, hy, r):
    qx, qy = abs(px) - hx + r, abs(py) - hy + r
    return math.hypot(max(qx, 0), max(qy, 0)) + min(max(qx, qy), 0) - r

BG = (28, 36, 51)
ORANGE = (255, 154, 31)
YELLOW = (255, 214, 64)

def logo_pixel(u, v):
    # u, v in [-1, 1]; returns (r, g, b, a) or None
    d = sd_round_rect(u, v, 0.96, 0.96, 0.34)
    if d > 0:
        return None
    col = BG
    # the rings of the toolpath
    for i, half in enumerate((0.74, 0.55, 0.36)):
        ring = abs(sd_round_rect(u, v, half, half * 0.78, half * 0.42)) - 0.035
        if ring < 0:
            t = i / 2.0
            col = tuple(int(ORANGE[k] * (1 - t) + YELLOW[k] * t) for k in range(3))
    # the tool
    if math.hypot(u, v) < 0.12:
        col = YELLOW
    return col

def render(size, ss=3):
    out = bytearray(size * size * 4)
    n = ss * ss
    for y in range(size):
        for x in range(size):
            r = g = b = a = 0
            for sy in range(ss):
                for sx in range(ss):
                    u = ((x + (sx + 0.5) / ss) / size) * 2 - 1
                    v = ((y + (sy + 0.5) / ss) / size) * 2 - 1
                    p = logo_pixel(u, v)
                    if p:
                        r += p[0]; g += p[1]; b += p[2]; a += 1
            i = (y * size + x) * 4
            if a:
                out[i] = r // a; out[i+1] = g // a; out[i+2] = b // a
            out[i+3] = 255 * a // n
    return out

def shrink(src, size):
    half = size // 2
    out = bytearray(half * half * 4)
    for y in range(half):
        for x in range(half):
            acc = [0, 0, 0, 0]
            wsum = 0
            for dy in (0, 1):
                for dx in (0, 1):
                    i = ((2*y+dy) * size + 2*x + dx) * 4
                    al = src[i+3]
                    acc[0] += src[i] * al; acc[1] += src[i+1] * al; acc[2] += src[i+2] * al; acc[3] += al
                    wsum += al
            o = (y * half + x) * 4
            if wsum:
                out[o] = acc[0] // wsum; out[o+1] = acc[1] // wsum; out[o+2] = acc[2] // wsum
            out[o+3] = acc[3] // 4
    return out

def main(outdir):
    master = render(512, 2)
    sizes = {512: master}
    s, img = 512, master
    while s > 16:
        img = shrink(img, s); s //= 2
        sizes[s] = img
    entries = []
    for s in (16, 32, 64, 128, 256):
        data = png(os.path.join(outdir, 'icon_%d.png' % s), s, s, sizes[s])
        entries.append((s, data))
    # ICO with PNG images
    ico = struct.pack('<HHH', 0, 1, len(entries))
    offset = 6 + 16 * len(entries)
    body = b''
    for s, data in entries:
        ico += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0, 1, 32, len(data), offset + len(body))
        body += data
    open(os.path.join(outdir, 'openpostpro.ico'), 'wb').write(ico + body)

    # splash: dark background with a thin accent line, the logo above the middle
    W, H, L = 560, 320, 160
    logo = render(L, 3)
    bg = bytearray(W * H * 4)
    for y in range(H):
        for x in range(W):
            k = y / H
            c = (20, 25, 35)
            i = (y * W + x) * 4
            bg[i:i+4] = bytes((*c, 255))
    lx, ly = (W - L) // 2, 36
    for y in range(L):
        for x in range(L):
            j = (y * L + x) * 4
            a = logo[j+3]
            if a:
                i = ((ly + y) * W + lx + x) * 4
                for c in range(3):
                    bg[i+c] = (logo[j+c] * a + bg[i+c] * (255 - a)) // 255
    for x in range(60, W - 60):
        i = (H - 70) * W * 4 + x * 4
        bg[i:i+4] = bytes((255, 154, 31, 255))
    png(os.path.join(outdir, 'splash.png'), W, H, bg)

main(sys.argv[1])
