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

// The second model: a grid of heights cut in tiles. A tile which was not touched, or which is a plateau (all its heights are the
// same), is only one number. The tool makes the heights of the tiles it goes through: the detail exists where the material was cut
// and changes, a plateau is simplified as soon as it is complete.
//
// The tiles are T cells on a side and have their own copy of the heights of their borders (T + 1 on a side), so the tool updates
// every tile which has the column it cuts and the tiles are independent of each other. The grid of the block is made of whole
// tiles: it can be larger than the block by less than a tile.
//
// The graphic card has the tiles of the atlas (the heights of the tiles which have detail), a number for each tile (the height of a
// plateau, or the place in the atlas), and the lists of the tiles to draw (plateaus: two triangles; detail: a grid of triangles).

#pragma once
#ifndef _SIM_MODEL_TILES_H
#define _SIM_MODEL_TILES_H

#include "sim_model.h"
#include <vector>

class SimModelTiles : public SimModel
{
public:
	static const int T = 32;			// cells on the side of a tile
	static const int V = T + 1;			// heights on the side of a tile

private:
	struct Tile
	{
		std::vector<float> data;		// V x V heights, empty for a plateau
		float flat = 0;					// the height of a plateau
		int slot = -1;					// the place in the atlas of the graphic card, -1 if none
		bool dirty = false;
	};

	double _origin_x = 0, _origin_y = 0;
	double _step = 1.0;
	double _top = 0, _bottom = -1;
	bool _through = false;
	int _ntx = 0, _nty = 0;				// number of tiles
	int _width = 0, _height = 0;		// number of heights of the whole grid
	std::vector<Tile> _tiles;
	std::vector<int> _dirty;			// the tiles which changed since the last upload
	std::vector<int> _row_first, _row_last;	// the columns under the tool, for each row of the move which is cut
	size_t _detailed = 0;				// the tiles which have their heights

	// the graphic card
	Renderer* _render = nullptr;
	Texture* _info = nullptr;			// a number for each tile
	Texture* _atlas = nullptr;
	Texture* _flat_list = nullptr;
	Texture* _detail_list = nullptr;
	int _slots = 0;						// the places of the atlas
	int _atlas_cols = 1;
	std::vector<int> _free;				// the free places
	std::vector<float> _info_data;		// what the graphic card has in tile_info (a copy to send the rows which changed in one call)
	int _info_first = 0, _info_last = -1;	// the rows of tile_info which changed
	std::vector<int> _pending;			// the tiles whose heights have to be sent to the atlas
	std::vector<float> _staging;		// several tiles side by side, sent in one call
	int _flat_count = 0, _detail_count = 0;
	bool _full = true;					// everything has to be sent
	bool _lists_dirty = true;
	bool _warned = false;

	void mark_dirty(int index);
	void cut_piece(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool);
	void settle(int index);
	void reset_slots();
	void rebuild_lists();
	void flush();
	void release_graphics();

public:
	~SimModelTiles() override;

	const char* name() const override { return "Tiles"; }
	void setup(const SimBlock& block) override;
	void reset() override;
	void cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool) override;
	double height_at(double x, double y) const override;
	double removed_volume() const override;
	glm::dvec3 low() const override { return glm::dvec3(_origin_x, _origin_y, _bottom); }
	glm::dvec3 high() const override { return glm::dvec3(_origin_x + (_width - 1) * _step, _origin_y + (_height - 1) * _step, _top); }
	bool through() const override { return _through; }
	size_t cells() const override { return _detailed * V * V + _tiles.size(); }
	size_t memory() const override { return _detailed * V * V * sizeof(float) + _tiles.size() * sizeof(Tile); }
	std::string describe() const override;

	void create_graphics(Renderer* render) override;
	void upload() override;
	void draw(const SimLook& look) override;
};

#endif
