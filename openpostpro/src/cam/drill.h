// OpenPostPro - www.openpostpro.org
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

#pragma once
#ifndef _DRILL_H
#define _DRILL_H

#include "toolpath.h"

#define _DRILL_SIZE 10

enum class DrillMode
{
	Manual   = 0,
	Drilling = 1,
	Pecking  = 2,
	Tapping  = 3,
	Boring   = 4
};

class Drill : public Toolpath
{
private:
	DrillMode _mode = DrillMode::Drilling;
	float _retract = 0;
	int _pause = 0;
	float _delta = 0;

public:
	GraphicType type() override { return GraphicType::CamDrill; }

	void mode(DrillMode value) { _mode = value; }
	DrillMode mode() { return _mode; }

	void retract(float value) { _retract = value; }
	float retract() { return _retract; }

	void delta(float value) { _delta = value; }
	float delta() { return _delta; }

	void pause(int value) { _pause = value; }
	int pause() { return _pause; }

	glm::vec2 point() { return _original[0][0].point; }
	void point(glm::vec2 value) { _original[0][0].point = value; }

	Drill(Renderer* r);
	~Drill();

	std::vector<Curve> coordinates() override;
	void compute() override;
	void scaled() override;
	void ui() override;
	void draw() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;
};

#endif