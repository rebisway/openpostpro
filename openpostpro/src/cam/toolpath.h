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
#ifndef _TOOLPATH_H
#define _TOOLPATH_H

#include "graphic.h"
#include <geometry.h>
#include <curve.h>

enum class StartPointType {
	normal,
	line,
	arc
};

class Toolpath : public Graphic
{
private:
	std::string _parent;
	std::vector<int> _references_id;

protected:
	bool _cw = false;
	float _radius = 0;
	std::vector<Curve> _original;
	TreeCurve* _tree = nullptr;
	std::vector<Curve> _computed;
	Buffer* _data_buffer = nullptr;
	Buffer* _deco_buffer = nullptr;
	std::vector<int> _data_indices;
	std::vector<int> _deco_indices;

	StartPointType _start_point_type = StartPointType::normal;
	float _start_point_length = 0;
	float _start_point_offset = 0;
	bool _start_point_inside = false;

	float _tabs_count = 0;
	float _tabs_length = 0;
	float _tabs_height = 0;

	void generate_startpoint(std::vector<Curve>& curves);
	void generate_data(std::vector<Curve>& curves);
	void generate_deco(std::vector<Curve>& curves);

public:
	std::string parent() { return _parent; }
	void parent(std::string value) { _parent = value; }

	void name(std::string value) { _name = value; }
	std::string name() { return _name; }

	void radius(float value) { _radius = value; }
	float radius() { return _radius; }

	std::vector<int> references();
	void references(std::vector<int> value);
	void reference(int value);

	virtual bool start_point_allowed() { return false; }

	void start_point_type(StartPointType value);
	StartPointType start_point_type() { return _start_point_type; }

	void start_point_length(float value);
	float start_point_length() { return _start_point_length; }

	void start_point_offset(float value);
	float start_point_offset() { return _start_point_offset; }

	void start_point_inside(bool value);
	float start_point_inside() { return _start_point_inside; }

	virtual bool tabs_allowed() { return false; }

	void tabs_count(float value);
	float tabs_count() { return _tabs_count; }

	void tabs_length(float value);
	float tabs_length() { return _tabs_length; }

	void tabs_height(float value);
	float tabs_height() { return _tabs_height; }
	
	virtual int closed_curves() { return 0; }

	Toolpath(Renderer* r);
	~Toolpath();

	void add(Curve value);
	void tree(TreeCurve* value);
	
	void draw() override;

	bool cw();
	virtual void cw(bool value);

	virtual std::vector<Curve> coordinates() { return std::vector<Curve>(); }
};

#endif