import math
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'openpostpro', 'script'))
import geom as g
def eq(a, b): return abs(a-b) < 1e-9
def peq(p, q): return eq(p[0], q[0]) and eq(p[1], q[1])
assert eq(g.normalize_angle(-90), 270) and eq(g.angle((0,0),(0,1)), 90)
assert peq(g.rotate((1,0),(0,0),90), (0,1))
assert peq(g.mirror((1,2),(0,0),(1,0)), (1,-2))
assert peq(g.line_intersection((0,0),(2,2),(0,2),(2,0)), (1,1))
assert g.line_intersection((0,0),(1,0),(0,1),(1,1)) is None
assert g.segment_intersection((0,0),(1,1),(2,0),(3,-1)) is None
assert peq(g.segment_intersection((0,0),(2,2),(0,2),(2,0)), (1,1))
assert eq(g.point_segment_distance((5,3),(0,0),(2,0)), math.hypot(3,3))
l = g.offset_line((0,0),(10,0),2); assert peq(l[0],(0,2)) and peq(l[1],(10,2))
c, r = g.circle_from_3_points((1,0),(0,1),(-1,0)); assert peq(c,(0,0)) and eq(r,1)
assert g.circle_from_3_points((0,0),(1,1),(2,2)) is None
a1,a2,sw = g.arc_angles((0,0),(1,0),(0,1),False); assert eq(sw,90)
a1,a2,sw = g.arc_angles((0,0),(1,0),(0,1),True); assert eq(sw,-270)
assert eq(g.arc_length((0,0),(1,0),(0,1),False), math.pi/2)
pts = g.circle_line_intersection((0,0),5,(-9,3),(9,3)); assert len(pts)==2 and all(eq(math.hypot(*p),5) for p in pts)
assert len(g.circle_line_intersection((0,0),5,(-9,6),(9,6)))==0
assert len(g.circle_line_intersection((0,0),5,(-9,5),(9,5)))==1
pts = g.circle_circle_intersection((0,0),5,(6,0),5); assert len(pts)==2 and all(eq(math.hypot(*p),5) and eq(math.hypot(p[0]-6,p[1]),5) for p in pts)
assert len(g.circle_circle_intersection((0,0),1,(5,0),1))==0
t = g.tangent_points((10,0),(0,0),5); assert len(t)==2 and all(eq(g.dot(g.sub(p,(0,0)),g.sub(p,(10,0))),0) for p in t)
rect=[(0,0),(10,0),(10,5),(0,5)]
assert eq(g.polygon_area(rect),50) and g.is_ccw(rect) and not g.is_ccw(rect[::-1])
assert g.bounding_box(rect)==((0,0),(10,5)) and eq(g.polyline_length(rect,True),30)
o = g.offset_polyline(rect, 1, True); assert all(peq(p,q) for p,q in zip(o,[(1,1),(9,1),(9,4),(1,4)])), o
o = g.offset_polyline([(0,0),(10,0),(10,5)], 1); assert all(peq(p,q) for p,q in zip(o,[(0,1),(9,1),(9,5)])), o
print('GEOM OK')
