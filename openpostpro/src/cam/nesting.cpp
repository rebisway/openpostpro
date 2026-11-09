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

#include "nesting.h"
#include <algorithm>
#include <cmath>

namespace nesting
{
	namespace
	{
		const float EPS = 1e-4f;

		struct Box
		{
			glm::vec2 min = glm::vec2(1e30f);
			glm::vec2 max = glm::vec2(-1e30f);
			void add(glm::vec2 p) { min = glm::min(min, p); max = glm::max(max, p); }
			bool separate(const Box& b) const { return max.x <= b.min.x || b.max.x <= min.x || max.y <= b.min.y || b.max.y <= min.y; }
		};

		// a part turned by one angle, its lower left corner of bounds at the origin
		struct Variant
		{
			float angle = 0;
			glm::vec2 offset = glm::vec2();	// to apply after the rotation to bring the bounds corner to the origin
			Ring outer;
			std::vector<Ring> holes;
			Box box;
			std::vector<Box> boxes;	// bounds of each ring: the outer one, then the holes
			glm::vec2 size = glm::vec2();
		};

		// a part already on the sheet: the variant, the position of its bounds corner
		struct Placed
		{
			const Variant* variant;
			glm::vec2 position;
			Box box;
		};

		glm::vec2 turn(glm::vec2 p, float c, float s)
		{
			return glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c);
		}

		Ring turned(const Ring& ring, float angle)
		{
			float a = glm::radians(angle);
			float c = std::cos(a), s = std::sin(a);
			c = std::fabs(c) < 1e-6f ? 0.0f : c;
			s = std::fabs(s) < 1e-6f ? 0.0f : s;
			Ring r;
			r.reserve(ring.size());
			for (glm::vec2 p : ring)
				r.push_back(turn(p, c, s));
			return r;
		}

		Variant make_variant(const Part& part, float angle)
		{
			Variant v;
			v.angle = angle;
			v.outer = turned(part.outer, angle);
			for (const Ring& h : part.holes)
				v.holes.push_back(turned(h, angle));
			for (glm::vec2 p : v.outer)
				v.box.add(p);
			v.offset = -v.box.min;
			for (glm::vec2& p : v.outer)
				p += v.offset;
			for (Ring& h : v.holes)
				for (glm::vec2& p : h)
					p += v.offset;
			v.boxes.resize(1 + v.holes.size());
			for (glm::vec2 p : v.outer)
				v.boxes[0].add(p);
			for (size_t i = 0; i < v.holes.size(); i++)
				for (glm::vec2 p : v.holes[i])
					v.boxes[i + 1].add(p);
			v.size = v.box.max - v.box.min;
			v.box.min = glm::vec2();
			v.box.max = v.size;
			return v;
		}

		float cross(glm::vec2 o, glm::vec2 a, glm::vec2 b)
		{
			return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
		}

		// the two segments cross each other strictly (touching or lying along each other is not a crossing)
		bool crossing(glm::vec2 a1, glm::vec2 a2, glm::vec2 b1, glm::vec2 b2)
		{
			float d1 = cross(a1, a2, b1), d2 = cross(a1, a2, b2);
			float d3 = cross(b1, b2, a1), d4 = cross(b1, b2, a2);
			return ((d1 > EPS && d2 < -EPS) || (d1 < -EPS && d2 > EPS)) && ((d3 > EPS && d4 < -EPS) || (d3 < -EPS && d4 > EPS));
		}

		float distance_segment(glm::vec2 p, glm::vec2 a, glm::vec2 b)
		{
			glm::vec2 ab = b - a;
			float l = glm::dot(ab, ab);
			float t = l > 0 ? glm::clamp(glm::dot(p - a, ab) / l, 0.0f, 1.0f) : 0;
			return glm::length(p - (a + ab * t));
		}

		// -1 on the ring, 0 outside, 1 inside
		int point_in_ring(glm::vec2 p, const Ring& ring, glm::vec2 shift)
		{
			bool in = false;
			size_t n = ring.size();
			for (size_t i = 0, j = n - 1; i < n; j = i++)
			{
				glm::vec2 a = ring[i] + shift, b = ring[j] + shift;
				if (distance_segment(p, a, b) < EPS * 10)
					return -1;
				if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
					in = !in;
			}
			return in ? 1 : 0;
		}

		// the point is strictly inside the material of the variant placed at shift
		bool in_material(glm::vec2 p, const Variant& v, glm::vec2 shift)
		{
			if (point_in_ring(p, v.outer, shift) != 1)
				return false;
			for (const Ring& h : v.holes)
			{
				int r = point_in_ring(p, h, shift);
				if (r != 0)
					return false;
			}
			return true;
		}

		bool rings_cross(const Ring& ra, glm::vec2 sa, const Ring& rb, glm::vec2 sb, const Box& ba, const Box& bb)
		{
			size_t na = ra.size(), nb = rb.size();
			for (size_t i = 0, j = na - 1; i < na; j = i++)
			{
				glm::vec2 a1 = ra[j] + sa, a2 = ra[i] + sa;
				float x0 = std::min(a1.x, a2.x), x1 = std::max(a1.x, a2.x), y0 = std::min(a1.y, a2.y), y1 = std::max(a1.y, a2.y);
				if (x1 < bb.min.x || x0 > bb.max.x || y1 < bb.min.y || y0 > bb.max.y)
					continue;
				for (size_t k = 0, l = nb - 1; k < nb; l = k++)
				{
					glm::vec2 b1 = rb[l] + sb, b2 = rb[k] + sb;
					if (std::max(b1.x, b2.x) < x0 || std::min(b1.x, b2.x) > x1 || std::max(b1.y, b2.y) < y0 || std::min(b1.y, b2.y) > y1)
						continue;
					if (crossing(a1, a2, b1, b2))
						return true;
				}
			}
			return false;
		}

		// shared material of two placed variants
		bool overlap_variants(const Variant& a, glm::vec2 pa, const Variant& b, glm::vec2 pb)
		{
			Box ba{ pa, pa + a.size }, bb{ pb, pb + b.size };
			if (ba.separate(bb))
				return false;

			// a part which starts inside the material of the other one: no need to look at the edges
			if (!b.outer.empty() && in_material(b.outer[0] + pb, a, pa))
				return true;
			if (!a.outer.empty() && in_material(a.outer[0] + pa, b, pb))
				return true;

			// edges which cross each other (the rings whose bounds are apart are not compared)
			for (size_t i = 0; i <= a.holes.size(); i++)
			{
				const Ring& x = i == 0 ? a.outer : a.holes[i - 1];
				Box xb{ a.boxes[i].min + pa, a.boxes[i].max + pa };
				if (xb.separate(bb))
					continue;
				for (size_t j = 0; j <= b.holes.size(); j++)
				{
					const Ring& y = j == 0 ? b.outer : b.holes[j - 1];
					Box yb{ b.boxes[j].min + pb, b.boxes[j].max + pb };
					if (xb.separate(yb))
						continue;
					if (rings_cross(x, pa, y, pb, xb, yb))
						return true;
				}
			}

			// no crossing: one outer ring can still be inside the material of the other one
			for (glm::vec2 p : b.outer)
				if (in_material(p + pb, a, pa))
					return true;
			for (glm::vec2 p : a.outer)
				if (in_material(p + pa, b, pb))
					return true;
			// same outline put on each other: the centre of the first edge is inside both
			if (!b.outer.empty())
			{
				glm::vec2 m = (b.outer[0] + b.outer[1 % b.outer.size()]) * 0.5f + pb;
				glm::vec2 d = (b.outer[1 % b.outer.size()] - b.outer[0]);
				glm::vec2 n = glm::vec2(-d.y, d.x);
				float l = glm::length(n);
				if (l > 0)
				{
					glm::vec2 q = m - n / l * 0.05f;	// a point just behind the edge (the ring is counter clockwise or not: both sides tried)
					glm::vec2 r = m + n / l * 0.05f;
					if (in_material(q, a, pa) || in_material(r, a, pa))
						if (in_material(q, b, pb) || in_material(r, b, pb))
							return true;
				}
			}
			return false;
		}

		// blocker: the part which stopped the previous position, tried first (the next position is likely stopped by the same one)
		bool fits(const Variant& v, glm::vec2 position, const std::vector<Placed>& placed, size_t& blocker)
		{
			Box box{ position, position + v.size };
			if (blocker < placed.size())
			{
				const Placed& p = placed[blocker];
				if (!box.separate(p.box) && overlap_variants(v, position, *p.variant, p.position))
					return false;
			}
			for (size_t i = 0; i < placed.size(); i++)
			{
				if (i == blocker)
					continue;
				const Placed& p = placed[i];
				if (box.separate(p.box))
					continue;
				if (overlap_variants(v, position, *p.variant, p.position))
				{
					blocker = i;
					return false;
				}
			}
			return true;
		}

		float ring_area(const Ring& r)
		{
			double s = 0;
			for (size_t i = 0, j = r.size() - 1; i < r.size(); j = i++)
				s += (double)r[j].x * r[i].y - (double)r[i].x * r[j].y;
			return (float)std::fabs(s * 0.5);
		}
	}

	Ring transform(const Ring& ring, float angle, glm::vec2 offset)
	{
		Ring r = turned(ring, angle);
		for (glm::vec2& p : r)
			p += offset;
		return r;
	}

	bool overlap(const Part& a, const Placement& pa, const Part& b, const Placement& pb)
	{
		Variant va = make_variant(a, pa.angle), vb = make_variant(b, pb.angle);
		// the placement offset is given for the rotated part: the position of the bounds corner follows
		glm::vec2 qa = pa.offset - va.offset, qb = pb.offset - vb.offset;
		return overlap_variants(va, qa, vb, qb);
	}

	float area(const Part& part)
	{
		float a = ring_area(part.outer);
		for (const Ring& h : part.holes)
			a -= ring_area(h);
		return a;
	}

	std::vector<Placement> nest(const std::vector<Part>& parts, const Options& options)
	{
		struct Item { int part; float area; };
		std::vector<Item> items;
		for (size_t i = 0; i < parts.size(); i++)
			for (int q = 0; q < parts[i].quantity; q++)
				items.push_back({ (int)i, area(parts[i]) });
		// the biggest first: the small ones then fill the holes and the gaps
		std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.area > b.area; });

		std::vector<std::vector<Variant>> variants(parts.size());
		for (size_t i = 0; i < parts.size(); i++)
			for (float a : options.angles)
				variants[i].push_back(make_variant(parts[i], a));

		std::vector<Placed> placed;
		std::vector<Placement> result;
		size_t blocker = 0;
		glm::vec2 lo(options.margin), hi(options.width - options.margin, options.height - options.margin);

		for (const Item& item : items)
		{
			const Variant* best = nullptr;
			glm::vec2 best_position = glm::vec2();
			float best_score = 1e30f;

			for (const Variant& v : variants[item.part])
			{
				if (v.size.x > hi.x - lo.x + EPS || v.size.y > hi.y - lo.y + EPS)
					continue;
				// scan from the left: the first column with a free position wins
				for (float x = lo.x; x + v.size.x <= hi.x + EPS; x += options.step)
				{
					if (x - lo.x > best_score)
						break;
					bool found = false;
					for (float y = lo.y; y + v.size.y <= hi.y + EPS; y += options.step)
					{
						if (!fits(v, glm::vec2(x, y), placed, blocker))
							continue;
						glm::vec2 p(x, y);
						// slide towards the lower left with smaller and smaller moves
						for (float s = options.step * 0.5f; s >= options.precision; s *= 0.5f)
						{
							bool moved = true;
							while (moved)
							{
								moved = false;
								glm::vec2 left(p.x - s, p.y), down(p.x, p.y - s);
								if (left.x >= lo.x && fits(v, left, placed, blocker)) { p = left; moved = true; }
								else if (down.y >= lo.y && fits(v, down, placed, blocker)) { p = down; moved = true; }
							}
						}
						float score = p.x + p.y * 0.001f;
						if (score < best_score)
						{
							best_score = score;
							best = &v;
							best_position = p;
						}
						found = true;
						break;
					}
					if (found)
						break;
				}
			}

			Placement pl;
			pl.part = item.part;
			if (best != nullptr)
			{
				pl.placed = true;
				pl.angle = best->angle;
				pl.offset = best_position + best->offset;
				placed.push_back({ best, best_position, Box{ best_position, best_position + best->size } });
			}
			result.push_back(pl);
		}
		return result;
	}

	float utilisation(const std::vector<Part>& parts, const std::vector<Placement>& placements, const Options& options)
	{
		double used = 0;
		for (const Placement& p : placements)
			if (p.placed)
				used += area(parts[p.part]);
		return (float)(used / ((double)options.width * options.height));
	}
}
