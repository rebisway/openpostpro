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

// The simple commands of the simulator: what a translator (Python) sends and what the player (C++) executes.
// See docs/SIMULATOR.fr.md for the contract. Everything here is independent of the interface and of OpenGL.

#pragma once
#ifndef _SIM_TYPES_H
#define _SIM_TYPES_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

enum class SimCommandType
{
	Rapid,
	Linear,
	Arc,
	Dwell,
	Tool,
	Spindle,
	Coolant,
	Torch,
	Stop,
	Message,
	Custom		// a command which the simulator does not know: it is noted once and ignored
};

enum class SimPlane { XY, XZ, YZ };

enum class SimLevel { Info, Warning, Error };

struct SimCommand
{
	SimCommandType type = SimCommandType::Message;
	int line = 0;						// line of the source, from 1 (0 when unknown)

	glm::dvec3 target = glm::dvec3(0);	// rapid, linear, arc: the end point, in millimeters
	glm::dvec3 center = glm::dvec3(0);	// arc: the center
	double feed = 0;					// linear, arc: mm/min
	bool cw = false;					// arc: clockwise
	SimPlane plane = SimPlane::XY;		// arc

	double seconds = 0;					// dwell
	int number = 0;						// tool
	double rpm = 0;						// spindle
	int state = 0;						// spindle: 0 off 1 cw 2 ccw. coolant: 0 off 1 flood 2 mist. torch: 0 off 1 on
	bool optional = false;				// stop: M1
	bool end = false;					// stop: the end of the program (M2, M30)

	SimLevel level = SimLevel::Info;	// message
	std::string text;					// message text, name of a custom command

	bool moves() const { return type == SimCommandType::Rapid || type == SimCommandType::Linear || type == SimCommandType::Arc; }
};

// the shape of the tool: the height of its surface above its tip at a distance of its axis
enum class SimToolShape
{
	Flat,		// a cylinder
	Ball,		// hemispherical
	Torus,		// a cylinder with a rounded corner
	Cone,		// a V cutter
	Plasma		// a torch: it cuts through the material, with the width of its kerf
};

struct SimTool
{
	SimToolShape shape = SimToolShape::Flat;
	double radius = 3.0;			// mm
	double small_radius = 0.0;		// torus: radius of the corner; cone: radius of the tip
	double angle = 60.0;			// cone: included angle in degrees
	double length = 30.0;			// mm, for the display
};

struct SimSettings
{
	double rapid_speed = 5000.0;		// mm/min, XY
	double rapid_z_speed = 5000.0;		// mm/min, when only Z moves
	double arc_tolerance = 0.02;		// mm, maximum distance between an arc and its chords
	bool optional_stop = true;			// M1 stops the program
	glm::dvec3 start_position = glm::dvec3(0);
	double precision = 15.0;			// percent of the radius of the tool: the step of the grid of the material
	int max_cells = 2000000;			// the step is made larger if the grid would have more cells (the drawing of 4 million cells takes 28 ms)
};

#endif
