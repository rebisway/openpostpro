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
#include <string>
#include <functional>
#include <vector>
#include <renderer.h>
#include <environment.h>
#include <history.h>

/* Macro for writing into file */
#define WN(a) std::to_string(a) + ';'
#define WB(a) std::to_string(a) + ';'
#define WF(a) stringex::to_string(a) + ';'
#define WS(a) stringex::replace(a, ";", "U003B") + ';'
#define WC(a) geometry::to_string(a, 0, ':')  + ';'
#define WPOINT(a) std::to_string(a.x) + ',' + std::to_string(a.y) + ';'
#define WSEGMENT(a) std::to_string((int)a.type) + ':' + std::to_string(a.point.x) + ',' + std::to_string(a.point.y) + (a.type == SegmentType::Arc || a.type == SegmentType::Circle ? ':' + std::to_string(a.center.x) + ',' + std::to_string(a.center.y) + ':' + std::to_string(a.cw) : "") + ';'

/* MACRO for reading from file*/
#define RS(a, b)  if (b < data.size()) a = stringex::replace(data[b], "U003B", ";");
#define RC(a, b)  if (b < data.size()) a = geometry::from_string(data[b], ':');
#define RI(a, b)  if (b < data.size()) a = std::stoi(data[b]);
#define RF(a, b)  if (b < data.size()) a = std::stof(data[b]);
#define RB(a, b)  if (b < data.size()) a = (bool)std::stoi(data[b]);
#define RU(a, b)  if (b < data.size()) a = (unsigned int)std::stoi(data[b]);
#define RT(b)	  if (b < data.size()) parent(data[b]);
#define RPOINT(a, b)  if (b < data.size()) a = geometry::from_string2(data[b], ',');
#define RSEGMENT(a, b) if (b < data.size()) \
						{ \
							auto d = stringex::split(data[b], ':'); \
							a.point = geometry::from_string2(d[1], ','); \
							if ((a.type = (SegmentType)std::stoi(d[0])) != SegmentType::Line) \
							{ \
								a.center = geometry::from_string2(d[2], ','); \
								a.cw = (bool)std::stoi(d[3]); \
								a.radius = geometry::distance(a.point, a.center); \
							} \
						}

enum class GraphicType
{
	None		  = 0,
	Layer		  = 1,
	Group		  = 2,
	Circle		  = 100,
	Point		  = 101,
	Arc			  = 102,
	Line		  = 103,
	Ellipse		  = 104,
	Polyline	  = 105,
	Text		  = 106,
	Spline		  = 107,
	CamMoveTo	  = 201,
	CamFollow	  = 202,
	CamOffset	  = 203,
	CamSpiral	  = 204,
	CamPocket     = 205,
	CamDrill      = 206,
	CadOrigin	  = 301,
	CadOffset	  = 302,
	CadSymmetry	  = 303,
	CadScale	  = 304,
	CadRotation   = 305,
	CadSChamfer   = 306,
	CadRChamfer   = 307,
	CadExtend     = 308,
	CadCut        = 309,
	CamStartpoint = 401,
	CamTab		  = 402
};

class Graphic
{
protected:
	unsigned int _id;
	std::string _name;
	Renderer* _render;
	bool _selected = false;		// true if shape is selected
	bool _needs_update = false;

	// update visual
	virtual void update() {};
	virtual void needs_update() { _needs_update = true; }

	// override to add the rows of the properties window, using the property_* helpers
	virtual void properties() {}
	// icon buttons displayed above the table of the properties, in the same way as the buttons of the Project window
	virtual void properties_toolbar() {}

	// draw the properties window: common frame, then the rows given by properties()
	void display_properties();

	// each helper draws one row (label + input). Return true when the user validated a new value, stored in 'value'
	bool property_point(const char* label, const char* id, glm::vec2& value);
	bool property_float(const char* label, const char* id, float& value);
	bool property_int(const char* label, const char* id, int& value);
	bool property_bool(const char* label, const char* id, bool& value);
	bool property_string(const char* label, const char* id, std::string& value);
	// drop-down list: 'selected' is the index in 'items', returns true when another item is chosen
	bool property_combo(const char* label, const char* id, int& selected, const std::vector<std::string>& items);
	// slider: returns true while the value moves; the undo step is pushed by the helper, once per drag (do not call modify())
	bool property_slider(const char* label, const char* id, float& value, float min, float max);
	// read only row
	void property_text(const char* label, const std::string& value);

	// push the undo action, then apply the change
	void modify(const std::function<void()>& change);

public:
	unsigned int id() { return _id; }
	void id(unsigned int value) { _id = value; }
	std::string name() { return _name; }
	void name(std::string n);

	bool selected() { return _selected; }
	void selected(bool value) { if (_selected != value) { _selected = value; } }

	bool shape();
	bool toolpath();

	Renderer* render() { return _render; }

	virtual GraphicType type() { return GraphicType::None; }

	Graphic(Renderer* r) { _id = environment::next_id(); _render = r; }

	virtual void transform(glm::tmat4x4<float> mat) {}

	// change renderer
	virtual void reset(Renderer* r) { _render = r; }

	// compute coordinates - must be called at end of overidden function
	virtual void compute();

	virtual void scaled();

	// draw visual - must be called at begining of overidden function
	virtual void draw();

	// draw anchors
	virtual void draw_anchors() {}

	// draw user interface
	virtual void ui() {}

	// deserialize 
	virtual std::string write() { return ""; }

	// serialize
	virtual void read(std::string value, float version=0) {}

private:
	void property_label(const char* label);
	std::string _slider_snapshot;		// state saved when a slider drag starts
};