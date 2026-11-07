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

// The first model: a height for each column of a regular grid (SimMaterial). The heights are a texture read by the shader sim.

#pragma once
#ifndef _SIM_MODEL_GRID_H
#define _SIM_MODEL_GRID_H

#include "sim_model.h"
#include "sim_material.h"

class SimModelGrid : public SimModel
{
private:
	SimMaterial _material;
	Renderer* _render = nullptr;
	Texture* _heights = nullptr;
	bool _through = false;

	void release_graphics();

public:
	~SimModelGrid() override;

	const char* name() const override { return "Grid"; }
	void setup(const SimBlock& block) override;
	void reset() override { _material.reset(); }
	void cut(const glm::dvec3& a, const glm::dvec3& b, const SimTool& tool) override { _material.cut(a, b, tool); }
	double height_at(double x, double y) const override { return _material.height_at(x, y); }
	double removed_volume() const override { return _material.removed_volume(); }
	glm::dvec3 low() const override { return glm::dvec3(_material.origin_x(), _material.origin_y(), _material.bottom()); }
	glm::dvec3 high() const override { return glm::dvec3(_material.origin_x() + (_material.width() - 1) * _material.step(), _material.origin_y() + (_material.height() - 1) * _material.step(), _material.top()); }
	bool through() const override { return _through; }
	size_t cells() const override { return (size_t)_material.width() * _material.height(); }
	size_t memory() const override { return _material.data().size() * sizeof(float); }
	std::string describe() const override;

	void create_graphics(Renderer* render) override;
	void upload() override;
	void draw(const SimLook& look) override;

	SimMaterial& material() { return _material; }
};

#endif
