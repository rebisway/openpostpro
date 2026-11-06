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
#ifndef _FOLLOW_H
#define _FOLLOW_H

#include "toolpath.h"

class Follow : public Toolpath
{
private:
	// where the tool is relative to the followed shape. The machine compensates the radius of the tool (the kerf of a plasma
	// torch): G41 or G42 is deduced from this side and from the direction of the path
	ToolSide _tool_side = ToolSide::Centered;

public:
	GraphicType type() override { return GraphicType::CamFollow; }

	ToolSide tool_side() { return _tool_side; }
	// with a compensation, the entry is on the side of the tool: inside for a tool inside, outside for a tool outside
	void tool_side(ToolSide value)
	{
		_tool_side = value;
		if (_tool_side != ToolSide::Centered)
		{
			_start_point_inside = _tool_side == ToolSide::Inside;
			// the start is moved so that the entry is on the side of the tool; the user can change it afterwards
			fit_start_point();
		}
		compute();	// the entry can change side
	}
	ToolRadiusCompensation radius_compensation() override { return compensation_for(_tool_side, _cw); }
	
	Follow(Renderer* r) : Toolpath(r) {}

	bool start_point_allowed() override { return true; }
	bool tabs_allowed() override { return true; }

	void update() override;
	void scaled() override;
	std::vector<Curve> coordinates() override;

	void properties() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;

};

#endif