# OpenPostPro - www.rebisway.com - contact@rebisway.com
# Copyright (C) 2024-2026 Thomas Gourgnier
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Geometry helpers for the CAD scripts. This file is a library: it has no class Cadscript,
# so it is not listed as a script. Use it with "import geom".
#
# Points are tuples (x, y). Angles are in degrees, counter-clockwise from the X axis.
# Functions that can fail (parallel lines, no intersection...) return None.

import math

EPS = 0.0001		# distance under which two points are the same


#######################################################
# numbers and angles                                  #
#######################################################

def almost_equal(a, b, eps=EPS):
	return abs(a - b) < eps


# angle in [0, 360[
def normalize_angle(a):
	a = math.fmod(a, 360.0)
	return a + 360.0 if a < 0 else a


#######################################################
# vectors                                             #
#######################################################

def add(a, b):
	return (a[0] + b[0], a[1] + b[1])


def sub(a, b):
	return (a[0] - b[0], a[1] - b[1])


def scale(a, k):
	return (a[0] * k, a[1] * k)


def dot(a, b):
	return a[0] * b[0] + a[1] * b[1]


# z component of the cross product: positive if b is counter-clockwise from a
def cross(a, b):
	return a[0] * b[1] - a[1] * b[0]


def length(a):
	return math.hypot(a[0], a[1])


def distance(a, b):
	return math.hypot(a[0] - b[0], a[1] - b[1])


def same(a, b, eps=EPS):
	return abs(a[0] - b[0]) < eps and abs(a[1] - b[1]) < eps


# unit vector, None for a null vector
def normalize(a):
	l = length(a)
	return None if l < 1e-12 else (a[0] / l, a[1] / l)


def lerp(a, b, t):
	return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


# angle of the vector from a to b
def angle(a, b):
	return normalize_angle(math.degrees(math.atan2(b[1] - a[1], b[0] - a[0])))


# angle between two vectors, in [0, 180]
def angle_between(u, v):
	lu, lv = length(u), length(v)
	if lu < 1e-12 or lv < 1e-12:
		return 0.0
	return math.degrees(math.acos(max(-1.0, min(1.0, dot(u, v) / (lu * lv)))))


def rotate(p, center, a):
	r = math.radians(a)
	c, s = math.cos(r), math.sin(r)
	x, y = p[0] - center[0], p[1] - center[1]
	return (center[0] + x * c - y * s, center[1] + x * s + y * c)


# mirror of p about the line through p1 and p2
def mirror(p, p1, p2):
	q = project_on_line(p, p1, p2)
	return None if q is None else (2 * q[0] - p[0], 2 * q[1] - p[1])


#######################################################
# lines and segments                                  #
#######################################################

# intersection of the infinite lines (a1, a2) and (b1, b2), None if they are parallel
def line_intersection(a1, a2, b1, b2):
	d1, d2 = sub(a2, a1), sub(b2, b1)
	den = cross(d1, d2)
	if abs(den) < 1e-12:
		return None
	t = cross(sub(b1, a1), d2) / den
	return (a1[0] + d1[0] * t, a1[1] + d1[1] * t)


# intersection of the segments (a1, a2) and (b1, b2), None if they do not cross
def segment_intersection(a1, a2, b1, b2):
	d1, d2 = sub(a2, a1), sub(b2, b1)
	den = cross(d1, d2)
	if abs(den) < 1e-12:
		return None
	t = cross(sub(b1, a1), d2) / den
	u = cross(sub(b1, a1), d1) / den
	if t < -1e-9 or t > 1 + 1e-9 or u < -1e-9 or u > 1 + 1e-9:
		return None
	return (a1[0] + d1[0] * t, a1[1] + d1[1] * t)


# closest point to p on the infinite line (p1, p2), None if p1 = p2
def project_on_line(p, p1, p2):
	d = sub(p2, p1)
	l2 = dot(d, d)
	if l2 < 1e-24:
		return None
	t = dot(sub(p, p1), d) / l2
	return (p1[0] + d[0] * t, p1[1] + d[1] * t)


# distance between p and the segment (p1, p2)
def point_segment_distance(p, p1, p2):
	d = sub(p2, p1)
	l2 = dot(d, d)
	if l2 < 1e-24:
		return distance(p, p1)
	t = max(0.0, min(1.0, dot(sub(p, p1), d) / l2))
	return distance(p, (p1[0] + d[0] * t, p1[1] + d[1] * t))


# the line (p1, p2) moved sideways by d: positive to the left of the direction p1 -> p2
def offset_line(p1, p2, d):
	n = normalize(sub(p2, p1))
	if n is None:
		return None
	k = (-n[1] * d, n[0] * d)
	return add(p1, k), add(p2, k)


#######################################################
# circles and arcs                                    #
#######################################################

# point of the circle at the given angle
def arc_point(center, r, a):
	rad = math.radians(a)
	return (center[0] + r * math.cos(rad), center[1] + r * math.sin(rad))


# (center, radius) of the circle through three points, None if they are aligned
def circle_from_3_points(a, b, c):
	d = 2 * (a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]))
	if abs(d) < 1e-12:
		return None
	a2, b2, c2 = dot(a, a), dot(b, b), dot(c, c)
	ux = (a2 * (b[1] - c[1]) + b2 * (c[1] - a[1]) + c2 * (a[1] - b[1])) / d
	uy = (a2 * (c[0] - b[0]) + b2 * (a[0] - c[0]) + c2 * (b[0] - a[0])) / d
	return (ux, uy), distance((ux, uy), a)


# (start angle, end angle, sweep) of the arc from start to end around center.
# The sweep is positive for a counter-clockwise arc and negative for a clockwise one, never 0.
def arc_angles(center, start, end, cw):
	a1 = angle(center, start)
	a2 = angle(center, end)
	sweep = a2 - a1
	if cw:
		sweep = -normalize_angle(-sweep) if abs(normalize_angle(-sweep)) > 1e-9 else -360.0
	else:
		sweep = normalize_angle(sweep) if abs(normalize_angle(sweep)) > 1e-9 else 360.0
	return a1, a2, sweep


# length of the arc from start to end
def arc_length(center, start, end, cw):
	return abs(arc_angles(center, start, end, cw)[2]) * math.pi / 180.0 * distance(center, start)


# intersections of a circle and the infinite line (p1, p2): a list of 0, 1 or 2 points
def circle_line_intersection(center, r, p1, p2):
	q = project_on_line(center, p1, p2)
	if q is None:
		return []
	h = distance(center, q)
	if h > r + EPS:
		return []
	if abs(h - r) <= EPS:
		return [q]
	n = normalize(sub(p2, p1))
	k = math.sqrt(r * r - h * h)
	return [(q[0] - n[0] * k, q[1] - n[1] * k), (q[0] + n[0] * k, q[1] + n[1] * k)]


# intersections of two circles: a list of 0, 1 or 2 points
def circle_circle_intersection(c1, r1, c2, r2):
	d = distance(c1, c2)
	if d < 1e-12 or d > r1 + r2 + EPS or d < abs(r1 - r2) - EPS:
		return []
	a = (r1 * r1 - r2 * r2 + d * d) / (2 * d)
	h2 = r1 * r1 - a * a
	m = lerp(c1, c2, a / d)
	if h2 <= EPS * EPS:
		return [m]
	h = math.sqrt(h2)
	ux, uy = (c2[0] - c1[0]) / d, (c2[1] - c1[1]) / d
	return [(m[0] - uy * h, m[1] + ux * h), (m[0] + uy * h, m[1] - ux * h)]


# tangent points of the lines from p to a circle: a list of 0, 1 or 2 points
def tangent_points(p, center, r):
	d = distance(p, center)
	if d < r - EPS:
		return []
	if d <= r + EPS:
		return [p]
	return circle_circle_intersection(center, r, lerp(p, center, 0.5), d / 2)


#######################################################
# polygons and polylines                              #
#######################################################

def bounding_box(points):
	xs = [p[0] for p in points]
	ys = [p[1] for p in points]
	return (min(xs), min(ys)), (max(xs), max(ys))


# signed area of a polygon: positive if the points go counter-clockwise
def polygon_area(points):
	s = 0.0
	n = len(points)
	for i in range(n):
		a, b = points[i], points[(i + 1) % n]
		s += a[0] * b[1] - b[0] * a[1]
	return s / 2.0


def is_ccw(points):
	return polygon_area(points) > 0


# True if the last point is the first one
def is_closed(points):
	return len(points) > 2 and same(points[0], points[-1])


def polyline_length(points, closed=False):
	n = len(points)
	total = sum(distance(points[i], points[i + 1]) for i in range(n - 1))
	if closed and n > 1:
		total += distance(points[-1], points[0])
	return total


# polyline moved sideways by d (positive to the left of the path), corners kept sharp.
# The points must not repeat the first one for a closed polyline. Consecutive duplicates are ignored.
def offset_polyline(points, d, closed=False):
	pts = [points[0]]
	for p in points[1:]:
		if not same(p, pts[-1]):
			pts.append(p)
	n = len(pts)
	if n < 2:
		return list(pts)

	sides = []
	for i in range(n if closed else n - 1):
		sides.append(offset_line(pts[i], pts[(i + 1) % n], d))

	result = []
	for i in range(n):
		if not closed and i == 0:
			result.append(sides[0][0])
		elif not closed and i == n - 1:
			result.append(sides[-1][1])
		else:
			prev, cur = sides[i - 1], sides[i % len(sides)]
			q = line_intersection(prev[0], prev[1], cur[0], cur[1])
			result.append(q if q is not None else cur[0])		# parallel sides: no corner to compute
	return result
