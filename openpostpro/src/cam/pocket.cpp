// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <parallel.h>
#include "pocket.h"
#include <imgui.h>
#include <lang.h>
#include "strings.h"
#include <algorithm>
#include <chrono>
#include <limits>

// A boolean operation only cuts the curves where they cross and joins the pieces: it never creates anything outside the two curves, and
// it can not have many more segments than the two curves together. A result which does (touching offsets, rounding errors) is a runaway :
// it is dropped and the two curves stay as they are. This does not depend on the size of the curves, a text offset by the radius of the
// tool has thousands of segments and has to be merged.
static bool merge_plausible(Curve& a, Curve& b, std::vector<Curve>& result)
{
	geometry::rectangle& ba = a.bounds();
	geometry::rectangle& bb = b.bounds();
	const float margin = geometry::ERR_FLOAT2;
	const float min_x = glm::min(glm::min(ba.top_left.x, ba.bottom_right.x), glm::min(bb.top_left.x, bb.bottom_right.x)) - margin;
	const float max_x = glm::max(glm::max(ba.top_left.x, ba.bottom_right.x), glm::max(bb.top_left.x, bb.bottom_right.x)) + margin;
	const float min_y = glm::min(glm::min(ba.top_left.y, ba.bottom_right.y), glm::min(bb.top_left.y, bb.bottom_right.y)) - margin;
	const float max_y = glm::max(glm::max(ba.top_left.y, ba.bottom_right.y), glm::max(bb.top_left.y, bb.bottom_right.y)) + margin;

	size_t size = 0;
	for (Curve& c : result)
	{
		geometry::rectangle& r = c.bounds();
		if (glm::min(r.top_left.x, r.bottom_right.x) < min_x || glm::max(r.top_left.x, r.bottom_right.x) > max_x ||
			glm::min(r.top_left.y, r.bottom_right.y) < min_y || glm::max(r.top_left.y, r.bottom_right.y) > max_y)
			return false;
		size += c.size();
	}
	return size <= 2 * ((size_t)a.size() + (size_t)b.size()) + 64;
}

void Pocket::mode(PocketMode value)
{
	if (value != _mode)
	{
		_mode = value;
		compute();
	}
}

void Pocket::outward(bool value)
{
	if (value != _outward)
	{
		_outward = value;
		compute();
	}
}

void Pocket::stepover(float value)
{
	if (value != _stepover)
	{
		_stepover = value;
		compute();
	}
}

void Pocket::contours(bool value)
{
	if (value != _contours)
	{
		_contours = value;
		compute();
	}
}

void Pocket::angle(float value)
{
	if (value != _angle)
	{
		_angle = value;
		compute();
	}
}

void Pocket::finishing(float value)
{
	if (value != _finishing)
	{
		_finishing = value;
		compute();
	}
}

namespace
{
	// crossing of a scan line with a curve
	struct ZigzagCrossing
	{
		glm::vec2 point;
		int curve = 0;			// index of the crossed curve
		int segment = 0;		// index of the crossed segment
		float position = 0;		// distance from the start of the segment, to sort the crossings along the curve
		int row = 0;
		int interval = -1;		// the interval of the row which has this crossing as one of its ends
		int rank = 0;			// index among the crossings of the curve, sorted along it
	};

	// part of a scan line which is inside the material
	struct ZigzagInterval
	{
		int left = -1;			// index of the crossing on the left
		int right = -1;			// index of the crossing on the right
		bool used = false;
	};

	// angle swept from the start of an arc to a point, following the direction of the arc
	float zigzag_sweep(Segment& s, glm::vec2 p)
	{
		float a0 = std::atan2(s.point.y - s.center.y, s.point.x - s.center.x);
		float a1 = std::atan2(p.y - s.center.y, p.x - s.center.x);
		float a = s.cw ? a0 - a1 : a1 - a0;
		while (a < 0)
			a += glm::two_pi<float>();
		while (a >= glm::two_pi<float>())
			a -= glm::two_pi<float>();
		return a;
	}

	// a scan line which goes through a vertex, or which is tangent to an arc, gives crossings which can not be counted safely
	bool zigzag_clear(std::vector<Curve>& curves, float y)
	{
		for (Curve& c : curves)
			for (Segment& s : c)
			{
				if (glm::abs(s.point.y - y) < geometry::ERR_FLOAT3)
					return false;
				if (s.type != SegmentType::Line && (glm::abs(s.center.y - s.radius - y) < geometry::ERR_FLOAT3 || glm::abs(s.center.y + s.radius - y) < geometry::ERR_FLOAT3))
					return false;
			}
		return true;
	}

	void zigzag_cross(std::vector<ZigzagCrossing>& crossings, Curve& c, int curve, float y, int row)
	{
		for (int i = 0; i < (int)c.size(); i++)
		{
			Segment& s = c[i];
			glm::vec2 dst = c[i == (int)c.size() - 1 ? 0 : i + 1].point;

			if (s.type == SegmentType::Line)
			{
				if ((s.point.y < y) != (dst.y < y))
				{
					ZigzagCrossing x;
					x.point = glm::vec2(s.point.x + (y - s.point.y) * (dst.x - s.point.x) / (dst.y - s.point.y), y);
					x.curve = curve;
					x.segment = i;
					x.position = glm::distance(s.point, x.point);
					x.row = row;
					crossings.push_back(x);
				}
			}
			else
			{
				float dy = y - s.center.y;
				if (glm::abs(dy) >= s.radius)
					continue;

				float dx = glm::sqrt(s.radius * s.radius - dy * dy);
				// an arc which ends where it starts is a whole circle, a very small sweep is a very small arc
				float total = glm::distance(s.point, dst) < geometry::ERR_FLOAT6 ? glm::two_pi<float>() : zigzag_sweep(s, dst);

				for (int k = -1; k <= 1; k += 2)
				{
					glm::vec2 p(s.center.x + k * dx, y);
					float a = zigzag_sweep(s, p);
					if (a < total)
					{
						ZigzagCrossing x;
						x.point = p;
						x.curve = curve;
						x.segment = i;
						x.position = a * s.radius;
						x.row = row;
						crossings.push_back(x);
					}
				}
			}
		}
	}

	// add the start of a piece of the path, which follows the segment s from the point p, in its direction or backwards
	void zigzag_add(Curve& path, glm::vec2 p, Segment& s, bool forward)
	{
		if (s.type == SegmentType::Line)
			path.add(p);
		else
			path.add(p, s.center, s.radius, forward ? s.cw : !s.cw);
	}

	// follow the curve c from a crossing to the next (or the previous) one. The destination is the start of the next piece of the path,
	// so it is not added.
	void zigzag_wall(Curve& path, Curve& c, ZigzagCrossing& from, ZigzagCrossing& to, bool forward)
	{
		int size = (int)c.size();
		int k = from.segment;
		bool first = true;

		zigzag_add(path, from.point, c[k], forward);

		if (forward)
		{
			while (!(k == to.segment && (!first || to.position > from.position)))
			{
				first = false;
				k = (k + 1) % size;
				zigzag_add(path, c[k].point, c[k], true);
			}
		}
		else
		{
			while (!(k == to.segment && (!first || to.position < from.position)))
			{
				first = false;
				int previous = (k + size - 1) % size;
				zigzag_add(path, c[k].point, c[previous], false);
				k = previous;
			}
		}
	}

	// the curves are the outer contours and the islands of the same part, the scan lines are horizontal and go from the bottom to the top
	std::vector<Curve> zigzag_lines(std::vector<Curve>& curves, float step)
	{
		std::vector<Curve> result;

		if (curves.size() == 0 || step <= 0)
			return result;

		float bottom = std::numeric_limits<float>::max(), top = std::numeric_limits<float>::lowest();
		for (Curve& c : curves)
		{
			bottom = glm::min(bottom, c.bounds().bottom());
			top = glm::max(top, c.bounds().top());
		}

		// every scan line is crossed with the curves, the crossings are sorted along the line, the material is between the odd
		// crossings and the even ones
		std::vector<ZigzagCrossing> crossings;
		std::vector<ZigzagInterval> intervals;
		int rows = 0;
		for (int r = 1; bottom + r * step < top; r++)
		{
			float y = bottom + r * step;
			while (!zigzag_clear(curves, y))
				y += 2 * geometry::ERR_FLOAT3;

			std::vector<ZigzagCrossing> line;
			for (int i = 0; i < (int)curves.size(); i++)
				if (curves[i].bounds().bottom() < y && curves[i].bounds().top() > y)
					zigzag_cross(line, curves[i], i, y, rows);
			std::sort(line.begin(), line.end(), [](const ZigzagCrossing& x1, const ZigzagCrossing& x2) { return x1.point.x < x2.point.x; });

			for (int i = 0; i + 1 < (int)line.size(); i += 2)
			{
				ZigzagInterval interval;
				interval.left = (int)crossings.size();
				interval.right = interval.left + 1;
				line[i].interval = line[i + 1].interval = (int)intervals.size();
				intervals.push_back(interval);
				crossings.push_back(line[i]);
				crossings.push_back(line[i + 1]);
			}
			rows++;
		}

		// the crossings of each curve are sorted along it, to find the pieces of the curve which link a row to the next one
		std::vector<std::vector<int>> along(curves.size());
		for (int i = 0; i < (int)crossings.size(); i++)
			along[crossings[i].curve].push_back(i);
		for (std::vector<int>& a : along)
		{
			std::sort(a.begin(), a.end(), [&](int i1, int i2)
			{
				if (crossings[i1].segment != crossings[i2].segment)
					return crossings[i1].segment < crossings[i2].segment;
				return crossings[i1].position < crossings[i2].position;
			});
			for (int i = 0; i < (int)a.size(); i++)
				crossings[a[i]].rank = i;
		}

		// we go through the intervals, from the first one on the left to its right end, we follow the curve up to the next row, and we go
		// back on the next interval, and so on, until there is no interval left above
		for (int first = 0; first < (int)intervals.size(); first++)
		{
			if (intervals[first].used)
				continue;

			Curve path;
			int interval = first;
			int from = intervals[first].left;
			while (true)
			{
				intervals[interval].used = true;
				int to = from == intervals[interval].left ? intervals[interval].right : intervals[interval].left;
				path.add(crossings[from].point);

				// the destination is the next or the previous crossing of the curve, if it is on the next row
				std::vector<int>& a = along[crossings[to].curve];
				int size = (int)a.size();
				int next = -1;
				bool forward = true;
				for (int k = 0; k < 2 && next == -1; k++)
				{
					int candidate = a[(crossings[to].rank + (k == 0 ? 1 : size - 1)) % size];
					if (crossings[candidate].row == crossings[to].row + 1 && !intervals[crossings[candidate].interval].used)
					{
						next = candidate;
						forward = k == 0;
					}
				}

				if (next == -1)
				{
					path.add(crossings[to].point);
					break;
				}

				zigzag_wall(path, curves[crossings[to].curve], crossings[to], crossings[next], forward);
				interval = crossings[next].interval;
				from = next;
			}
			result.push_back(path);
		}

		return result;
	}
}

std::vector<Curve> Pocket::zigzag(std::vector<Curve>& curves)
{
	std::vector<Curve> result;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	// the scan lines are horizontal : the curves are turned by the opposite of the angle of the lines, and the lines are turned back
	const float rotation = glm::radians(_angle);
	auto rotate = [](Curve& c, float angle)
	{
		float cs = glm::cos(angle), sn = glm::sin(angle);
		for (Segment& s : c)
		{
			s.point = glm::vec2(s.point.x * cs - s.point.y * sn, s.point.x * sn + s.point.y * cs);
			s.center = glm::vec2(s.center.x * cs - s.center.y * sn, s.center.x * sn + s.center.y * cs);
		}
		c.reset_bounds();
	};

	for (std::vector<Curve>& group : sorted_levels)
	{
		for (Curve& c : group)
		{
			// the circles are turned into two arcs, to be followed like any other curve
			if (c.size() > 0 && c[0].type == SegmentType::Circle)
			{
				Curve arcs;
				glm::vec2 center = c[0].center;
				float r = c[0].radius;
				arcs.add(center + glm::vec2(r, 0), center, r, c[0].cw);
				arcs.add(center - glm::vec2(r, 0), center, r, c[0].cw);
				arcs.level(c.level());
				c = arcs;
			}

			if (rotation != 0)
				rotate(c, -rotation);
			else
				c.reset_bounds();
		}

		auto lines = zigzag_lines(group, step());
		for (Curve& l : lines)
		{
			if (rotation != 0)
				rotate(l, rotation);
			result.push_back(l);
		}
	}

	return result;
}

std::vector<Curve> Pocket::offset(std::vector<Curve>& curves)
{
	// a pocket which can not be computed (offsets that touch each other exactly for instance) must not freeze the application :
	// after this delay, the curves already computed are kept as they are
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);

	std::vector<Curve> result, temp;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	for (std::vector<Curve> inter : sorted_levels)
	{
		Curve::sort_level(inter);
		auto cw = inter.front().cw();
		const int first_level = inter.front().level();

		// the curves of the pocket stay in the outer contour of the group : a curve which gets out of it comes from a boolean operation
		// that failed (touching offsets for instance), and it would grow forever
		// (all the curves which are the material of the group are used, the first one can be a small piece)
		float outer_min_x = std::numeric_limits<float>::max(), outer_max_x = -std::numeric_limits<float>::max();
		float outer_min_y = std::numeric_limits<float>::max(), outer_max_y = -std::numeric_limits<float>::max();
		for (Curve& c : inter)
		{
			if (c.level() != inter.front().level())
				continue;
			geometry::rectangle& b = c.bounds();
			outer_min_x = glm::min(outer_min_x, glm::min(b.top_left.x, b.bottom_right.x));
			outer_max_x = glm::max(outer_max_x, glm::max(b.top_left.x, b.bottom_right.x));
			outer_min_y = glm::min(outer_min_y, glm::min(b.top_left.y, b.bottom_right.y));
			outer_max_y = glm::max(outer_max_y, glm::max(b.top_left.y, b.bottom_right.y));
		}
		outer_min_x -= geometry::ERR_FLOAT2; outer_max_x += geometry::ERR_FLOAT2; outer_min_y -= geometry::ERR_FLOAT2; outer_max_y += geometry::ERR_FLOAT2;

		// checking iner and outer, the inner contour should exist, otherwise there is an algo issue
		int smaller_level = std::numeric_limits<int>::max();
		int greater_level = std::numeric_limits<int>::max();

		while (inter.size() > 0 && smaller_level == greater_level && std::chrono::steady_clock::now() < deadline)
		{
			// sort the list following level, because if different level, we have to make the substrat based on lower level
			Curve::sort_level(inter);

			temp.clear();

			smaller_level = std::numeric_limits<int>::max();
			greater_level = std::numeric_limits<int>::max();

			for (Curve& c : inter)
			{
				auto o = c.offset((c.cw() == cw ? -radius() : radius()));
				smaller_level = glm::min(smaller_level, c.level());
				if (o.size() > 0)
					greater_level = glm::min(greater_level, c.level());
				temp.insert(temp.begin(), o.begin(), o.end());
			}

			// set when the remaining material lies entirely inside a growing hole
			bool region_empty = false;
			// every merge replaces curves by other curves, the budget guarantees that the loop ends even if a degenerate case would
			// give the same curves again and again
			int merge_budget = (int)temp.size() * 2 + 10;
			if (temp.size() > 1)
			{
				std::vector<Curve>::iterator it1 = temp.begin();
				while (it1 != temp.end() && temp.size() > 1 && !region_empty && merge_budget > 0 && std::chrono::steady_clock::now() < deadline)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != temp.end())
					{
						auto cw1 = (*it1).cw();
						auto cw2 = (*it2).cw();

						// curves which bounds do not touch have nothing to merge
						if ((*it1).bounds().outside((*it2).bounds()))
						{
							it2++;
							continue;
						}
						std::vector<Curve> merged_curves;

						// the lower level curve (the material) lies entirely inside a higher level curve (a hole offset):
						// there is nothing left to machine, and the holes must not keep growing outside of the material
						if ((*it1).level() != (*it2).level())
						{
							Curve& material = (*it1).level() < (*it2).level() ? (*it1) : (*it2);
							Curve& hole = (*it1).level() < (*it2).level() ? (*it2) : (*it1);
							if (material.inside(hole))
							{
								// other pieces of material remain : only this one is dropped
								const int material_level = material.level();
								if (std::count_if(temp.begin(), temp.end(), [&](Curve& c) { return c.level() == material_level; }) <= 1)
								{
									region_empty = true;
									break;
								}
								temp.erase((*it1).level() < (*it2).level() ? it1 : it2);
								merged = true;
								break;
							}
						}

						if ((*it1).level() != (*it2).level())
						{
							if ((*it1).level() < (*it2).level())
								merged_curves = (*it1).boolean_substract((*it2));
							else
								merged_curves = (*it2).boolean_substract((*it1));
						}
						else
							merged_curves = (*it1).boolean_union((*it2));

						if (!merge_plausible(*it1, *it2, merged_curves))
							merged_curves.clear();


						// the result keeps the lower level (the outer contour), whatever the order of the two curves
						int merged_level = glm::min((*it1).level(), (*it2).level());
						if (merged = merged_curves.size() > 0)
						{
							for (Curve& m : merged_curves)
							{
								m.close();
								//m.cw(cw1);
								// a subtraction can leave a hole inside the material (the holes enclosed a ring of material for instance) :
								// a curve which turns against the first curve is a hole, it must keep growing as a hole
								int level = merged_level;
								if ((*it1).level() != (*it2).level() && (level - first_level) % 2 == 0 && m.cw() != cw)
									level++;
								m.level(level);
							}
							merge_budget--;
							temp.erase(it2);
							temp.erase(it1);
							temp.insert(temp.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
						{
							(*it1).cw(cw1);
							(*it1).close();
							(*it2).cw(cw2);
							(*it2).close();
							it2++;
						}
					}
					if (merged)
						it1 = temp.begin();
					else
						it1++;
				}
			}

			if (region_empty)
				temp.clear();

			// a hole which neither lies in nor touches a curve of a lower level has no material left around : it is a piece of inner offset
			// which has to be excluded, otherwise it keeps growing outwards
			temp.erase(std::remove_if(temp.begin(), temp.end(), [&](Curve& c)
			{
				if (c.level() <= 1 || c.cw() == cw)
					return false;
				for (Curve& d : temp)
					if (d.level() < c.level() && !c.bounds().outside(d.bounds()) && (c.inside(d) || c.curve_intersect(d)))
						return false;
				return true;
			}), temp.end());

			temp.erase(std::remove_if(temp.begin(), temp.end(), [&](Curve& c)
			{
				geometry::rectangle& b = c.bounds();
				return glm::min(b.top_left.x, b.bottom_right.x) < outer_min_x || glm::max(b.top_left.x, b.bottom_right.x) > outer_max_x
					|| glm::min(b.top_left.y, b.bottom_right.y) < outer_min_y || glm::max(b.top_left.y, b.bottom_right.y) > outer_max_y;
			}), temp.end());

			auto s = temp.size();

			// without any material left, the holes have nothing to cut and must not keep growing
			if (std::none_of(temp.begin(), temp.end(), [&](Curve& c) { return c.level() == smaller_level; }))
				temp.clear();

			result.insert(result.end(), temp.begin(), temp.end());
			inter = temp;
		}
	}

	for (auto& c : result)
		c.close();

	return result;
}

std::vector<Curve> Pocket::spiral(std::vector<Curve> loops)
{
	std::vector<Curve> result;
	if (loops.empty())
		return result;

	// circles become closed curves of two arcs
	for (Curve& c : loops)
	{
		if (c.size() > 0 && c[0].type == SegmentType::Circle)
		{
			Curve arcs;
			glm::vec2 center = c[0].center;
			float r = c[0].radius;
			arcs.add(center + glm::vec2(r, 0), center, r, c[0].cw);
			arcs.add(center - glm::vec2(r, 0), center, r, c[0].cw);
			arcs.add(center + glm::vec2(r, 0), center, r, c[0].cw);
			arcs.level(c.level());
			c = arcs;
		}
		c.close();
	}

	// the same curve, starting at its vertex k (the last point of a closed curve is its first one)
	auto rotate = [](Curve& c, int k)
	{
		Curve out;
		int n = (int)c.size();
		for (int i = k; i < n - 1; i++)
			out.push_back(c[i]);
		for (int i = 0; i < k; i++)
			out.push_back(c[i]);
		out.push_back(Segment(c[k].point));
		out.level(c.level());
		return out;
	};

	// links longer than that start a new spiral (another island of the pocket)
	const float max_link = radius() * 6.0f;

	// point at a distance along a closed curve
	auto point_at = [](Curve& c, float d)
	{
		for (size_t k = 0; k + 1 < c.size(); k++)
		{
			if (d <= c[k].length || k + 2 == c.size())
				return c[k].coordinates_at(glm::clamp(d, 0.0f, c[k].length), c[k + 1].point);
			d -= c[k].length;
		}
		return c.front().point;
	};

	// a chain is a list of loops, from the outside to the inside, each one starting near the end of the previous one
	std::vector<std::vector<Curve>> chains;
	chains.push_back(std::vector<Curve>{ rotate(loops.front(), 0) });
	loops.erase(loops.begin());

	while (!loops.empty())
	{
		glm::vec2 end = chains.back().back().back().point;
		float best = std::numeric_limits<float>::max();
		int best_loop = 0, best_vertex = 0;
		for (int i = 0; i < (int)loops.size(); i++)
		{
			for (int k = 0; k < (int)loops[i].size() - 1; k++)
			{
				float d = glm::distance(end, loops[i][k].point);
				if (d < best)
				{
					best = d;
					best_loop = i;
					best_vertex = k;
				}
			}
		}

		Curve next = rotate(loops[best_loop], best_vertex);
		loops.erase(loops.begin() + best_loop);

		if (best <= max_link)
			chains.back().push_back(next);
		else
			chains.push_back(std::vector<Curve>{ next });
	}

	// each turn of the spiral moves progressively from a loop to the next one : the point at the fraction t of the turn is the
	// blend (1 - t) / t of the points at the fraction t of the two loops
	if (_outward)
		for (std::vector<Curve>& chain : chains)
			std::reverse(chain.begin(), chain.end());

	const float sample = glm::max(radius() / 2.0f, 0.05f);
	for (std::vector<Curve>& chain : chains)
	{
		Curve path;

		// going inwards, the spiral starts with a full turn on the outer loop (going outwards, it ends with it)
		if (!_outward && chain.size() > 1)
			for (Segment& s : chain.front())
				path.push_back(s);

		for (size_t i = 0; i + 1 < chain.size(); i++)
		{
			Curve& a = chain[i];
			Curve& b = chain[i + 1];
			float la = a.length(), lb = b.length();
			int count = glm::clamp((int)glm::ceil(glm::max(la, lb) / sample), 16, 4000);
			for (int n = 0; n < count; n++)
			{
				float t = (float)n / count;
				glm::vec2 pa = point_at(a, t * la), pb = point_at(b, t * lb);
				path.push_back(Segment(pa * (1.0f - t) + pb * t));
			}
		}

		// the last loop is followed as it is
		Curve& last = chain.back();
		for (Segment& s : last)
			path.push_back(s);
		path.level(last.level());
		result.push_back(path);
	}

	return result;
}

std::vector<Curve> Pocket::merge(std::vector<Curve>& curves)
{
	// a pocket which can not be computed (offsets that touch each other exactly for instance) must not freeze the application :
	// after this delay, the curves already computed are kept as they are
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);

	std::vector<Curve> result, temp;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	for (std::vector<Curve> inter : sorted_levels)
	{
		Curve::sort_level(inter);
		auto cw = inter.front().cw();
		const int first_level = inter.front().level();

		// checking iner and outer, the inner contour should exist, otherwise there is an algo issue
		int smaller_level = std::numeric_limits<int>::max();
		int greater_level = std::numeric_limits<int>::max();

		while (inter.size() > 0 && smaller_level == greater_level && std::chrono::steady_clock::now() < deadline)
		{
			// sort the list following level, because if different level, we have to make the substrat based on lower level
			Curve::sort_level(inter);

			temp.clear();

			smaller_level = std::numeric_limits<int>::max();
			greater_level = std::numeric_limits<int>::max();

			for (Curve& c : inter)
			{
				smaller_level = glm::min(smaller_level, c.level());
				temp.push_back(c);
			}

			// set when the remaining material lies entirely inside a growing hole
			bool region_empty = false;
			// every merge replaces curves by other curves, the budget guarantees that the loop ends even if a degenerate case would
			// give the same curves again and again
			int merge_budget = (int)temp.size() * 2 + 10;
			if (temp.size() > 1)
			{
				std::vector<Curve>::iterator it1 = temp.begin();
				while (it1 != temp.end() && temp.size() > 1 && !region_empty && merge_budget > 0 && std::chrono::steady_clock::now() < deadline)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != temp.end())
					{
						auto cw1 = (*it1).cw();
						auto cw2 = (*it2).cw();

						// curves which bounds do not touch have nothing to merge
						if ((*it1).bounds().outside((*it2).bounds()))
						{
							it2++;
							continue;
						}
						std::vector<Curve> merged_curves;

						// the lower level curve (the material) lies entirely inside a higher level curve (a hole offset):
						// there is nothing left to machine, and the holes must not keep growing outside of the material
						if ((*it1).level() != (*it2).level())
						{
							Curve& material = (*it1).level() < (*it2).level() ? (*it1) : (*it2);
							Curve& hole = (*it1).level() < (*it2).level() ? (*it2) : (*it1);
							if (material.inside(hole))
							{
								// other pieces of material remain : only this one is dropped
								const int material_level = material.level();
								if (std::count_if(temp.begin(), temp.end(), [&](Curve& c) { return c.level() == material_level; }) <= 1)
								{
									region_empty = true;
									break;
								}
								temp.erase((*it1).level() < (*it2).level() ? it1 : it2);
								merged = true;
								break;
							}
						}

						if ((*it1).level() != (*it2).level())
						{
							if ((*it1).level() < (*it2).level())
								merged_curves = (*it1).boolean_substract((*it2));
							else
								merged_curves = (*it2).boolean_substract((*it1));
						}
						else
							merged_curves = (*it1).boolean_union((*it2));

						if (!merge_plausible(*it1, *it2, merged_curves))
							merged_curves.clear();


						// the result keeps the lower level (the outer contour), whatever the order of the two curves
						int merged_level = glm::min((*it1).level(), (*it2).level());
						if (merged = merged_curves.size() > 0)
						{
							for (Curve& m : merged_curves)
							{
								m.close();
								// a subtraction can leave a hole inside the material (the holes enclosed a ring of material for instance) :
								// a curve which turns against the first curve is a hole, it must keep growing as a hole
								int level = merged_level;
								if ((*it1).level() != (*it2).level() && (level - first_level) % 2 == 0 && m.cw() != cw)
									level++;
								m.level(level);
							}
							merge_budget--;
							temp.erase(it2);
							temp.erase(it1);
							temp.insert(temp.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
						{
							(*it1).cw(cw1);
							(*it1).close();
							(*it2).cw(cw2);
							(*it2).close();
							it2++;
						}
					}
					if (merged)
						it1 = temp.begin();
					else
						it1++;
				}
			}

			if (region_empty)
				temp.clear();

			// a hole which neither lies in nor touches a curve of a lower level has no material left around : it is a piece of inner offset
			// which has to be excluded, otherwise it keeps growing outwards
			temp.erase(std::remove_if(temp.begin(), temp.end(), [&](Curve& c)
			{
				if (c.level() <= 1 || c.cw() == cw)
					return false;
				for (Curve& d : temp)
					if (d.level() < c.level() && !c.bounds().outside(d.bounds()) && (c.inside(d) || c.curve_intersect(d)))
						return false;
				return true;
			}), temp.end());

			auto s = temp.size();

			// without any material left, the holes have nothing to cut and must not keep growing
			if (std::none_of(temp.begin(), temp.end(), [&](Curve& c) { return c.level() == smaller_level; }))
				temp.clear();

			result.insert(result.end(), temp.begin(), temp.end());
			inter = temp;
		}
	}

	for (auto c : result)
		c.close();

	return result;
}

void Pocket::refresh(TreeCurve* value)
{
	if (_tree != nullptr)
		delete _tree;
	_tree = value;
	compute();
}

// The machined regions of a tree: an outline (even level) with its islands, which are the curves directly inside it. What is inside an
// island is machined too, but it is another region, separated from the first one by the material of the island: its outline is a
// new one. The levels of a region are 1 for the outline and 2 for the islands (the algorithms pair the levels 1-2, 3-4...), as in a simple pocket.
static void collect_regions(TreeCurve* node, std::vector<std::vector<Curve>>& regions)
{
	if (node == nullptr)
		return;
	std::vector<Curve> region;
	if (node->curve != nullptr && node->curve->size() > 0)
	{
		Curve outline = *node->curve;
		outline.level(1);
		region.push_back(outline);
	}
	std::vector<TreeCurve*> inside;
	for (TreeCurve* island : node->children)
	{
		if (island->curve != nullptr && island->curve->size() > 0)
		{
			Curve c = *island->curve;
			c.level(2);
			region.push_back(c);
		}
		inside.insert(inside.end(), island->children.begin(), island->children.end());
	}
	if (!region.empty())
	{
		for (size_t i = 0; i < region.size(); i++)
			region[i].index((int)i);
		regions.push_back(region);
	}
	for (TreeCurve* next : inside)
		collect_regions(next, regions);
}

bool Pocket::defer_read = false;

void Pocket::update()
{
	calculate();
	generate();
}

void Pocket::generate()
{
	if (_tree != nullptr)
	{
		generate_data(_computed);
		generate_deco(_computed);
	}
}

void Pocket::calculate()
{
	_calculated = true;
	if (_tree != nullptr)
	{
		_computed.clear();

		// the finishing passes of all the parts are machined at the very end
		std::vector<Curve> last;

		_tree->cw_alter(!cw());

		// The parts (a region: an outline with the islands inside it; the inside of a letter is another one) are independent: they are computed together, each one in
		// its own result, and put one after the other in the order of the tree, as a sequential loop does
		struct Part
		{
			std::vector<Curve> computed;
			std::vector<Curve> last;
		};
		std::vector<std::vector<Curve>> regions;
		for (TreeCurve* top : _tree->children)
			collect_regions(top, regions);
		std::vector<Part> parts(regions.size());
		parallel_for(parts.size(), [&](size_t index)
		{
			std::vector<Curve>& computed = parts[index].computed;
			std::vector<Curve>& last = parts[index].last;
			std::vector<Curve> inner;
			std::vector<Curve> original = regions[index];
			bool cw = original[0].cw();


			// we first proceed to an offset with the radius
			for (Curve& c : original)
			{
				c.reduce(0.25f);
				auto curves = c.offset((c.cw() == cw ? -radius() : radius()));
				inner.insert(inner.end(), curves.begin(), curves.end());
			}
			inner = merge(inner);

			// if finishing is set, we offset by finishing
			std::vector<Curve> finish;
			if (_finishing > 0)
			{
				// the first offset is the finishing one, the roughing one is farther inside, by the value of the finishing
				for (Curve& c : inner)
				{
					auto curves = c.offset((c.cw() == cw ? -_finishing : _finishing));
					finish.insert(finish.end(), curves.begin(), curves.end());
				}
				finish = merge(finish);
			}
			else
				finish.insert(finish.end(), inner.begin(), inner.end());

			std::vector<Curve> process;
			if (finish.size() > 0)
			{
				if (_mode == PocketMode::Zigzag)
					process = zigzag(finish);
				else if (_mode == PocketMode::Offset)
					process = offset(finish);
				else if (_mode == PocketMode::Spiral)
					process = spiral(offset(finish));
			}


			computed.insert(computed.end(), process.begin(), process.end());

			// the last offset is the finishing pass : the first one when there is a finishing value, the only one otherwise. The zigzag
			// can be machined without the contour of its area
			bool contours = _mode != PocketMode::Zigzag || _contours;
			if (_finishing > 0)
			{
				if (contours)
					computed.insert(computed.end(), finish.begin(), finish.end());
				last.insert(last.end(), inner.begin(), inner.end());
			}
			else if (contours)
				last.insert(last.end(), finish.begin(), finish.end());
		});
		for (Part& part : parts)
		{
			_computed.insert(_computed.end(), part.computed.begin(), part.computed.end());
			last.insert(last.end(), part.last.begin(), part.last.end());
		}

		// the finishing pass stays at the very end, whatever the order of the other curves
		_final_count = last.size();
		_computed.insert(_computed.end(), last.begin(), last.end());
	}
}

void Pocket::scaled()
{
	generate_deco(_computed);
}

std::vector<Curve> Pocket::coordinates()
{
	std::vector<Curve> result;

	for (Curve c : _computed)
	{
		Curve n = c;
		result.push_back(n);
	}
	return result;
}

void Pocket::properties()
{
	int mode_index = (int)_mode;
	if (property_combo(Lang::l("MODE"), "##MODE", mode_index, { "Zigzag", "Offset", "Spiral" }))
		modify([&] { mode((PocketMode)mode_index); });

	float finishing_value = _finishing;
	if (property_float(Lang::l("FINISHING"), "##FINISHING", finishing_value))
		modify([&] { finishing(glm::max(finishing_value, 0.0f)); });

	if (_mode == PocketMode::Zigzag)
	{
		float step_value = step();
		if (property_float(Lang::l("STEPOVER"), "##STEPOVER", step_value))
			modify([&] { stepover(glm::max(step_value, 0.0f)); });

		float angle_value = _angle;
		if (property_float(Lang::l("ANGLE"), "##ANGLE", angle_value))
			modify([&] { angle(angle_value); });

		bool contours_value = _contours;
		if (property_bool(Lang::l("CONTOURS"), "##CONTOURS", contours_value))
			modify([&] { contours(contours_value); });
	}
	else if (_mode == PocketMode::Spiral)
	{
		bool outward_value = _outward;
		if (property_bool(Lang::l("SPIRAL_OUTWARD"), "##SPIRAL_OUTWARD", outward_value))
			modify([&] { outward(outward_value); });
	}
}

std::string Pocket::write()
{
	std::string result =
		WN((short)type()) +
		WN(id()) +
		WS(parent()) +
		WS(_name) +
		WN(_radius) +
		WB(_cw) +
		WN((int)_mode) +
		WN(_finishing) +
		WN(_stepover) +
		WN(_angle) +
		WB(_climb) +
		WB(_contours) +
		WB(_outward);

	auto ref = references();
	result += WS(std::string("[#REF_IDS#]"));
	result += WN(ref.size());
	for (int i = 0; i < ref.size(); i++)
		result += WN(ref[i]);

	// the curves which the pocket is computed from
	std::vector<Curve> curves;
	if (_tree != nullptr)
		curves = _tree->curves();

	result += WS(std::string("[#COORDINATES#]"));
	result += WN(curves.size());
	for (auto& c : curves)
	{
		result += WN(c.size());
		for (int i = 0; i < c.size(); i++)
			result += WSEGMENT(c[i]);
	}

	return result;
}

void Pocket::read(std::string value, float version)
{
	_original.clear();
	auto data = stringex::split(value, ';');

	RU(_id, 1);
	RT(2);
	RS(_name, 3);
	RF(_radius, 4);
	RB(_cw, 5);
	int mode = (int)_mode;
	RI(mode, 6);
	_mode = (PocketMode)mode;
	RF(_finishing, 7);
	RF(_stepover, 8);
	RF(_angle, 9);
	RB(_climb, 10);

	// the files written before this option have the references block here
	if (data.size() > 11 && data[11] != "[#REF_IDS#]")
	{
		RB(_contours, 11);
		if (data.size() > 12 && data[12] != "[#REF_IDS#]")
			RB(_outward, 12);
	}

	int ref = -1;
	for (int i = 0; i < data.size() && ref == -1; i++)
	{
		if (data[i] == "[#REF_IDS#]")
			ref = i + 1;
	}
	if (ref != -1)
	{
		std::vector<int> ids;
		int count = 0, id = 0;
		RI(count, ref);
		ref++;
		for (int i = 0; i < count; i++)
		{
			RI(id, ref + i);
			ids.push_back(id);
		}
		references(ids);
	}

	ref = -1;
	for (int i = 0; i < data.size() && ref == -1; i++)
	{
		if (data[i] == "[#COORDINATES#]")
			ref = i + 1;
	}
	if (ref != -1)
	{
		std::vector<Curve> curves;
		int count = 0;
		RI(count, ref);
		ref++;
		for (int i = 0; i < count; i++)
		{
			int size = 0;
			RI(size, ref);
			ref++;
			Curve c;
			for (int j = 0; j < size; j++)
			{
				Segment s;
				RSEGMENT(s, ref);
				c.add(s.type, s.point, s.center, s.radius, s.cw);
				ref++;
			}
			curves.push_back(c);
		}

		// the tree is built like the one of a new pocket
		if (curves.size() > 0)
		{
			TreeCurve* tree = new TreeCurve(curves);
			tree->sort();
			tree->cw_alter();
			_tree = tree;

		}
	}
	// when a document is read, all its pockets are computed together afterwards
	_calculated = false;
	if (!defer_read)
		update();
}
