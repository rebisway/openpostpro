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

#include "sim_player.h"
#include <algorithm>
#include <chrono>
#include <cmath>

void SimPlayer::program(const SimProgram* value)
{
	_program = value;
	restart();
	_state = State::Stopped;
}

void SimPlayer::restart()
{
	_index = 0;
	_started = false;
	_progress = 0;
	_emitted = 0;
	_target = none;
	_target_end = false;
	_stopped_by_program = false;
	_tool = 0;
	_spindle = 0;
	_coolant = 0;
	_torch = false;
	_rpm = 0;
	_line = 0;
	_position = _program ? _program->settings().start_position : glm::dvec3(0);
	if (on_reset)
		on_reset();
}

// ---------------------------------------------------------------------------------------------- the buttons
void SimPlayer::play()
{
	if (_program == nullptr || _program->size() == 0)
		return;
	if (_state == State::Finished)
		restart();
	_target = none;
	_target_end = false;
	_stopped_by_program = false;
	_state = State::Playing;
}

void SimPlayer::pause()
{
	if (_state == State::Playing)
		_state = State::Paused;
	_target = none;
	_target_end = false;
}

void SimPlayer::toggle()
{
	if (_state == State::Playing)
		pause();
	else
		play();
}

void SimPlayer::stop()
{
	restart();
	_state = State::Stopped;
}

void SimPlayer::go_to_command(size_t index)
{
	if (_program == nullptr)
		return;
	index = std::min(index, _program->size());
	// a command which was passed (or is half played) is played again from the start
	if (index < _index || (index == _index && _progress > 0) || _state == State::Finished)
		restart();
	_target = index;
	_target_end = false;
	_stopped_by_program = false;
	_state = State::Playing;
}

void SimPlayer::go_to_line(int line)
{
	if (_program == nullptr)
		return;
	go_to_command(_program->command_at_line(line));
}

void SimPlayer::go_to_end()
{
	if (_program == nullptr || _program->size() == 0)
		return;
	if (_state == State::Finished)
		return;
	_target = _program->size();
	_target_end = true;
	_stopped_by_program = false;
	_state = State::Playing;
}

void SimPlayer::step_command()
{
	if (_program == nullptr || _state == State::Finished)
		return;
	// the next move (the commands which show nothing are played with it)
	size_t i = _index + (_started ? 1 : 0);
	while (i < _program->size() && !_program->at(i).moves())
		i++;
	go_to_command(std::min(i + 1, _program->size()));
}

void SimPlayer::step_line()
{
	if (_program == nullptr || _state == State::Finished)
		return;
	size_t i = _index + (_started ? 1 : 0);
	if (i >= _program->size())
	{
		go_to_command(_program->size());
		return;
	}
	int line = _program->at(i).line;
	while (i < _program->size() && _program->at(i).line <= line)
		i++;
	go_to_command(i);
}

// ---------------------------------------------------------------------------------------------- playing
double SimPlayer::elapsed() const
{
	if (_program == nullptr || _program->size() == 0)
		return 0;
	if (_index >= _program->size())
		return _program->stats().total_time;
	return _program->start_time(_index) + _progress * _program->duration(_index);
}

double SimPlayer::fraction() const
{
	if (_program == nullptr || _program->size() == 0)
		return 0;
	if (_index >= _program->size())
		return 1;
	double total = _program->stats().total_time;
	if (total > 1e-9)
		return std::min(1.0, elapsed() / total);
	return (double)_index / (double)_program->size();
}

void SimPlayer::report(double t)
{
	if (t <= _emitted)
		return;

	double range = t - _emitted;
	int pieces = std::max(1, (int)std::ceil(range * _path.pieces(_program->settings().arc_tolerance) - 1e-9));
	const SimCommand& c = _program->at(_index);

	for (int k = 1; k <= pieces; k++)
	{
		double tk = _emitted + range * k / pieces;
		glm::dvec3 p = (k == pieces && t >= 1.0) ? _path.to() : _path.at(tk);
		if (glm::distance(p, _position) > 1e-12 && on_move)
		{
			SimMove move;
			move.from = _position;
			move.to = p;
			move.rapid = c.type == SimCommandType::Rapid;
			move.line = c.line;
			move.spindle = _spindle;
			move.torch = _torch;
			on_move(move);
		}
		_position = p;
	}
	_emitted = t;
}

void SimPlayer::begin_command()
{
	const SimCommand& c = _program->at(_index);
	_started = true;
	_progress = 0;
	_emitted = 0;
	_line = c.line;

	switch (c.type)
	{
	case SimCommandType::Rapid:
	case SimCommandType::Linear:
	case SimCommandType::Arc:
		_path = SimPath(_position, c);
		break;
	case SimCommandType::Tool:
		_tool = c.number;
		break;
	case SimCommandType::Spindle:
		_spindle = c.state;
		_rpm = c.rpm;
		break;
	case SimCommandType::Coolant:
		_coolant = c.state;
		break;
	case SimCommandType::Torch:
		_torch = c.state != 0;
		break;
	case SimCommandType::Stop:
		if (c.end)
		{
			// the end of the program: what follows is not played
			_index = _program->size();
			_started = false;
			_state = State::Finished;
		}
		else if (c.optional && !_program->settings().optional_stop)
		{
		}
		else if (!_waits_ignored)
		{
			// the program asks to stop: the player waits for the button
			_stopped_by_program = true;
			_state = State::Paused;
		}
		break;
	default:
		break;
	}
}

void SimPlayer::complete_command()
{
	if (_index >= _program->size())
		return;

	const SimCommand& c = _program->at(_index);
	if (c.moves())
	{
		report(1.0);
		_position = c.target;
	}
	_progress = 0;
	_emitted = 0;
	_started = false;
	_index++;
}

void SimPlayer::update(double seconds, double budget)
{
	if (_program == nullptr || _state != State::Playing)
		return;

	using clock = std::chrono::steady_clock;
	auto deadline = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(budget));
	bool fast = going() || _full_speed;
	_waits_ignored = fast;
	double time = fast ? 0.0 : seconds * _speed;
	int iterations = 0;

	while (_state == State::Playing && _index < _program->size())
	{
		// going to a command: the player pauses when it is reached
		if (_target != none && !_started && _index >= _target)
		{
			_state = _target_end ? State::Finished : State::Paused;
			_target = none;
			_target_end = false;
			break;
		}

		if (!_started)
		{
			begin_command();
			if (_state != State::Playing || _index >= _program->size())
				break;
		}

		const SimCommand& c = _program->at(_index);
		double duration = _program->duration(_index);

		if (fast || duration <= 1e-12)
		{
			complete_command();
		}
		else
		{
			double remaining = duration * (1.0 - _progress);
			if (time >= remaining)
			{
				time -= remaining;
				complete_command();
			}
			else
			{
				_progress += time / duration;
				if (c.moves())
					report(_progress);
				time = 0;
				break;
			}
		}

		// the work of this frame is limited: the interface goes on, and the player goes on in the next frame
		if (fast && (++iterations & 31) == 0 && clock::now() >= deadline)
			break;
	}

	if (_index >= _program->size() && _state == State::Playing)
	{
		_state = (_target != none && !_target_end) ? State::Paused : State::Finished;
		_target = none;
		_target_end = false;
	}
}
