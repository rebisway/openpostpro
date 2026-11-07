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
#include "compensation.h"

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
	// the path leaves the contour before the tool goes back up: as a line, back to the start of the entry; as an arc, a quarter of
	// circle which goes on tangent after the start of the contour. Only with an entry
	bool _lead_out = false;

	float _tabs_count = 0;
	float _tabs_length = 0;
	float _tabs_height = 0;
	// position of the center of each tab, in length along the curve (independent of the start point)
	std::vector<float> _tabs_positions;
	int _tab_selected = 0;
	float _tabs_total = 0;
	Buffer* _tab_buffer = nullptr;
	// end index in the buffer and index of the tab of each drawn part
	std::vector<std::pair<int, int>> _tab_indices;

	void generate_startpoint(std::vector<Curve>& curves);
	// splits a closed curve to flag the moves which are tabs (tabs_count tabs of tabs_length, evenly spread)
	// positions: the tabs are at _tabs_positions (single closed curve), otherwise they are evenly spread.
	// offset: length of the curve which has been moved to the start of the curve.
	Curve generate_tabs(Curve& curve, float offset = 0, bool positions = false);
	// evenly spread positions when the number of tabs changed
	void tabs_init_positions(float total);
	void generate_tabs_deco(std::vector<Curve>& curves);
	void tabs_properties(const Curve& curve);
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

	// draw the properties window, the rows come from properties()
	void ui() override { display_properties(); }

	void start_point_type(StartPointType value);
	StartPointType start_point_type() { return _start_point_type; }

	void start_point_length(float value);
	float start_point_length() { return _start_point_length; }

	void start_point_offset(float value);
	float start_point_offset() { return _start_point_offset; }

	void start_point_inside(bool value);
	float start_point_inside() { return _start_point_inside; }

	bool lead_out() { return _lead_out && _start_point_type != StartPointType::normal; }
	void lead_out(bool value);

	virtual bool tabs_allowed() { return false; }

	// moves the start of the path so that the entry comes from the side of the tool (with a radius compensation)
	void fit_start_point();

	// side of the tool relative to the direction of the movement (G41 / G42), None for the toolpaths which are not compensated
	virtual ToolRadiusCompensation radius_compensation() { return ToolRadiusCompensation::None; }

	void tabs_count(float value);
	float tabs_count() { return _tabs_count; }

	void tabs_length(float value);
	float tabs_length() { return _tabs_length; }

	void tabs_height(float value);
	float tabs_height() { return _tabs_height; }

	void tab_position(int index, float value);
	// text of the positions for write() and reading them back
	std::string tabs_write();
	void tabs_read(const std::vector<std::string>& data, int index);
	// draws the tabs over the toolpath, with their own colors
	void draw_tabs(glm::vec4 color, glm::vec4 selected_color);
	
	virtual int closed_curves() { return 0; }

	Toolpath(Renderer* r);
	~Toolpath();

	void add(Curve value);
	void tree(TreeCurve* value);

	// the shapes the toolpath was made from were modified: takes the new geometry (a tree built from the referenced shapes, which the
	// toolpath owns) and computes the toolpath again, with its own settings
	virtual void refresh(TreeCurve* value);

	void draw() override;

	bool cw();
	virtual void cw(bool value);

	virtual std::vector<Curve> coordinates() { return std::vector<Curve>(); }

	// number of curves at the end of coordinates() which have to be machined after all the others (the finishing pass of a pocket)
	virtual size_t final_count() { return 0; }
};

#endif