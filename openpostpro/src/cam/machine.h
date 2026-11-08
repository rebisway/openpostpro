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
#include <vector>
#include <glm/glm.hpp>

// Where the zero of the machine is on its table, seen from above with Y going up on the drawing
enum class MachineOrigin
{
	BottomLeft,
	BottomRight,
	TopLeft,
	TopRight,
	Center
};

// A machine: the useful area of its table and the way its axes go. The origin of the drawing is the zero of the machine (G54): the area
// is around it. Some machines count the axes the other way than the operator sees them (X which grows from the right to the left on a
// machine whose console is in front of it): then the program has the opposite values, and the arcs turn the other way if only one of X
// and Y is reversed. The drawing and the simulator show the machine as the operator sees it.
struct Machine
{
	std::string name;
	float width = 600;			// travel of X, in millimeters
	float depth = 400;			// travel of Y
	float height = 100;			// travel of Z
	MachineOrigin origin = MachineOrigin::BottomLeft;
	bool invert_x = false;
	bool invert_y = false;
	bool invert_z = false;

	// the area in the coordinates of the drawing: x, y of the corner low-left, then high-right
	void area(glm::vec2& low, glm::vec2& high) const;
	bool contains(glm::vec2 point, float tolerance = 0.001f) const;

	// the program axes from the drawing ones, and back (the same operation: it is a change of sign)
	glm::vec3 to_machine(glm::vec3 point) const { return glm::vec3(invert_x ? -point.x : point.x, invert_y ? -point.y : point.y, invert_z ? -point.z : point.z); }
	glm::vec3 from_machine(glm::vec3 point) const { return to_machine(point); }
	// an arc in the plane XY turns the other way when one axis is reversed (and not when both are)
	bool flips_arcs() const { return invert_x != invert_y; }
	// the center of an arc (relative to its start, or absolute: the same change of sign) and its direction, in the program
	void arc_to_machine(float& i, float& j, bool& cw) const
	{
		if (invert_x) i = -i;
		if (invert_y) j = -j;
		if (flips_arcs()) cw = !cw;
	}
	bool reversed() const { return invert_x || invert_y || invert_z; }

	std::string write() const;
	static Machine read(const std::string& text);
};

// The machines of the user, in a text file (one line each, the name is the key)
class MachineTable
{
private:
	std::vector<Machine> _machines;

public:
	std::vector<Machine>& machines() { return _machines; }
	Machine* find(const std::string& name);
	// adds the machine; the name gets a number if it is already used. Returns the machine in the table
	Machine& add(Machine machine);
	bool remove(const std::string& name);
	bool rename(const std::string& name, const std::string& to);

	std::string write() const;
	void read(const std::string& text);

	static std::string default_path();
	bool load(const std::string& path = "");
	bool save(const std::string& path = "") const;
};

MachineTable& machine_table();
