# Post-processors derived from the generic ones and HPGL, driven through postcore like the application does.
import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'postpro'))
import postcore
import generic_mill, generic_plasma, mach3_mill, mach3_plasma, num1060_mill, num1060_plasma, burny_mill, burny_plasma, hpgl


def make(module):
    post = module.Postpro()
    core = postcore.Postcore()
    core.set_postpro(post)
    post.set_core(core)
    core.set_language('en-us')
    core.set_unit('mm')
    return post, core


def program(module, relative=None):
    post, core = make(module)
    out = core.start_loop() + core.start_program()
    out += core.rapid(['X', 'Y'], [10.0, 5.0])
    out += core.start_single_path()
    out += core.linear(['Z'], [-1.0])
    out += core.linear(['X'], [20.0])
    out += core.clockwise(['X', 'Y', 'I'], [30.0, 5.0, 5.0]) if False else core.clockwise(['X', 'Y', 'I', 'J'], [30.0, 15.0, 5.0, 5.0])
    out += core.pause(1.5)
    out += core.stop_single_path()
    out += core.stop_program() + core.stop_loop()
    return post, out


for module in (generic_mill, generic_plasma, mach3_mill, mach3_plasma, num1060_mill, num1060_plasma, burny_mill, burny_plasma, hpgl):
    post, out = program(module)
    assert out.strip(), module.__name__
    assert post.get_description() and post.get_category() in ('MILL', 'PLASMA', 'OTHER')

# Mach3: tap file, absolute arc centers announced with G90.1
post, out = program(mach3_mill)
assert post.get_extention() == 'tap' and 'G90.1' in out and not post.ijk_relative(), out
post, out = program(mach3_plasma)
assert post.get_extention() == 'tap' and 'G90.1' in out, out

# Num 1060: spaces, no G64, dwell with F, X Y Z always passed
post, out = program(num1060_mill)
assert 'G64' not in out and 'G4 F1.5' in out and 'G1 Z-1' in out and 'G2 X30' in out, out
post, out = program(num1060_plasma)
assert post.plasma_stop == 'M0' and 'M0' in out and 'G64' not in out, out

# Burny: dwell with F, relative centers; the plasma gives X Y I J on every arc
post, out = program(burny_mill)
assert 'G4F1.5' in out.replace(' ', '') and post.ijk_relative(), out
post, core = make(burny_plasma)
core.start_loop()
core.rapid(['X', 'Y'], [10.0, 5.0])
text = core.clockwise(['X', 'I'], [30.0, 5.0])		# Y and J not given
assert text.strip() == 'G2X30Y5I5J0', text
text = core.clockwise(['Y', 'J'], [15.0, 5.0])
assert text.strip() == 'X30Y15I0J5', text			# G2 is not repeated in condensed mode

# the generic ones are unchanged
post, out = program(generic_mill)
assert 'G4P1.5' in out.replace(' ', '') and 'G64' in out, out

# HPGL: IN; at the start, pen up / down, arcs cut into segments of the right radius
post, core = make(hpgl)
out = core.start_loop() + core.start_program()
out += core.rapid(['X', 'Y'], [10.0, 0.0])
out += core.linear(['X', 'Y'], [20.0, 0.0])
assert out.startswith('IN;') and 'PU 400,0;' in out and 'PD 800,0;' in out, out
arc = core.counter_clockwise(['X', 'Y', 'I', 'J'], [0.0, 0.0, -10.0, 0.0])		# half turn around (10, 0), from (20, 0) to (0, 0)
points = [tuple(int(v) for v in l[3:-1].split(',')) for l in arc.split('\n') if l.startswith('PD ')]
assert len(points) > 20, len(points)
assert all(abs(math.hypot(x / 40 - 10, y / 40) - 10) < 0.02 for x, y in points), points[:3]
assert points[-1] == (0, 0)
assert max(y for x, y in points) > 300 and min(y for x, y in points) >= 0, 'counter clockwise from (20, 0) goes through positive y'
# clockwise goes the other way round
post, core = make(hpgl)
core.start_loop()
core.rapid(['X', 'Y'], [10.0, 0.0])
cw = core.clockwise(['X', 'Y', 'I', 'J'], [0.0, 10.0, -10.0, 0.0])		# clockwise from (10, 0) to (0, 10) around the origin: three quarters of a turn, through the bottom
ys = [int(l[3:-1].split(',')[1]) for l in cw.split('\n') if l.startswith('PD ')]
assert min(ys) < 0, 'clockwise from (10,0) must go through negative y'
# inch: 1016 units per inch
post, core = make(hpgl)
core.set_unit('inch')
core.start_loop()
assert core.rapid(['X', 'Y'], [1.0, 0.0]).strip() == 'PU 1016,0;'
# tool = pen
assert make(hpgl)[0].tool(3) == 'SP3;'
print('ok')

# the dialog box asks the core: the header and the footer of the file are edited on several lines, the others are not
for module in (generic_mill, mach3_mill, generic_plasma, mach3_plasma):
    post, core = make(module)
    keys = [post.get_property_key(i) for i in range(post.get_properties_count())]
    multi = [keys[i] for i in range(len(keys)) if core.get_property_multiline(i)]
    assert 'pre_data' in multi and 'post_data' in multi, (module.__name__, multi)
    assert 'ext' not in multi and 'cond' not in multi, (module.__name__, multi)
print('multiline properties: ok')

# return to zero: the tool goes back to X0 Y0 at the safe height at the end of the program, only when it is asked
for module in (generic_mill, mach3_mill, generic_plasma, burny_plasma):
    post, core = make(module)
    keys = [post.get_property_key(i) for i in range(post.get_properties_count())]
    assert 'return_zero' in keys, module.__name__
    core.rapid(['X', 'Y'], [50.0, 20.0])
    without = core.stop_program()
    post, core = make(module)
    post.set_property(keys.index('return_zero'), True)
    core.rapid(['X', 'Y'], [50.0, 20.0])
    with_zero = core.stop_program()
    assert 'X0' in with_zero and 'Y0' in with_zero and 'X0' not in without, (module.__name__, without, with_zero)
    assert with_zero.index('Z') < with_zero.index('X0'), module.__name__
print('return to zero: ok')
