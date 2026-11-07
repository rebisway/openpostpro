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

// The drawing of the simulator, with the renderer of the application: the block of material (drawn by its model, SimModel), the paths of the tool, the tool, the box of the block and the axes. It knows nothing of the interface (windows, menus).

#pragma once
#ifndef _SIM_VIEW_H
#define _SIM_VIEW_H

#include "sim_model.h"
#include "sim_player.h"
#include "renderer.h"
#include "orbit_cam.h"

class SimView
{
private:
	// lines which grow: the buffer is made larger by steps, and only its first `count` vertices are drawn
	struct Lines
	{
		std::vector<glm::vec3> vertices;
		size_t count = 0;
		size_t capacity = 0;
		Buffer* buffer = nullptr;
		bool changed = false;
		bool recreate = false;

		void clear() { count = 0; changed = true; }
		void add(const glm::vec3& a, const glm::vec3& b);
	};

	Renderer* _render;
	OrbitCamera _camera;

	SimModel* _model = nullptr;					// the material: it is not owned, it has to live as long as the view uses it

	Lines _feed;
	Lines _rapid;
	Buffer* _box = nullptr;
	Buffer* _axes = nullptr;
	Buffer* _tool = nullptr;
	float _axes_length = 10.0f;
	SimTool _tool_shape;
	bool _tool_changed = true;
	glm::vec3 _tool_position = glm::vec3(0);

	void sync(Lines& lines);
	void build_tool();
	void build_box();

public:
	// colors (red, green, blue, alpha)
	glm::vec4 top_color = glm::vec4(0.78f, 0.64f, 0.46f, 1.0f);		// the block which was not touched
	glm::vec4 cut_color = glm::vec4(0.58f, 0.42f, 0.29f, 1.0f);		// what was machined
	glm::vec4 side_color = glm::vec4(0.68f, 0.54f, 0.38f, 1.0f);
	glm::vec4 feed_color = glm::vec4(1.0f, 0.62f, 0.05f, 1.0f);
	glm::vec4 rapid_color = glm::vec4(0.35f, 0.75f, 1.0f, 1.0f);
	glm::vec4 tool_color = glm::vec4(1.0f, 1.0f, 0.4f, 1.0f);
	glm::vec4 box_color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	glm::vec4 background = glm::vec4(0.18f, 0.19f, 0.21f, 1.0f);

	bool show_material = true;
	bool show_feed = true;
	bool show_rapid = true;
	bool show_tool = true;
	bool show_axes = true;
	bool show_box = true;

	SimView(Renderer* render);
	~SimView();

	OrbitCamera& camera() { return _camera; }

	// A new block (the model is set up): its graphic resources are made again, the camera looks at the block
	void set_model(SimModel& model);
	// What changed in the model since the last call is sent to the graphic card
	void upload();

	// The paths: a piece of path, or all of them removed
	void add_move(const SimMove& move);
	void clear_paths();
	size_t path_vertices() const { return _feed.count + _rapid.count; }

	// The tool at its position
	void set_tool(const SimTool& tool, const glm::dvec3& position);

	// Draws the scene in the viewport which is set. The size is the one of the viewport, in pixels
	void draw(float width, float height);

	// the camera looks at the block
	void fit();
};

#endif
