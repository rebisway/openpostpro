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

#include "sim_model_tiles.h"
#include "sim_material.h"
#include "sim_footprint.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <limits>

static const int LIST_WIDTH = 1024;			// the lists of tiles are in a texture of this width
static const int ATLAS_WIDTH = 16384;		// the width of the atlas, in heights
static const int MAX_SLOTS = 240000;		// a place is 4.3 KB on the graphic card (a texture is 16384 high at most: 484 rows of 496 places)
static const int INITIAL_SLOTS = 2048;		// the atlas starts small and grows where the tool goes
// the whole grid (if every tile had its heights) may have this number of times the cells of the budget when the tool leaves plateaus
// (the memory is only used where the tool goes); a tool with a round or a conical point leaves no plateau, the budget is the one of the grid
static const int VIRTUAL_FACTOR = 4;

SimModelTiles::~SimModelTiles()
{
	release_graphics();
}

void SimModelTiles::release_graphics()
{
	if (_render != nullptr)
	{
		if (_info) _render->delete_texture(_info);
		if (_atlas) _render->delete_texture(_atlas);
		if (_flat_list) _render->delete_texture(_flat_list);
		if (_detail_list) _render->delete_texture(_detail_list);
	}
	_info = _atlas = _flat_list = _detail_list = nullptr;
}

void SimModelTiles::setup(const SimBlock& block)
{
	_step = std::max(block.tool_radius * block.precision / 100.0, 0.01);
	const double span_x = std::max(block.high.x - block.low.x, _step), span_y = std::max(block.high.y - block.low.y, _step);
	double cells = (span_x / _step + 1.0) * (span_y / _step + 1.0);
	// every tile can have its heights: the grid is limited by the number of places of the atlas as well
	double budget = (double)MAX_SLOTS * T * T;
	if (block.max_cells > 0)
		budget = std::min(budget, (double)block.max_cells * (block.plateaus ? VIRTUAL_FACTOR : 1));
	if (cells > budget)
		_step *= std::sqrt(cells / budget);

	_origin_x = block.low.x;
	_origin_y = block.low.y;
	_top = block.top;
	_bottom = std::min(block.bottom, block.top);
	_through = block.through;
	_ntx = std::max(1, (int)std::ceil(span_x / _step / T));
	_nty = std::max(1, (int)std::ceil(span_y / _step / T));
	_width = _ntx * T + 1;
	_height = _nty * T + 1;
	_tiles.assign((size_t)_ntx * _nty, Tile());
	// a place in the atlas for each tile: no tile stays without its heights on the graphic card
	_slots_limit = (int)std::min<size_t>(_tiles.size(), MAX_SLOTS);
	_slots = std::min(_slots_limit, INITIAL_SLOTS);
	_grow = false;
	_warned = false;
	reset();
	// the graphic resources have the size of the old block: they are made again by create_graphics
	release_graphics();
}

void SimModelTiles::reset()
{
	for (Tile& t : _tiles)
	{
		t.data.clear();
		t.data.shrink_to_fit();
		t.flat = (float)_top;
		t.slot = -1;
		t.dirty = false;
	}
	_dirty.clear();
	_detailed = 0;
	_full = true;
	_lists_dirty = true;
}

std::string SimModelTiles::describe() const
{
	char text[160];
	snprintf(text, sizeof(text), "%d x %d tiles of %d x %d, step %.4f mm, %zu with detail (%.1f %%)%s", _ntx, _nty, T, T, _step, _detailed,
		_tiles.empty() ? 0.0 : 100.0 * _detailed / _tiles.size(), _warned ? ", ATLAS FULL: more cells needed" : "");
	return text;
}

double SimModelTiles::height_at(double x, double y) const
{
	if (_tiles.empty())
		return _top;
	int ix = std::clamp((int)std::lround((x - _origin_x) / _step), 0, _width - 1);
	int iy = std::clamp((int)std::lround((y - _origin_y) / _step), 0, _height - 1);
	int tx = std::min(ix / T, _ntx - 1), ty = std::min(iy / T, _nty - 1);
	const Tile& t = _tiles[(size_t)ty * _ntx + tx];
	if (t.data.empty())
		return t.flat;
	return t.data[(size_t)(iy - ty * T) * V + (ix - tx * T)];
}

double SimModelTiles::removed_volume() const
{
	// every column is counted once: the cells of the tiles, the last row and the last column of heights are the borders of the grid
	double sum = 0;
	for (const Tile& t : _tiles)
	{
		if (t.data.empty())
			sum += (_top - t.flat) * T * T;
		else
			for (int y = 0; y < T; y++)
				for (int x = 0; x < T; x++)
					sum += _top - t.data[(size_t)y * V + x];
	}
	return sum * _step * _step;
}

void SimModelTiles::mark_dirty(int index)
{
	Tile& t = _tiles[index];
	if (!t.dirty)
	{
		t.dirty = true;
		_dirty.push_back(index);
	}
}

void SimModelTiles::cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool)
{
	if (_tiles.empty())
		return;

	// long moves are cut by pieces, so that the zone looked at for each piece stays small
	double length = glm::distance(glm::dvec2(a), glm::dvec2(b));
	double piece = std::max(tool.radius, 6.0 * _step);
	// a flat tool at a constant height does not compute anything for each column: the pieces can be long
	if ((tool.shape == SimToolShape::Flat || tool.shape == SimToolShape::Plasma) && std::fabs(b.z - a.z) < 1e-12)
		piece *= 8.0;
	int count = std::max(1, (int)std::ceil(length / piece));
	for (int i = 0; i < count; i++)
	{
		glm::dvec3 p0 = a + (b - a) * ((double)i / count);
		glm::dvec3 p1 = a + (b - a) * ((double)(i + 1) / count);
		cut_piece(p0, p1, tool);
	}
}

void SimModelTiles::cut_piece(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool)
{
	const double r = tool.radius;
	if (r <= 0)
		return;

	double low_x = std::min(a.x, b.x) - r, high_x = std::max(a.x, b.x) + r;
	double low_y = std::min(a.y, b.y) - r, high_y = std::max(a.y, b.y) + r;
	int x0 = std::max(0, (int)std::floor((low_x - _origin_x) / _step));
	int x1 = std::min(_width - 1, (int)std::ceil((high_x - _origin_x) / _step));
	int y0 = std::max(0, (int)std::floor((low_y - _origin_y) / _step));
	int y1 = std::min(_height - 1, (int)std::ceil((high_y - _origin_y) / _step));
	if (x0 > x1 || y0 > y1)
		return;

	// the columns under the tool: for each row of columns, the first and the last one (nothing if first > last)
	const glm::dvec2 a2(a.x, a.y), b2(b.x, b.y);
	_row_first.assign((size_t)(y1 - y0 + 1), 1);
	_row_last.assign((size_t)(y1 - y0 + 1), 0);
	for (int iy = y0; iy <= y1; iy++)
	{
		double x_low, x_high;
		if (footprint_row(a2, b2, r, _origin_y + iy * _step, x_low, x_high))
		{
			_row_first[iy - y0] = std::max(x0, (int)std::ceil((x_low - _origin_x) / _step));
			_row_last[iy - y0] = std::min(x1, (int)std::floor((x_high - _origin_x) / _step));
		}
	}

	// the tiles which have a height of the zone (a tile has the heights from its first column to the first one of the next tile)
	const int tx0 = std::max(0, (x0 + T - 1) / T - 1), tx1 = std::min(_ntx - 1, x1 / T);
	const int ty0 = std::max(0, (y0 + T - 1) / T - 1), ty1 = std::min(_nty - 1, y1 / T);

	glm::dvec2 d(b.x - a.x, b.y - a.y);
	double l2 = glm::dot(d, d);
	bool through = tool.shape == SimToolShape::Plasma;
	// the profile of a flat tool does not depend on the distance: no need of the square root
	bool flat_tool = tool.shape == SimToolShape::Flat || tool.shape == SimToolShape::Plasma;
	// a move at the same height has one candidate, a plunge or a ramp has its two ends too
	bool level = std::fabs(b.z - a.z) < 1e-12;
	// a flat tool which does not change its height cuts every column under it at the same height: nothing to compute
	const bool constant = flat_tool && level;
	double r2 = r * r;
	float bottom = (float)_bottom;
	const float constant_height = through ? bottom : (float)std::max(a.z, _bottom);
	double lowest = std::min(a.z, b.z);

	for (int ty = ty0; ty <= ty1; ty++)
	{
		for (int tx = tx0; tx <= tx1; tx++)
		{
			const int index = ty * _ntx + tx;
			Tile& tile = _tiles[index];
			// a plateau which is under the tool cannot be cut
			if (!through && tile.data.empty() && tile.flat <= lowest)
				continue;

			const int bx = tx * T, by = ty * T;
			const int ly0 = std::max(y0, by) - by, ly1 = std::min(y1, by + T) - by;
			bool changed = false;

			for (int ly = ly0; ly <= ly1; ly++)
			{
				// the columns of this tile on the row which are under the tool
				const int lx0 = std::max(_row_first[by + ly - y0], bx) - bx, lx1 = std::min(_row_last[by + ly - y0], bx + T) - bx;
				if (lx0 > lx1)
					continue;
				double py = _origin_y + (by + ly) * _step;
				for (int lx = lx0; lx <= lx1; lx++)
				{
					float h = constant_height;
					if (!constant)
					{
						double px = _origin_x + (bx + lx) * _step;

						// the nearest point of the move, and its two ends (a plunge only has the lowest end)
						double t = l2 < 1e-18 ? 0.0 : std::clamp(((px - a.x) * d.x + (py - a.y) * d.y) / l2, 0.0, 1.0);
						double best = std::numeric_limits<double>::max();
						bool touched = false;
						int candidates = level ? 1 : 3;
						double ts[3] = { t, 0.0, 1.0 };
						for (int k = 0; k < candidates; k++)
						{
							double tt = ts[k];
							double ex = px - (a.x + d.x * tt), ey = py - (a.y + d.y * tt);
							double dist2 = ex * ex + ey * ey;
							if (dist2 > r2)
								continue;
							touched = true;
							double z = a.z + (b.z - a.z) * tt;
							best = std::min(best, flat_tool ? z : z + SimMaterial::profile(tool, std::sqrt(dist2)));
						}
						if (!touched)
							continue;
						h = through ? bottom : (float)std::max(best, _bottom);
					}

					float current = tile.data.empty() ? tile.flat : tile.data[(size_t)ly * V + lx];
					if (h < current)
					{
						if (tile.data.empty())
						{
							tile.data.assign((size_t)V * V, tile.flat);
							_detailed++;
						}
						tile.data[(size_t)ly * V + lx] = h;
						changed = true;
					}
				}
			}
			if (changed)
				mark_dirty(index);
		}
	}
}

// ------------------------------------------------------------------------------------------------------ graphics
void SimModelTiles::create_graphics(Renderer* render)
{
	release_graphics();
	_render = render;
	if (_tiles.empty())
		return;

	_atlas_cols = ATLAS_WIDTH / V;
	const int list_rows = ((int)_tiles.size() + LIST_WIDTH - 1) / LIST_WIDTH;
	_info = _render->create_float_texture(_ntx, _nty, nullptr);
	make_atlas();
	_flat_list = _render->create_float_texture(LIST_WIDTH, list_rows, nullptr);
	_detail_list = _render->create_float_texture(LIST_WIDTH, list_rows, nullptr);
	_info_data.assign(_tiles.size(), 0.0f);
	_info_first = 0;
	_info_last = _nty - 1;
	_pending.clear();
	_full = true;
	_lists_dirty = true;
}

void SimModelTiles::make_atlas()
{
	if (_atlas)
		_render->delete_texture(_atlas);
	const int rows = (_slots + _atlas_cols - 1) / _atlas_cols;
	_atlas = _render->create_float_texture(_atlas_cols * V, rows * V, nullptr);
}

// The atlas is full: it has twice the places (up to a place for each tile) and everything is sent again
void SimModelTiles::grow_atlas()
{
	_grow = false;
	_slots = (int)std::min<long long>((long long)_slots * 2, _slots_limit);
	make_atlas();
	_pending.clear();
	_full = true;
}

void SimModelTiles::reset_slots()
{
	_free.clear();
	for (int s = _slots - 1; s >= 0; s--)
		_free.push_back(s);
	for (Tile& t : _tiles)
		t.slot = -1;
}

// A tile which changed: a plateau is simplified, a tile with detail has its place in the atlas and its heights are sent
void SimModelTiles::settle(int index)
{
	Tile& t = _tiles[index];
	t.dirty = false;

	if (!t.data.empty())
	{
		auto range = std::minmax_element(t.data.begin(), t.data.end());
		if (*range.second - *range.first < 1e-6f)
		{
			t.flat = *range.first;
			t.data.clear();
			t.data.shrink_to_fit();
			_detailed--;
		}
	}
	if (_render == nullptr || _info == nullptr)
		return;

	const int tx = index % _ntx, ty = index / _ntx;
	float info;
	if (t.data.empty())
	{
		if (t.slot >= 0)
		{
			_free.push_back(t.slot);
			t.slot = -1;
			_lists_dirty = true;
		}
		info = t.flat;
	}
	else
	{
		if (t.slot < 0)
		{
			if (_free.empty())
			{
				// no place left: the tile stays as it was for the graphic card (it is tried again at its next change)
				// the atlas grows (at the end of the upload) if it can: the tile is sent again then
				if (_slots < _slots_limit)
					_grow = true;
				else
					_warned = true;
				return;
			}
			t.slot = _free.back();
			_free.pop_back();
			_lists_dirty = true;
		}
		_pending.push_back(index);
		info = 1.0e6f + (float)t.slot;
	}
	_info_data[index] = info;
	_info_first = _info_last < _info_first ? ty : std::min(_info_first, ty);
	_info_last = std::max(_info_last, ty);
}

// What changed is sent in a few calls: the rows of tile_info, and the tiles which are side by side in the atlas together
void SimModelTiles::flush()
{
	if (_info_last >= _info_first)
		_render->update_float_texture(_info, 0, _info_first, _ntx, _info_last - _info_first + 1, &_info_data[(size_t)_info_first * _ntx], _ntx);
	_info_first = 0;
	_info_last = -1;

	std::sort(_pending.begin(), _pending.end(), [&](int a, int b) { return _tiles[a].slot < _tiles[b].slot; });
	size_t i = 0;
	while (i < _pending.size())
	{
		// a run: the next slots in the same row of the atlas
		const int first = _tiles[_pending[i]].slot;
		size_t j = i + 1;
		while (j < _pending.size() && _tiles[_pending[j]].slot == first + (int)(j - i) && (_tiles[_pending[j]].slot % _atlas_cols) != 0)
			j++;
		const int count = (int)(j - i);
		const int row_length = count * V;
		_staging.resize((size_t)row_length * V);
		for (int k = 0; k < count; k++)
		{
			const std::vector<float>& data = _tiles[_pending[i + k]].data;
			for (int y = 0; y < V; y++)
				std::copy(data.begin() + (size_t)y * V, data.begin() + (size_t)(y + 1) * V, _staging.begin() + (size_t)y * row_length + (size_t)k * V);
		}
		_render->update_float_texture(_atlas, (first % _atlas_cols) * V, (first / _atlas_cols) * V, row_length, V, _staging.data(), row_length);
		i = j;
	}
	_pending.clear();
}

void SimModelTiles::rebuild_lists()
{
	const int list_rows = ((int)_tiles.size() + LIST_WIDTH - 1) / LIST_WIDTH;
	std::vector<float> flat((size_t)list_rows * LIST_WIDTH, 0.0f), detail((size_t)list_rows * LIST_WIDTH, 0.0f);
	_flat_count = _detail_count = 0;
	for (int i = 0; i < (int)_tiles.size(); i++)
	{
		if (_tiles[i].slot >= 0)
			detail[_detail_count++] = (float)i;
		else
			flat[_flat_count++] = (float)i;
	}
	_render->update_float_texture(_flat_list, 0, 0, LIST_WIDTH, list_rows, flat.data(), LIST_WIDTH);
	_render->update_float_texture(_detail_list, 0, 0, LIST_WIDTH, list_rows, detail.data(), LIST_WIDTH);
	_lists_dirty = false;
}

void SimModelTiles::upload()
{
	const bool graphics = _render != nullptr && _info != nullptr;
	if (graphics && _full)
	{
		reset_slots();
		for (int i = 0; i < (int)_tiles.size(); i++)
			mark_dirty(i);
		_full = false;
		_lists_dirty = true;
	}

	std::vector<int> changed;
	changed.swap(_dirty);
	for (int index : changed)
		settle(index);

	if (graphics)
		flush();
	if (graphics && _lists_dirty)
		rebuild_lists();

	// a larger atlas: everything is sent again (the tiles are in memory), as many times as the atlas has to double
	if (graphics && _grow)
	{
		grow_atlas();
		upload();
	}
}

void SimModelTiles::draw(const SimLook& look)
{
	if (_info == nullptr || _tiles.empty())
		return;
	_render->use_program("simtile");
	_render->set_uniform("projection", look.projection);
	_render->set_uniform("modelview", look.modelview);
	_render->set_uniform("eye", look.eye);
	_render->set_uniform("top_color", look.top_color);
	_render->set_uniform("cut_color", look.cut_color);
	_render->set_uniform("side_color", look.side_color);
	_render->set_uniform("tiles_x", _ntx);
	_render->set_uniform("tiles_y", _nty);
	_render->set_uniform("grid_w", _width);
	_render->set_uniform("grid_h", _height);
	_render->set_uniform("atlas_cols", _atlas_cols);
	_render->set_uniform("origin", glm::vec2((float)_origin_x, (float)_origin_y));
	_render->set_uniform("step", (float)_step);
	_render->set_uniform("top", (float)_top);
	_render->set_uniform("bottom", (float)_bottom);
	_render->bind_texture(_info, 0);
	_render->set_uniform("tile_info", 0);
	_render->bind_texture(_atlas, 1);
	_render->set_uniform("atlas", 1);
	_render->bind_texture(_flat_list, 2);
	_render->set_uniform("flat_list", 2);
	_render->bind_texture(_detail_list, 3);
	_render->set_uniform("detail_list", 3);

	if (_flat_count > 0)
	{
		_render->set_uniform("part", 0);
		_render->draw_arrays(_render->pr_triangles(), _flat_count * 6);
	}
	if (_detail_count > 0)
	{
		_render->set_uniform("part", 6);
		_render->draw_arrays(_render->pr_triangles(), _detail_count * T * T * 6);
	}
	_render->set_uniform("part", 1);
	_render->draw_arrays(_render->pr_triangle_strip(), _width * 2);
	_render->set_uniform("part", 2);
	_render->draw_arrays(_render->pr_triangle_strip(), _width * 2);
	_render->set_uniform("part", 3);
	_render->draw_arrays(_render->pr_triangle_strip(), _height * 2);
	_render->set_uniform("part", 4);
	_render->draw_arrays(_render->pr_triangle_strip(), _height * 2);
	if (!_through)
	{
		_render->set_uniform("part", 5);
		_render->draw_arrays(_render->pr_triangles(), 6);
	}
	for (int unit = 3; unit >= 0; unit--)
		_render->bind_texture(nullptr, unit);
}
