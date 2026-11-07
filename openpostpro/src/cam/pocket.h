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

#pragma once
#ifndef _POCKET_H
#define _POCKET_H

#include "toolpath.h"

enum class PocketMode
{
	Zigzag,
	Offset,
	Spiral
};

class Pocket : public Toolpath
{
private:
	std::vector<int> _indices;
	bool _climb = true;
	PocketMode _mode = PocketMode::Offset;
	float _finishing = 0.0f;
	float _stepover = 0.0f;
	float _angle = 0.0f;
	bool _contours = true;
	bool _outward = false;
	// number of curves of the toolpath which are the finishing pass (the last ones)
	size_t _final_count = 0;

public:
	GraphicType type() override { return GraphicType::CamPocket; }

	Pocket(Renderer* r) : Toolpath(r) {}

	PocketMode mode() { return _mode; }
	void mode(PocketMode value);

	float finishing() const { return _finishing; }
	void finishing(float value);

	// distance between two scan lines of the zigzag, the radius of the tool by default
	float stepover() { return _stepover; }
	void stepover(float value);
	float step() { return _stepover > 0 ? _stepover : radius(); }

	// angle of the scan lines of the zigzag, in degrees
	float angle() { return _angle; }
	void angle(float value);

	// if false, the zigzag is machined without the contour of the roughing area
	bool contours() { return _contours; }
	void contours(bool value);

	// spiral roughing direction: from the inside to the outside when true (finishing passes are always last)
	bool outward() { return _outward; }
	void outward(bool value);

	bool climb() { return _climb; }
	void climb(bool value) { _climb = value; }

	std::vector<Curve> zigzag(std::vector<Curve>& curves);
	std::vector<Curve> offset(std::vector<Curve>& curves);
	// chains concentric loops into continuous spirals (outside to inside)
	std::vector<Curve> spiral(std::vector<Curve> loops);
	std::vector<Curve> merge(std::vector<Curve>& curves);

	void update() override;
	// a pocket keeps the whole tree of its shapes
	void refresh(TreeCurve* value) override;
	void scaled() override;
	std::vector<Curve> coordinates() override;
	size_t final_count() override { return _final_count; }

	void properties() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;
};

#endif