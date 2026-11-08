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

// A program for the simulator: the list of the commands made by a translator, with what is known about it before it is played
// (the time of each command, the bounds, the messages).

#pragma once
#ifndef _SIM_PROGRAM_H
#define _SIM_PROGRAM_H

#include "sim_types.h"

struct SimStats
{
	glm::dvec3 low = glm::dvec3(0);			// the box which holds every position of the tool
	glm::dvec3 high = glm::dvec3(0);
	glm::dvec3 cut_low = glm::dvec3(0);		// the box which holds the moves at a feed rate (the ones which cut)
	glm::dvec3 cut_high = glm::dvec3(0);
	bool has_cut = false;
	double total_time = 0;					// seconds, with the rapid speeds of the settings
	double rapid_length = 0;
	double feed_length = 0;
	int moves = 0;
	int infos = 0, warnings = 0, errors = 0;
	int unknown = 0;						// commands which the simulator does not know
};

class SimProgram
{
private:
	std::vector<SimCommand> _commands;
	std::vector<double> _times;				// the time at the end of each command, in seconds
	SimStats _stats;
	SimSettings _settings;

public:
	void clear();
	void add(const SimCommand& command) { _commands.push_back(command); }
	void add(const std::vector<SimCommand>& commands) { _commands.insert(_commands.end(), commands.begin(), commands.end()); }

	// Changes the sign of the coordinates of the axes which are true (a machine whose axes count the other way than the drawing: the
	// program has the opposite values, the simulator shows the part as it is). An arc turns the other way when only one of the two axes
	// of its plane is changed. To call before analyze().
	void mirror(bool x, bool y, bool z);

	// Computes the times and the bounds. To call again when the settings change
	void analyze(const SimSettings& settings);

	const std::vector<SimCommand>& commands() const { return _commands; }
	size_t size() const { return _commands.size(); }
	const SimCommand& at(size_t i) const { return _commands[i]; }
	const SimStats& stats() const { return _stats; }
	const SimSettings& settings() const { return _settings; }

	// the time in seconds at the start and at the end of a command
	double start_time(size_t i) const { return i == 0 ? 0.0 : _times[i - 1]; }
	double end_time(size_t i) const { return _times[i]; }
	double duration(size_t i) const { return _times[i] - start_time(i); }

	// the first command of a line of the source (or the following ones), or the number of commands if there is none
	size_t command_at_line(int line) const;

	// the time in seconds of a move of the settings, from a position
	static double move_time(const SimCommand& command, const glm::dvec3& from, const SimSettings& settings);
};

#endif
