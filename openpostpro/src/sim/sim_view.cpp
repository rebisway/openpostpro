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

#include "sim_view.h"
#include <algorithm>
#include <cmath>

void SimView::Lines::add(const glm::vec3& a, const glm::vec3& b)
{
	if (count + 2 > capacity)
	{
		// larger by steps, the vertices which are not used yet repeat the last one (a line of length zero, which is not seen)
		size_t bigger = std::max<size_t>(4096, capacity * 2);
		vertices.resize(bigger, a);
		capacity = bigger;
		recreate = true;
	}
	vertices[count++] = a;
	vertices[count++] = b;
	changed = true;
}

SimView::SimView(Renderer* render)
{
	_render = render;
	_camera.set_size(1.0f, 1.0f);
}

SimView::~SimView()
{
	if (_heights) _render->delete_texture(_heights);
	if (_feed.buffer) _render->delete_buffer(_feed.buffer);
	if (_rapid.buffer) _render->delete_buffer(_rapid.buffer);
	if (_box) _render->delete_buffer(_box);
	if (_axes) _render->delete_buffer(_axes);
	if (_tool) _render->delete_buffer(_tool);
}

void SimView::set_material(const SimMaterial& material, bool through)
{
	if (_heights)
	{
		_render->delete_texture(_heights);
		_heights = nullptr;
	}

	_width = material.width();
	_height = material.height();
	_origin = glm::vec2((float)material.origin_x(), (float)material.origin_y());
	_step = (float)material.step();
	_top = (float)material.top();
	_bottom = (float)material.bottom();
	_through = through;
	if (_width > 0 && _height > 0)
		_heights = _render->create_float_texture(_width, _height, material.data().data());

	build_box();
	fit();
}

void SimView::fit()
{
	if (_width <= 0)
		return;
	glm::vec3 low(_origin.x, _origin.y, _bottom);
	glm::vec3 high(_origin.x + (_width - 1) * _step, _origin.y + (_height - 1) * _step, _top);
	_camera.fit(low, high);
}

void SimView::upload(SimMaterial& material)
{
	int x0, y0, x1, y1;
	if (_heights == nullptr || !material.take_dirty(x0, y0, x1, y1))
		return;
	if (material.width() != _width || material.height() != _height)
		return;
	const float* first = &material.data()[(size_t)y0 * _width + x0];
	_render->update_float_texture(_heights, x0, y0, x1 - x0 + 1, y1 - y0 + 1, first, _width);
}

void SimView::add_move(const SimMove& move)
{
	glm::vec3 a((float)move.from.x, (float)move.from.y, (float)move.from.z);
	glm::vec3 b((float)move.to.x, (float)move.to.y, (float)move.to.z);
	(move.rapid ? _rapid : _feed).add(a, b);
}

void SimView::clear_paths()
{
	_feed.clear();
	_rapid.clear();
}

void SimView::set_tool(const SimTool& tool, const glm::dvec3& position)
{
	if (tool.shape != _tool_shape.shape || tool.radius != _tool_shape.radius || tool.length != _tool_shape.length)
		_tool_changed = true;
	_tool_shape = tool;
	_tool_position = glm::vec3((float)position.x, (float)position.y, (float)position.z);
}

void SimView::sync(Lines& lines)
{
	if (lines.capacity == 0)
		return;
	if (lines.recreate || lines.buffer == nullptr)
	{
		if (lines.buffer)
			_render->delete_buffer(lines.buffer);
		lines.buffer = _render->create_buffer(lines.vertices);
		lines.recreate = false;
		lines.changed = false;
	}
	else if (lines.changed)
	{
		lines.buffer->update(lines.vertices);
		lines.changed = false;
	}
}

void SimView::build_box()
{
	if (_box)
	{
		_render->delete_buffer(_box);
		_box = nullptr;
	}
	if (_width <= 0)
		return;

	float x0 = _origin.x, y0 = _origin.y, x1 = _origin.x + (_width - 1) * _step, y1 = _origin.y + (_height - 1) * _step;
	float z0 = _bottom, z1 = _top;
	glm::vec3 c[8] = { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 }, { x0, y1, z0 }, { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 } };
	int edges[24] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };
	std::vector<glm::vec3> vertices;
	for (int i = 0; i < 24; i++)
		vertices.push_back(c[edges[i]]);
	_box = _render->create_buffer(vertices);

	// the axes at the origin of the program
	if (_axes)
	{
		_render->delete_buffer(_axes);
		_axes = nullptr;
	}
	_axes_length = std::max(5.0f, std::max(x1 - x0, y1 - y0) * 0.12f);
	std::vector<glm::vec3> axes = {
		{ 0, 0, 0 }, { _axes_length, 0, 0 }, { 0, 0, 0 }, { 0, _axes_length, 0 }, { 0, 0, 0 }, { 0, 0, _axes_length } };
	_axes = _render->create_buffer(axes);
}

void SimView::build_tool()
{
	if (_tool)
	{
		_render->delete_buffer(_tool);
		_tool = nullptr;
	}
	_tool_changed = false;

	float r = (float)std::max(_tool_shape.radius, 0.1);
	float length = (float)std::max(_tool_shape.length, 1.0);
	const int n = 24;
	std::vector<glm::vec3> v;
	// a circle at the tip and one at the top of the cutting part, the lines between them, a cross under the tip
	for (int i = 0; i < n; i++)
	{
		float a0 = 6.2831853f * i / n, a1 = 6.2831853f * (i + 1) / n;
		v.push_back(glm::vec3(r * std::cos(a0), r * std::sin(a0), 0));
		v.push_back(glm::vec3(r * std::cos(a1), r * std::sin(a1), 0));
		v.push_back(glm::vec3(r * std::cos(a0), r * std::sin(a0), length));
		v.push_back(glm::vec3(r * std::cos(a1), r * std::sin(a1), length));
	}
	for (int i = 0; i < 4; i++)
	{
		float a = 1.5707963f * i;
		v.push_back(glm::vec3(r * std::cos(a), r * std::sin(a), 0));
		v.push_back(glm::vec3(r * std::cos(a), r * std::sin(a), length));
	}
	v.push_back(glm::vec3(-r, 0, 0)); v.push_back(glm::vec3(r, 0, 0));
	v.push_back(glm::vec3(0, -r, 0)); v.push_back(glm::vec3(0, r, 0));
	_tool = _render->create_buffer(v);
}

void SimView::draw(float width, float height)
{
	_camera.set_size(width, height);

	_render->depth_test(true);

	// the block
	if (show_material && _heights != nullptr)
	{
		_render->use_program("sim");
		_render->set_uniform("projection", _camera.projection());
		_render->set_uniform("modelview", _camera.modelview());
		_render->set_uniform("eye", _camera.eye());
		_render->set_uniform("top_color", top_color);
		_render->set_uniform("cut_color", cut_color);
		_render->set_uniform("side_color", side_color);
		_render->set_uniform("grid_w", _width);
		_render->set_uniform("grid_h", _height);
		_render->set_uniform("origin", _origin);
		_render->set_uniform("step", _step);
		_render->set_uniform("top", _top);
		_render->set_uniform("bottom", _bottom);
		_render->bind_texture(_heights, 0);
		_render->set_uniform("heights", 0);

		_render->set_uniform("part", 0);
		_render->draw_arrays(_render->pr_triangles(), (_width - 1) * (_height - 1) * 6);
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
		_render->bind_texture(nullptr, 0);
	}

	// the lines
	_render->use_program("line3d");
	_render->set_uniform("projection", _camera.projection());
	_render->set_uniform("modelview", _camera.modelview());
	_render->set_uniform("offset", glm::vec3(0.0f));

	if (show_box && _box)
	{
		_render->set_uniform("inner_color", box_color);
		_box->draw(_render->pr_lines());
	}
	if (show_axes && _axes)
	{
		_render->set_uniform("inner_color", glm::vec4(1.0f, 0.25f, 0.25f, 1.0f));
		_axes->draw(_render->pr_lines(), 0, 2);
		_render->set_uniform("inner_color", glm::vec4(0.3f, 1.0f, 0.3f, 1.0f));
		_axes->draw(_render->pr_lines(), 2, 2);
		_render->set_uniform("inner_color", glm::vec4(0.4f, 0.5f, 1.0f, 1.0f));
		_axes->draw(_render->pr_lines(), 4, 2);
	}

	sync(_feed);
	sync(_rapid);
	if (show_rapid && _rapid.buffer && _rapid.count > 0)
	{
		_render->set_uniform("inner_color", rapid_color);
		_rapid.buffer->draw(_render->pr_lines(), 0, (int)_rapid.count);
	}
	if (show_feed && _feed.buffer && _feed.count > 0)
	{
		_render->set_uniform("inner_color", feed_color);
		_feed.buffer->draw(_render->pr_lines(), 0, (int)_feed.count);
	}

	if (show_tool)
	{
		if (_tool_changed || _tool == nullptr)
			build_tool();
		_render->set_uniform("offset", _tool_position);
		_render->set_uniform("inner_color", tool_color);
		_tool->draw(_render->pr_lines());
	}

	_render->depth_test(false);
}
