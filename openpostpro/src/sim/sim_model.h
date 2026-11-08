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

// A model of the material of the simulator: what the tool takes away, and how it is drawn. The models have the same interface, so the
// methods can be tried and compared on the same program (same cuts, same volume, same heights; the memory and the times differ).
//
// To add a method: derive SimModel, add it in SimModel::create and SimModel::method_name (sim_model.cpp).

#pragma once
#ifndef _SIM_MODEL_H
#define _SIM_MODEL_H

#include "sim_types.h"
#include "renderer.h"
#include <memory>
#include <string>

// The block to machine, and what the model needs to choose its resolution
struct SimBlock
{
	glm::dvec2 low = glm::dvec2(0);		// corners in the plane
	glm::dvec2 high = glm::dvec2(1);
	double top = 0;						// heights of the top and of the bottom of the block
	double bottom = -1;
	double tool_radius = 1;				// the resolution is a part of the radius of the tool...
	double precision = 15;				// ... (precision, in percent) ...
	int max_cells = 4000000;			// ... and the model keeps its memory and its drawing under this number of cells
	bool through = false;				// a torch: the block has no bottom face, it is cut through
	bool plateaus = true;				// the tool leaves flat floors (a flat tool): the models which simplify them can use a finer step
};

// What the drawing needs from the scene
struct SimLook
{
	glm::mat4 projection = glm::mat4(1);
	glm::mat4 modelview = glm::mat4(1);
	glm::vec3 eye = glm::vec3(0);
	glm::vec4 top_color = glm::vec4(1);		// the block which was not touched
	glm::vec4 cut_color = glm::vec4(1);		// what was machined
	glm::vec4 side_color = glm::vec4(1);
};

class SimModel
{
public:
	virtual ~SimModel() {}

	virtual const char* name() const = 0;

	// The block: back to the material as it was before the machining
	virtual void setup(const SimBlock& block) = 0;
	virtual void reset() = 0;

	// The tool goes from a to b (positions of its tip)
	virtual void cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool) = 0;

	// The height of the material under a point of the plane, and the volume which was taken away (mm3)
	virtual double height_at(double x, double y) const = 0;
	virtual double removed_volume() const = 0;

	// The box of the block (x, y, and the bottom for low; the top for high)
	virtual glm::dvec3 low() const = 0;
	virtual glm::dvec3 high() const = 0;
	virtual bool through() const = 0;

	// The size of the model: elements (columns, tiles, vertices), memory in bytes, and a sentence for the log
	virtual size_t cells() const = 0;
	virtual size_t memory() const = 0;
	virtual std::string describe() const = 0;

	// The graphic resources are made after setup, the changes are sent to the graphic card by upload, draw shows the block
	virtual void create_graphics(Renderer* render) = 0;
	virtual void upload() = 0;
	virtual void draw(const SimLook& look) = 0;

	// The methods
	static int method_count();
	static const char* method_name(int method);
	static std::unique_ptr<SimModel> create(int method);
};

#endif
