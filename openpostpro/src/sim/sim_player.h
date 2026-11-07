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

// The player of the simulator: plays the commands of a program at the real feed rates (or faster), pauses, steps, goes back,
// goes to a line or to the end. It does not draw and does not know the material: it reports each piece of path it goes along
// (on_move) and when it goes back to the start (on_reset).
//
// The commands of the program (dwell, stop M0 and M1) are executed as a machine does, except when the player goes fast
// (full speed, going to a line, going to the end): then they are skipped. The buttons (play, pause, step, back) are independent
// of the commands of the program.

#pragma once
#ifndef _SIM_PLAYER_H
#define _SIM_PLAYER_H

#include "sim_program.h"
#include "sim_geometry.h"
#include <functional>

struct SimMove
{
	glm::dvec3 from;
	glm::dvec3 to;
	bool rapid = false;
	int line = 0;
	int spindle = 0;		// 0 off, 1 clockwise, 2 counter-clockwise
	bool torch = false;
};

class SimPlayer
{
public:
	enum class State
	{
		Stopped,		// at the start
		Playing,
		Paused,			// by the button, or by a stop of the program
		Finished		// the end of the program (M2, M30, or the last command)
	};

	static const size_t none = (size_t)-1;

private:
	const SimProgram* _program = nullptr;
	State _state = State::Stopped;
	double _speed = 1.0;				// 1 is the real speed
	bool _full_speed = false;			// as fast as possible, without the waits of the program

	size_t _index = 0;					// the command which is played
	bool _started = false;				// the command has begun
	double _progress = 0;				// 0 to 1 in the command
	double _emitted = 0;				// the part of the move which was reported
	SimPath _path;
	glm::dvec3 _position = glm::dvec3(0);

	size_t _target = none;				// going to a command: the player goes fast until it is reached
	bool _target_end = false;			// ... or to the end of the program
	bool _stopped_by_program = false;	// a stop (M0, M1) of the program holds the player
	bool _waits_ignored = false;		// the waits of the program are skipped (fast play)

	int _tool = 0;
	int _spindle = 0;
	int _coolant = 0;
	bool _torch = false;
	double _rpm = 0;
	int _line = 0;

	void begin_command();
	void complete_command();
	void report(double t);
	void restart();

public:
	std::function<void(const SimMove&)> on_move;
	std::function<void()> on_reset;

	void program(const SimProgram* value);
	const SimProgram* program() const { return _program; }

	// the buttons
	void play();
	void pause();
	void toggle();
	void stop();					// back to the start
	void step_command();			// plays one command, then pauses
	void step_line();				// plays the commands of one line of the program, then pauses
	void go_to_command(size_t index);
	void go_to_line(int line);
	void go_to_end();
	void set_speed(double multiplier) { _speed = multiplier < 0.001 ? 0.001 : multiplier; }
	double speed() const { return _speed; }
	void full_speed(bool value) { _full_speed = value; }
	bool full_speed() const { return _full_speed; }

	// Plays during the frame: `seconds` is the time of the frame, `budget` the time which can be used by the work, in seconds
	void update(double seconds, double budget = 0.008);

	State state() const { return _state; }
	bool playing() const { return _state == State::Playing; }
	bool going() const { return _target != none || _target_end; }		// going to a line or to the end
	bool held_by_program() const { return _stopped_by_program; }
	size_t index() const { return _index; }
	int line() const { return _line; }
	glm::dvec3 position() const { return _position; }
	double elapsed() const;			// seconds of the machine since the start
	double total() const { return _program ? _program->stats().total_time : 0.0; }
	double fraction() const;		// 0 to 1, the part of the commands which was played
	int tool() const { return _tool; }
	int spindle() const { return _spindle; }
	int coolant() const { return _coolant; }
	bool torch() const { return _torch; }
	double rpm() const { return _rpm; }
	// the feed rate (mm/min) of the command which is played, 0 for a rapid move or when nothing is played
	double feed() const { return (_program && _index < _program->size()) ? _program->at(_index).feed : 0.0; }
};

#endif
