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
#include <parallel.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <limits>
#include <mutex>
#include <random>
#include <thread>

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
			std::vector<Ring> inner;
			Box box;
			std::vector<Box> boxes;	// bounds of each ring: the outer one, then the inner ones
			glm::vec2 size = glm::vec2();

			// The grid of the cells (see Occupancy): the cells of the bounds, the ones whose centre is in the material, and the core of the
			// material (the cell and its 8 neighbours are in the material). cells[k] gives the core as offsets in the grid of the sheet.
			int cols = 0, rows = 0;
			std::vector<unsigned char> core;
			std::vector<int> cells;
			// the material as bits, one row after the other (the algorithm which minimizes the overlap)
			// (a margin of one cell around the bounds; the dilated mask has the cells which are at one cell or less from the material)
			int mcols = 0, mrows = 0, mwords = 0;
			std::vector<unsigned long long> mbits, dbits;
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
			for (const Ring& h : part.inner)
				v.inner.push_back(turned(h, angle));
			for (glm::vec2 p : v.outer)
				v.box.add(p);
			v.offset = -v.box.min;
			for (glm::vec2& p : v.outer)
				p += v.offset;
			for (Ring& h : v.inner)
				for (glm::vec2& p : h)
					p += v.offset;
			v.boxes.resize(1 + v.inner.size());
			for (glm::vec2 p : v.outer)
				v.boxes[0].add(p);
			for (size_t i = 0; i < v.inner.size(); i++)
				for (glm::vec2 p : v.inner[i])
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

		// the point is strictly inside the material of the variant placed at shift: it is inside the outer ring and an even number of
		// inner rings (none, or the hole and its island...), and on no ring
		bool in_material(glm::vec2 p, const Variant& v, glm::vec2 shift)
		{
			if (point_in_ring(p, v.outer, shift) != 1)
				return false;
			int depth = 0;
			for (const Ring& r : v.inner)
			{
				int where = point_in_ring(p, r, shift);
				if (where == -1)
					return false;
				depth += where;
			}
			return depth % 2 == 0;
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
			for (size_t i = 0; i <= a.inner.size(); i++)
			{
				const Ring& x = i == 0 ? a.outer : a.inner[i - 1];
				Box xb{ a.boxes[i].min + pa, a.boxes[i].max + pa };
				if (xb.separate(bb))
					continue;
				for (size_t j = 0; j <= b.inner.size(); j++)
				{
					const Ring& y = j == 0 ? b.outer : b.inner[j - 1];
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
			// Outlines put on each other, or edges which lie along each other (a ring of one part on a ring of the other one, the arm of an L
			// on the arm of another L): nothing crosses and no vertex is strictly inside. Beside the middle of some edges of each ring, on each
			// side, a point which is in the material of both parts is an overlap. The edges are spread along the ring (all of them for a ring
			// with few edges).
			auto probe = [&](const Variant& owner, glm::vec2 po)
			{
				for (size_t k = 0; k <= owner.inner.size(); k++)
				{
					const Ring& ring = k == 0 ? owner.outer : owner.inner[k - 1];
					if (ring.size() < 2)
						continue;
					size_t gap = std::max<size_t>(1, ring.size() / 8);
					for (size_t e = 0; e < ring.size(); e += gap)
					{
						glm::vec2 from = ring[e], to = ring[(e + 1) % ring.size()];
						glm::vec2 d = to - from;
						float l = glm::length(d);
						if (l <= 0)
							continue;
						glm::vec2 m = (from + to) * 0.5f + po;
						glm::vec2 n = glm::vec2(-d.y, d.x) / l * 0.05f;
						for (glm::vec2 q : { m - n, m + n })
							if (in_material(q, a, pa) && in_material(q, b, pb))
								return true;
					}
				}
				return false;
			};
			if (probe(a, pa) || probe(b, pb))
				return true;
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
		for (size_t i = 0; i < part.inner.size(); i++)
		{
			// a ring inside an even number of other inner rings is a hole, inside an odd number an island
			int around = 0;
			for (size_t j = 0; j < part.inner.size(); j++)
				if (j != i && point_in_ring(part.inner[i][0], part.inner[j], glm::vec2()) == 1)
					around++;
			a += around % 2 == 0 ? -ring_area(part.inner[i]) : ring_area(part.inner[i]);
		}
		return a;
	}

	namespace
	{
		// The cells of a variant: the grid has the origin at the corner of the bounds. A scan line gives the cells whose centre is in the material
		// (the rings together: inside of an odd number of them).
		std::vector<unsigned char> rasterize(const Variant& v, float cell, int& cols, int& rows)
		{
			cols = (int)std::ceil(v.size.x / cell);
			rows = (int)std::ceil(v.size.y / cell);
			std::vector<unsigned char> in((size_t)cols * rows, 0);
			std::vector<float> xs;
			auto edges = [&](const Ring& r, float y)
			{
				for (size_t e = 0; e < r.size(); e++)
				{
					glm::vec2 a = r[e], b = r[(e + 1) % r.size()];
					if ((a.y > y) != (b.y > y))
						xs.push_back(a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y));
				}
			};
			for (int j = 0; j < rows; j++)
			{
				float y = (j + 0.5f) * cell;
				xs.clear();
				edges(v.outer, y);
				for (const Ring& r : v.inner)
					edges(r, y);
				std::sort(xs.begin(), xs.end());
				for (size_t k = 0; k + 1 < xs.size(); k += 2)
				{
					int first = std::max(0, (int)std::ceil(xs[k] / cell - 0.5f)), last = std::min(cols - 1, (int)std::floor(xs[k + 1] / cell - 0.5f));
					for (int i = first; i <= last; i++)
						in[(size_t)j * cols + i] = 1;
				}
			}
			return in;
		}

		// the core of the material (the cell and its 8 neighbours are in the material): the occupancy grid of the greedy algorithm
		void fill_cells(Variant& v, float cell, int stride)
		{
			std::vector<unsigned char> in = rasterize(v, cell, v.cols, v.rows);
			v.core.assign(in.size(), 0);
			v.cells.clear();
			for (int j = 1; j + 1 < v.rows; j++)
				for (int i = 1; i + 1 < v.cols; i++)
				{
					bool all = true;
					for (int dj = -1; dj <= 1 && all; dj++)
						for (int di = -1; di <= 1; di++)
							if (!in[(size_t)(j + dj) * v.cols + i + di])
							{
								all = false;
								break;
							}
					if (all)
					{
						v.core[(size_t)j * v.cols + i] = 1;
						v.cells.push_back(j * stride + i);
					}
				}
			// the cells are tried in any order: the ones which are not near each other first find a taken cell sooner
			std::mt19937 random(7);
			std::shuffle(v.cells.begin(), v.cells.end(), random);
		}

		// The cells of the material as bits, a row after the other (the algorithm which minimizes the overlap counts the cells of two parts
		// which are on each other, 64 at a time), and the same cells with the ones which are at one cell from them. The origin of the bits is
		// one cell before the corner of the bounds. Two parts overlap when the dilated cells of the first are on cells of the second one: the
		// parts which do not overlap are at one cell from each other at least, which leaves room for what the cells do not see.
		void fill_mask(Variant& v, float cell)
		{
			int cols = 0, rows = 0;
			std::vector<unsigned char> in = rasterize(v, cell, cols, rows);
			v.mcols = cols + 2;
			v.mrows = rows + 2;
			v.mwords = (v.mcols + 63) / 64;
			v.mbits.assign((size_t)v.mwords * v.mrows, 0);
			v.dbits.assign((size_t)v.mwords * v.mrows, 0);
			for (int j = 0; j < rows; j++)
				for (int i = 0; i < cols; i++)
					if (in[(size_t)j * cols + i])
					{
						v.mbits[(size_t)(j + 1) * v.mwords + (i + 1) / 64] |= 1ull << ((i + 1) % 64);
						for (int dj = 0; dj <= 2; dj++)
							for (int di = 0; di <= 2; di++)
								v.dbits[(size_t)(j + dj) * v.mwords + (i + di) / 64] |= 1ull << ((i + di) % 64);
					}
		}

		struct Context
		{
			const std::vector<Part>& parts;
			const Options& options;
			std::vector<std::vector<Variant>> variants;
			glm::vec2 lo, hi;
			// the grid of the sheet: its cell, a multiple of which is the step of the search, and its size in cells
			float cell = 1;
			int stride = 0, height = 0;
			// the cell of the masks of the algorithm which minimizes the overlap
			float mask_cell = 1;
			std::vector<float> angles;

			Context(const std::vector<Part>& p, const Options& o) : parts(p), options(o), variants(p.size()),
				lo(o.margin), hi(o.width - o.margin, o.height - o.margin)
			{
				int per_step = std::max(1, (int)std::lround(options.step / 2));
				cell = options.step / per_step;
				stride = (int)std::ceil((hi.x - lo.x) / cell) + 2;
				height = (int)std::ceil((hi.y - lo.y) / cell) + 2;
				angles = options.angles;
				// the algorithm which minimizes the overlap keeps the masks of all the variants: not more than 36 rotations
				if (options.algorithm == Algorithm::Overlap && angles.size() > 36)
				{
					std::vector<float> thinned;
					for (size_t k = 0; k < 36; k++)
						thinned.push_back(angles[k * angles.size() / 36]);
					angles = thinned;
				}
				mask_cell = std::min(1.0f, cell);
				std::vector<Variant*> all;
				for (size_t i = 0; i < parts.size(); i++)
					for (float a : angles)
						variants[i].push_back(make_variant(parts[i], a));
				for (auto& list : variants)
					for (Variant& v : list)
						all.push_back(&v);
				parallel_for(all.size(), [&](size_t k)
				{
					fill_cells(*all[k], cell, stride);
					if (options.algorithm == Algorithm::Overlap)
						fill_mask(*all[k], mask_cell);
				});
			}
		};

		// An occupancy grid keeps the cells of the sheet which are in the material of a part already placed (the core of its material).
		// A part which has a core cell on an occupied cell overlaps for sure: the position is refused without the exact test, which is
		// much longer. The grid is only used for the positions of the search which are on its cells.
		struct Occupancy
		{
			const Context& c;
			std::vector<unsigned char> cells;

			Occupancy(const Context& context) : c(context), cells((size_t)context.stride * context.height, 0) {}

			// the part is at this position: the cells which it covers for sure are taken (its position is not on the cells: a cell of the
			// grid is covered by the two cells of the part which are on it)
			void add(const Variant& v, glm::vec2 position)
			{
				int bx = (int)std::floor((position.x - c.lo.x) / c.cell), by = (int)std::floor((position.y - c.lo.y) / c.cell);
				for (int j = 0; j + 1 < v.rows; j++)
					for (int i = 0; i + 1 < v.cols; i++)
						if (v.core[j * v.cols + i] && v.core[j * v.cols + i + 1] && v.core[(j + 1) * v.cols + i] && v.core[(j + 1) * v.cols + i + 1])
						{
							int x = bx + i + 1, y = by + j + 1;
							if (x >= 0 && x < c.stride && y >= 0 && y < c.height)
								cells[(size_t)y * c.stride + x] = 1;
						}
			}

			// the part at the cell (x, y) has a core cell on a taken cell
			bool hits(const Variant& v, int x, int y) const
			{
				const unsigned char* base = cells.data() + (size_t)y * c.stride + x;
				for (int offset : v.cells)
					if (base[offset])
						return true;
				return false;
			}
		};

		struct Spot
		{
			const Variant* variant = nullptr;
			glm::vec2 position = glm::vec2();
			float score = 1e30f;
		};

		// the best place of one variant of a part among the parts already placed: the first column from the left which has a free
		// position, the lowest one, moved to the lower left with smaller and smaller moves. The variants do not look at each other
		// (a bound shared between them gave another result each time, the threads not being as fast): the result is always the same.
		Spot scan(const Context& c, const Variant& v, const std::vector<Placed>& placed, const Occupancy& occupancy)
		{
			Spot best;
			if (v.size.x > c.hi.x - c.lo.x + EPS || v.size.y > c.hi.y - c.lo.y + EPS)
				return best;
			size_t blocker = 0;
			for (float x = c.lo.x; x + v.size.x <= c.hi.x + EPS; x += c.options.step)
			{
				bool found = false;
				for (float y = c.lo.y; y + v.size.y <= c.hi.y + EPS; y += c.options.step)
				{
					// the position is on the grid: a taken cell refuses it at once
					if (occupancy.hits(v, (int)std::lround((x - c.lo.x) / c.cell), (int)std::lround((y - c.lo.y) / c.cell)))
						continue;
					if (!fits(v, glm::vec2(x, y), placed, blocker))
						continue;
					glm::vec2 p(x, y);
					for (float s = c.options.step * 0.5f; s >= c.options.precision; s *= 0.5f)
					{
						bool moved = true;
						while (moved)
						{
							moved = false;
							glm::vec2 left(p.x - s, p.y), down(p.x, p.y - s);
							if (left.x >= c.lo.x && fits(v, left, placed, blocker)) { p = left; moved = true; }
							else if (down.y >= c.lo.y && fits(v, down, placed, blocker)) { p = down; moved = true; }
						}
					}
					// the right edge: a part is placed where the used length grows the least (measured: 1 to 2 % shorter than the left edge)
					float score = p.x + v.size.x + p.y * 0.001f;
					if (score < best.score)
					{
						best.score = score;
						best.variant = &v;
						best.position = p;
					}
					found = true;
					break;
				}
				if (found)
					break;
			}
			return best;
		}

		// the best place of a part: the variants (the rotations) are looked at on all the cores
		Spot find_spot(const Context& c, int part, const std::vector<Placed>& placed, const Occupancy& occupancy)
		{
			const std::vector<Variant>& variants = c.variants[part];
			std::vector<Spot> spots(variants.size());
			parallel_for(variants.size(), [&](size_t i) { spots[i] = scan(c, variants[i], placed, occupancy); });
			Spot best;
			for (Spot& s : spots)
				if (s.variant != nullptr && s.score < best.score)
					best = s;
			return best;
		}

		// Places the parts in this order. cutoff: a length which has to be beaten, the run ends as soon as it is reached (a part added
		// never shortens the sheet used). Returns false if the run was stopped, or cut.
		bool run(const Context& c, const std::vector<int>& order, std::vector<Placement>& result, Progress* progress, float cutoff, bool report)
		{
			std::vector<Placed> placed;
			Occupancy occupancy(c);
			result.clear();
			float right = 0;
			for (size_t k = 0; k < order.size(); k++)
			{
				if (progress != nullptr && progress->stop)
					return false;
				Spot best = find_spot(c, order[k], placed, occupancy);
				Placement pl;
				pl.part = order[k];
				if (best.variant != nullptr)
				{
					pl.placed = true;
					pl.angle = best.variant->angle;
					pl.offset = best.position + best.variant->offset;
					placed.push_back({ best.variant, best.position, Box{ best.position, best.position + best.variant->size } });
					occupancy.add(*best.variant, best.position);
					right = std::max(right, best.position.x + best.variant->size.x);
				}
				result.push_back(pl);
				if (cutoff > 0 && right >= cutoff)
					return false;
				if (report && progress != nullptr)
					progress->done = (float)(k + 1) / (float)order.size();
			}
			return true;
		}

		// the parts of the quantities, the biggest first: the small ones then fill the holes and the gaps
		std::vector<int> by_area(const std::vector<Part>& parts)
		{
			struct Item { int part; float area; };
			std::vector<Item> items;
			for (size_t i = 0; i < parts.size(); i++)
				for (int q = 0; q < parts[i].quantity; q++)
					items.push_back({ (int)i, area(parts[i]) });
			std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.area > b.area; });
			std::vector<int> order;
			for (const Item& item : items)
				order.push_back(item.part);
			return order;
		}

		// the unplaced parts first (the fewer the better), then the length
		bool better(int missing_a, float length_a, int missing_b, float length_b)
		{
			return missing_a != missing_b ? missing_a < missing_b : length_a < length_b;
		}

		int missing(const std::vector<Placement>& r)
		{
			int n = 0;
			for (const Placement& p : r)
				n += p.placed ? 0 : 1;
			return n;
		}
	}

	namespace
	{
		// ----------------------------------------------------------------------------------------------------------------------------
		// The algorithm which minimizes the overlap. A part is a variant (its rotation) and a place on the grid of the masks. The overlap
		// of two parts is the number of cells of their masks which are on each other, the cost of a part is the sum of its overlaps, each
		// one with the weight of the pair. The weights grow for the pairs which keep on overlapping, so that the parts leave a bad
		// arrangement instead of staying in it.
		// ----------------------------------------------------------------------------------------------------------------------------
		struct Item
		{
			int part = 0, variant = 0;
			glm::vec2 position = glm::vec2();	// the corner of the bounds, exact
			int cx = 0, cy = 0;					// the same, on the grid of the masks
		};

		// the 64 columns of a row of bits from the column start (a column before the row, or after it, is empty)
		inline unsigned long long bits64(const unsigned long long* row, int words, int start)
		{
			if (start <= -64)
				return 0;
			if (start < 0)
				return row[0] << (-start);
			int w = start >> 6, s = start & 63;
			unsigned long long low = w < words ? row[w] >> s : 0;
			unsigned long long high = s != 0 && w + 1 < words ? row[w + 1] << (64 - s) : 0;
			return low | high;
		}

		// the number of cells of the second part which are on the dilated cells of the first one (ax, ay and bx, by: the corners of the masks)
		int overlap_cells(const Variant& a, int ax, int ay, const Variant& b, int bx, int by)
		{
			int x0 = std::max(ax, bx), x1 = std::min(ax + a.mcols, bx + b.mcols);
			int y0 = std::max(ay, by), y1 = std::min(ay + a.mrows, by + b.mrows);
			if (x0 >= x1 || y0 >= y1)
				return 0;
			int total = 0;
			for (int y = y0; y < y1; y++)
			{
				const unsigned long long* ra = &a.dbits[(size_t)(y - ay) * a.mwords];
				const unsigned long long* rb = &b.mbits[(size_t)(y - by) * b.mwords];
				for (int x = x0; x < x1; x += 64)
				{
					int n = std::min(64, x1 - x);
					unsigned long long mask = n == 64 ? ~0ull : (1ull << n) - 1;
					total += std::popcount(bits64(ra, a.mwords, x - ax) & bits64(rb, b.mwords, x - bx) & mask);
				}
			}
			return total;
		}

		// the parts and the sheet of one try: the length of the sheet, the weights of the pairs, the random numbers
		struct Packing
		{
			const Context& c;
			float cell;
			float length;
			std::vector<Item> items;
			std::vector<float> weight;
			std::mt19937& random;

			Packing(const Context& context, float cell_size, float sheet_length, std::vector<Item> parts, std::mt19937& generator)
				: c(context), cell(cell_size), length(sheet_length), items(std::move(parts)), random(generator)
			{
				weight.assign(items.size() * items.size(), 1.0f);
			}

			const Variant& variant(const Item& item) const { return c.variants[item.part][item.variant]; }

			// the cells of a part which can be at the left of the sheet, and at the bottom: the part must be inside of the sheet
			bool fits_sheet(const Variant& v) const
			{
				return v.size.x <= length - c.lo.x + EPS && v.size.y <= c.hi.y - c.lo.y + EPS;
			}
			int max_cx(const Variant& v) const { return std::max(0, (int)std::floor((length - c.lo.x - v.size.x) / cell + 1e-4f)); }
			int max_cy(const Variant& v) const { return std::max(0, (int)std::floor((c.hi.y - c.lo.y - v.size.y) / cell + 1e-4f)); }

			// the weighted overlap of the part i if it was at this place; the sum stops as soon as it is not smaller than the cutoff
			double value(int i, const Variant& v, int cx, int cy, double cutoff) const
			{
				const int n = (int)items.size();
				double sum = 0;
				for (int j = 0; j < n; j++)
				{
					if (j == i)
						continue;
					const Item& other = items[j];
					const Variant& ov = variant(other);
					if (cx >= other.cx + ov.mcols || other.cx >= cx + v.mcols || cy >= other.cy + ov.mrows || other.cy >= cy + v.mrows)
						continue;
					int count = overlap_cells(v, cx - 1, cy - 1, ov, other.cx - 1, other.cy - 1);
					if (count != 0)
					{
						sum += weight[(size_t)i * n + j] * count;
						if (sum >= cutoff)
							return sum;
					}
				}
				return sum;
			}

			void put(int i, int variant_index, int cx, int cy)
			{
				Item& item = items[i];
				item.variant = variant_index;
				item.cx = cx;
				item.cy = cy;
				item.position = c.lo + glm::vec2(cx * cell, cy * cell);
			}

			// The part i goes to the place where it overlaps the least: places taken at random in the sheet with any rotation, places
			// near the one it has with the same rotation, then the best ones are improved with moves of smaller and smaller size.
			void relocate(int i)
			{
				const Item& item = items[i];
				const std::vector<Variant>& variants = c.variants[item.part];
				const double current = value(i, variant(item), item.cx, item.cy, 1e300);
				if (current <= 0)
					return;

				struct Candidate { int variant, cx, cy; double value; };
				std::vector<Candidate> candidates;
				auto consider = [&](int vi, int cx, int cy)
				{
					const Variant& v = variants[vi];
					if (v.mcols == 0 || !fits_sheet(v))
						return;
					cx = std::clamp(cx, 0, max_cx(v));
					cy = std::clamp(cy, 0, max_cy(v));
					candidates.push_back({ vi, cx, cy, value(i, v, cx, cy, current * 4) });
				};
				const int samples = 30;
				for (int k = 0; k < samples; k++)
				{
					int vi = (int)(random() % variants.size());
					const Variant& v = variants[vi];
					consider(vi, (int)(random() % (max_cx(v) + 1)), (int)(random() % (max_cy(v) + 1)));
				}
				for (int k = 0; k < samples; k++)
					consider(item.variant, item.cx + (int)(random() % 41) - 20, item.cy + (int)(random() % 41) - 20);
				candidates.push_back({ item.variant, item.cx, item.cy, current });

				std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) { return a.value < b.value; });
				if (candidates.size() > 3)
					candidates.resize(3);

				Candidate best = candidates[0];
				for (Candidate cd : candidates)
				{
					const Variant& v = variants[cd.variant];
					for (int step = 16; step >= 1; step /= 2)
					{
						bool improved = true;
						for (int guard = 0; improved && guard < 40 && cd.value > 0; guard++)
						{
							improved = false;
							for (int d = 0; d < 8; d++)
							{
								static const int dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
								int nx = std::clamp(cd.cx + dx[d] * step, 0, max_cx(v)), ny = std::clamp(cd.cy + dy[d] * step, 0, max_cy(v));
								if (nx == cd.cx && ny == cd.cy)
									continue;
								double val = value(i, v, nx, ny, cd.value);
								if (val < cd.value)
								{
									cd = { cd.variant, nx, ny, val };
									improved = true;
								}
							}
						}
					}
					if (cd.value < best.value)
						best = cd;
				}
				if (best.value < current)
					put(i, best.variant, best.cx, best.cy);
			}

			struct Pair { int a, b, count; };

			// the pairs of parts which overlap, and the total
			std::vector<Pair> overlaps(long long& total) const
			{
				const int n = (int)items.size();
				std::vector<Pair> pairs;
				total = 0;
				for (int i = 0; i < n; i++)
				{
					const Variant& vi = variant(items[i]);
					for (int j = i + 1; j < n; j++)
					{
						const Item& b = items[j];
						const Variant& vj = variant(b);
						if (items[i].cx >= b.cx + vj.mcols || b.cx >= items[i].cx + vi.mcols || items[i].cy >= b.cy + vj.mrows || b.cy >= items[i].cy + vi.mrows)
							continue;
						int count = overlap_cells(vi, items[i].cx - 1, items[i].cy - 1, vj, b.cx - 1, b.cy - 1);
						if (count != 0)
						{
							pairs.push_back({ i, j, count });
							total += count;
						}
					}
				}
				return pairs;
			}

			// The parts which overlap are moved, one after the other, then the pairs which still overlap weigh more. It ends when nothing
			// overlaps, or when it does not improve any more, or when the time is over.
			bool separate(std::chrono::steady_clock::time_point deadline, Progress* progress)
			{
				const int n = (int)items.size();
				long long best_total = std::numeric_limits<long long>::max();
				int stale = 0;
				for (;;)
				{
					if ((progress != nullptr && progress->stop) || std::chrono::steady_clock::now() > deadline)
						return false;
					long long total = 0;
					std::vector<Pair> pairs = overlaps(total);
					if (pairs.empty())
						return true;
					if (total < best_total - best_total / 50)
					{
						best_total = total;
						stale = 0;
					}
					else if (++stale > 40)
						return false;

					std::vector<int> moving;
					for (const Pair& p : pairs)
					{
						moving.push_back(p.a);
						moving.push_back(p.b);
					}
					std::sort(moving.begin(), moving.end());
					moving.erase(std::unique(moving.begin(), moving.end()), moving.end());
					std::shuffle(moving.begin(), moving.end(), random);
					for (int i : moving)
						relocate(i);

					pairs = overlaps(total);
					int biggest = 1;
					for (const Pair& p : pairs)
						biggest = std::max(biggest, p.count);
					for (float& w : weight)
						w = std::max(1.0f, w * 0.97f);
					for (const Pair& p : pairs)
					{
						float grow = 1.3f + (float)p.count / (float)biggest;
						weight[(size_t)p.a * n + p.b] *= grow;
						weight[(size_t)p.b * n + p.a] *= grow;
					}
				}
			}
		};

		// the parts as they are placed: a part, its variant and its exact place
		std::vector<Item> items_of(const Context& c, const std::vector<Placement>& result, float cell)
		{
			std::vector<Item> items;
			for (const Placement& p : result)
			{
				Item item;
				item.part = p.part;
				const std::vector<Variant>& variants = c.variants[p.part];
				for (size_t v = 0; v < variants.size(); v++)
					if (variants[v].angle == p.angle)
						item.variant = (int)v;
				item.position = p.offset - variants[item.variant].offset;
				item.cx = (int)std::lround((item.position.x - c.lo.x) / cell);
				item.cy = (int)std::lround((item.position.y - c.lo.y) / cell);
				items.push_back(item);
			}
			return items;
		}

		std::vector<Placement> result_of(const Context& c, const std::vector<Item>& items)
		{
			std::vector<Placement> result;
			for (const Item& item : items)
			{
				const Variant& v = c.variants[item.part][item.variant];
				Placement p;
				p.part = item.part;
				p.placed = true;
				p.angle = v.angle;
				p.offset = item.position + v.offset;
				result.push_back(p);
			}
			return result;
		}

		float length_of(const Context& c, const std::vector<Item>& items)
		{
			float right = 0;
			for (const Item& item : items)
				right = std::max(right, item.position.x + c.variants[item.part][item.variant].size.x);
			return right;
		}

		// The cells of the masks are not the polygons: two parts which do not overlap on the grid can overlap a little. Each part is looked
		// at with the exact test; the one which overlaps is moved by small steps in every direction to the nearest place which is free.
		bool make_exact(const Context& c, std::vector<Item>& items, float length, float cell)
		{
			size_t blocker = 0;
			for (size_t i = 0; i < items.size(); i++)
			{
				std::vector<Placed> others;
				for (size_t j = 0; j < items.size(); j++)
					if (j != i)
					{
						const Variant& v = c.variants[items[j].part][items[j].variant];
						others.push_back({ &v, items[j].position, Box{ items[j].position, items[j].position + v.size } });
					}
				const Variant& v = c.variants[items[i].part][items[i].variant];
				if (fits(v, items[i].position, others, blocker))
					continue;
				bool repaired = false;
				for (float radius = cell * 0.25f; radius <= cell * 4 && !repaired; radius += cell * 0.25f)
					for (int d = 0; d < 8 && !repaired; d++)
					{
						static const float dx[8] = { 1, -1, 0, 0, 0.7071f, 0.7071f, -0.7071f, -0.7071f }, dy[8] = { 0, 0, 1, -1, 0.7071f, -0.7071f, 0.7071f, -0.7071f };
						glm::vec2 p = items[i].position + glm::vec2(dx[d], dy[d]) * radius;
						if (p.x < c.lo.x || p.y < c.lo.y || p.x + v.size.x > length + EPS || p.y + v.size.y > c.hi.y + EPS)
							continue;
						if (fits(v, p, others, blocker))
						{
							items[i].position = p;
							repaired = true;
						}
					}
				if (!repaired)
					return false;
			}
			return true;
		}

		// The parts are moved to the left and down while they can (exact test): what the grid left of room between them is taken back.
		void compact(const Context& c, std::vector<Item>& items)
		{
			std::vector<size_t> order(items.size());
			for (size_t i = 0; i < order.size(); i++)
				order[i] = i;
			std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return items[a].position.x + c.variants[items[a].part][items[a].variant].size.x > items[b].position.x + c.variants[items[b].part][items[b].variant].size.x; });
			size_t blocker = 0;
			for (size_t i : order)
			{
				std::vector<Placed> others;
				for (size_t j = 0; j < items.size(); j++)
					if (j != i)
					{
						const Variant& v = c.variants[items[j].part][items[j].variant];
						others.push_back({ &v, items[j].position, Box{ items[j].position, items[j].position + v.size } });
					}
				const Variant& v = c.variants[items[i].part][items[i].variant];
				glm::vec2 p = items[i].position;
				for (float s = c.options.step * 0.5f; s >= c.options.precision; s *= 0.5f)
				{
					bool moved = true;
					while (moved)
					{
						moved = false;
						glm::vec2 left(p.x - s, p.y), down(p.x, p.y - s);
						if (left.x >= c.lo.x && fits(v, left, others, blocker)) { p = left; moved = true; }
						else if (down.y >= c.lo.y && fits(v, down, others, blocker)) { p = down; moved = true; }
					}
				}
				items[i].position = p;
			}
		}

		// The result of the greedy algorithm is made shorter by the tries of every core: a try shortens the sheet, separates the parts and
		// keeps the arrangement if it is exact and shorter.
		std::vector<Placement> minimize_overlap(const Context& c, const std::vector<Placement>& first, Progress* progress)
		{
			if (missing(first) != 0 || first.size() < 2)
				return first;
			const float cell = c.mask_cell;
			std::mutex lock;
			std::vector<Item> best = items_of(c, first, cell);
			float best_length = length_of(c, best);
			if (progress != nullptr)
				progress->length = best_length;
			const auto begin = std::chrono::steady_clock::now();
			const auto deadline = begin + std::chrono::milliseconds((long long)(c.options.time * 1000));
			auto elapsed = [&]() { return std::chrono::duration<float>(std::chrono::steady_clock::now() - begin).count(); };
			std::atomic<int> tries{ 0 };

			parallel_for(std::max(1u, std::thread::hardware_concurrency()), [&](size_t worker)
			{
				std::mt19937 random((unsigned)(777 + worker * 104729));
				while (std::chrono::steady_clock::now() < deadline && !(progress != nullptr && progress->stop))
				{
					std::vector<Item> start;
					float from = 0;
					{
						std::lock_guard<std::mutex> guard(lock);
						start = best;
						from = best_length;
					}
					// From 2 % longer to 1 % shorter (measured: the tries which are only longer do not find anything, the ones which are only
					// shorter almost never work). The parts are on the grid, with a space between them: once they are separated, they are
					// brought back together by the exact test, which is where the length is won.
					float shrink = -0.02f + 0.03f * (float)(random() % 1000) / 1000.0f;
					float target = from * (1 - shrink);
					if (target <= c.lo.x)
						break;

					Packing packing(c, cell, target, start, random);
					// every part on the grid, the ones which go out of the sheet are put against its end
					for (int i = 0; i < (int)packing.items.size(); i++)
					{
						const Variant& v = packing.variant(packing.items[i]);
						int cx = std::clamp((int)std::lround((packing.items[i].position.x - c.lo.x) / cell), 0, packing.max_cx(v));
						int cy = std::clamp((int)std::lround((packing.items[i].position.y - c.lo.y) / cell), 0, packing.max_cy(v));
						packing.put(i, packing.items[i].variant, cx, cy);
					}

					bool done = packing.separate(deadline, progress);
					tries++;
					if (progress != nullptr)
					{
						progress->tries = tries.load();
						progress->done = std::min(1.0f, elapsed() / c.options.time);
					}
					if (!done || !make_exact(c, packing.items, target, cell))
						continue;
					compact(c, packing.items);
					float length = length_of(c, packing.items);
					std::lock_guard<std::mutex> guard(lock);
					if (length < best_length - 1e-3f)
					{
						best = packing.items;
						best_length = length;
						if (progress != nullptr)
							progress->length = length;
					}
				}
			});
			if (progress != nullptr)
				progress->done = 1;
			return result_of(c, best);
		}
	}

	std::vector<Placement> nest(const std::vector<Part>& parts, const Options& options, Progress* progress)
	{
		Context c(parts, options);
		std::vector<int> order = by_area(parts);

		std::vector<Placement> first;
		if (!run(c, order, first, progress, 0, options.algorithm == Algorithm::Greedy))
		{
			// stopped: the parts which are not done are not placed
			for (size_t k = first.size(); k < order.size(); k++)
			{
				Placement pl;
				pl.part = order[k];
				first.push_back(pl);
			}
			return first;
		}
		float best_length = length(parts, first);
		if (progress != nullptr)
			progress->length = best_length;
		if (options.algorithm == Algorithm::Overlap)
			return minimize_overlap(c, first, progress);
		if (options.algorithm != Algorithm::Search || order.size() < 2)
			return first;

		// The search: the best order so far is changed a little (two parts exchanged, one part moved, a few parts reversed) and the
		// parts are placed again; the order which gives the shortest sheet is kept. Every core works on its own tries.
		std::mutex lock;
		std::vector<int> best_order = order;
		std::vector<Placement> best = first;
		int best_missing = missing(first);
		const auto begin = std::chrono::steady_clock::now();
		auto elapsed = [&]() { return std::chrono::duration<float>(std::chrono::steady_clock::now() - begin).count(); };
		std::atomic<int> tries{ 0 };

		parallel_for(std::max(1u, std::thread::hardware_concurrency()), [&](size_t worker)
		{
			std::mt19937 random((unsigned)(12345 + worker * 7919));
			while (elapsed() < options.time && !(progress != nullptr && progress->stop))
			{
				std::vector<int> candidate;
				float cutoff = 0;
				{
					std::lock_guard<std::mutex> guard(lock);
					candidate = best_order;
					// a try can end as soon as it is not shorter, when the best result has all its parts
					cutoff = best_missing == 0 ? best_length : 0;
				}
				const int n = (int)candidate.size();
				auto position = [&]() { return (int)(random() % n); };
				// most of the changes are near each other: the big parts keep their place
				auto near = [&](int i) { return std::clamp(i + (int)(random() % 21) - 10, 0, n - 1); };
				switch (random() % 3)
				{
				case 0:
				{
					int i = position();
					std::swap(candidate[i], candidate[near(i)]);
					break;
				}
				case 1:
				{
					int i = position(), j = near(i);
					int part = candidate[i];
					candidate.erase(candidate.begin() + i);
					candidate.insert(candidate.begin() + j, part);
					break;
				}
				default:
				{
					int i = position(), j = near(i);
					std::reverse(candidate.begin() + std::min(i, j), candidate.begin() + std::max(i, j) + 1);
				}
				}

				std::vector<Placement> result;
				bool complete = run(c, candidate, result, progress, cutoff, false);
				tries++;
				if (progress != nullptr)
				{
					progress->tries = tries.load();
					progress->done = std::min(1.0f, elapsed() / options.time);
				}
				if (!complete)
					continue;
				float len = length(parts, result);
				int miss = missing(result);
				std::lock_guard<std::mutex> guard(lock);
				if (better(miss, len, best_missing, best_length))
				{
					best_order = candidate;
					best = result;
					best_missing = miss;
					best_length = len;
					if (progress != nullptr)
						progress->length = len;
				}
			}
		});
		if (progress != nullptr)
			progress->done = 1;
		return best;
	}

	float length(const std::vector<Part>& parts, const std::vector<Placement>& placements)
	{
		float right = 0;
		for (const Placement& p : placements)
			if (p.placed)
				for (glm::vec2 q : transform(parts[p.part].outer, p.angle, p.offset))
					right = std::max(right, q.x);
		return right;
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
