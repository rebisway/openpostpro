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

#include "sim_model_grid.h"

SimModelGrid::~SimModelGrid()
{
	release_graphics();
}

void SimModelGrid::release_graphics()
{
	if (_render != nullptr && _heights != nullptr)
		_render->delete_texture(_heights);
	_heights = nullptr;
}

void SimModelGrid::setup(const SimBlock& block)
{
	double step = SimMaterial::choose_step(block.high.x - block.low.x, block.high.y - block.low.y, block.tool_radius, block.precision, block.max_cells);
	_material.setup(block.low, block.high, block.top, block.bottom, step);
	_through = block.through;
	// the texture has the size of the old grid: it is made again by create_graphics
	release_graphics();
}

std::string SimModelGrid::describe() const
{
	return std::to_string(_material.width()) + " x " + std::to_string(_material.height()) + " columns, step " + std::to_string(_material.step()) + " mm";
}

void SimModelGrid::create_graphics(Renderer* render)
{
	release_graphics();
	_render = render;
	if (_material.width() > 0 && _material.height() > 0)
		_heights = _render->create_float_texture(_material.width(), _material.height(), _material.data().data());
}

void SimModelGrid::upload()
{
	int x0, y0, x1, y1;
	if (_heights == nullptr || !_material.take_dirty(x0, y0, x1, y1))
		return;
	const int width = _material.width();
	const float* first = &_material.data()[(size_t)y0 * width + x0];
	_render->update_float_texture(_heights, x0, y0, x1 - x0 + 1, y1 - y0 + 1, first, width);
}

void SimModelGrid::draw(const SimLook& look)
{
	if (_heights == nullptr)
		return;
	const int width = _material.width(), height = _material.height();
	_render->use_program("sim");
	_render->set_uniform("projection", look.projection);
	_render->set_uniform("modelview", look.modelview);
	_render->set_uniform("eye", look.eye);
	_render->set_uniform("top_color", look.top_color);
	_render->set_uniform("cut_color", look.cut_color);
	_render->set_uniform("side_color", look.side_color);
	_render->set_uniform("grid_w", width);
	_render->set_uniform("grid_h", height);
	_render->set_uniform("origin", glm::vec2((float)_material.origin_x(), (float)_material.origin_y()));
	_render->set_uniform("step", (float)_material.step());
	_render->set_uniform("top", (float)_material.top());
	_render->set_uniform("bottom", (float)_material.bottom());
	_render->bind_texture(_heights, 0);
	_render->set_uniform("heights", 0);

	_render->set_uniform("part", 0);
	_render->draw_arrays(_render->pr_triangles(), (width - 1) * (height - 1) * 6);
	_render->set_uniform("part", 1);
	_render->draw_arrays(_render->pr_triangle_strip(), width * 2);
	_render->set_uniform("part", 2);
	_render->draw_arrays(_render->pr_triangle_strip(), width * 2);
	_render->set_uniform("part", 3);
	_render->draw_arrays(_render->pr_triangle_strip(), height * 2);
	_render->set_uniform("part", 4);
	_render->draw_arrays(_render->pr_triangle_strip(), height * 2);
	if (!_through)
	{
		_render->set_uniform("part", 5);
		_render->draw_arrays(_render->pr_triangles(), 6);
	}
	_render->bind_texture(nullptr, 0);
}
