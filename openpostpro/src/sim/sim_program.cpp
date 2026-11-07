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

#include "sim_program.h"
#include "sim_geometry.h"
#include <algorithm>
#include <cmath>

void SimProgram::clear()
{
	_commands.clear();
	_times.clear();
	_stats = SimStats();
}

double SimProgram::move_time(const SimCommand& command, const glm::dvec3& from, const SimSettings& settings)
{
	switch (command.type)
	{
	case SimCommandType::Rapid:
	{
		double length = glm::distance(from, command.target);
		bool only_z = glm::distance(glm::dvec2(from), glm::dvec2(command.target)) < 1e-9;
		double speed = only_z ? settings.rapid_z_speed : settings.rapid_speed;
		return speed > 0 ? length / speed * 60.0 : 0.0;
	}
	case SimCommandType::Linear:
	case SimCommandType::Arc:
	{
		double length = SimPath(from, command).length();
		double feed = command.feed > 0 ? command.feed : 1.0;
		return length / feed * 60.0;
	}
	case SimCommandType::Dwell:
		return std::max(0.0, command.seconds);
	default:
		return 0.0;
	}
}

void SimProgram::analyze(const SimSettings& settings)
{
	_settings = settings;
	_stats = SimStats();
	_times.assign(_commands.size(), 0.0);

	glm::dvec3 position = settings.start_position;
	_stats.low = _stats.high = position;
	double time = 0;

	for (size_t i = 0; i < _commands.size(); i++)
	{
		const SimCommand& c = _commands[i];

		if (c.moves())
		{
			SimPath path(position, c);
			glm::dvec3 low, high;
			path.bounds(low, high);
			_stats.low = glm::min(_stats.low, low);
			_stats.high = glm::max(_stats.high, high);
			_stats.moves++;

			if (c.type == SimCommandType::Rapid)
				_stats.rapid_length += path.length();
			else
			{
				_stats.feed_length += path.length();
				if (!_stats.has_cut)
				{
					_stats.cut_low = low;
					_stats.cut_high = high;
					_stats.has_cut = true;
				}
				else
				{
					_stats.cut_low = glm::min(_stats.cut_low, low);
					_stats.cut_high = glm::max(_stats.cut_high, high);
				}
			}
			time += move_time(c, position, settings);
			position = c.target;
		}
		else if (c.type == SimCommandType::Dwell)
			time += move_time(c, position, settings);
		else if (c.type == SimCommandType::Message)
		{
			if (c.level == SimLevel::Error) _stats.errors++;
			else if (c.level == SimLevel::Warning) _stats.warnings++;
			else _stats.infos++;
		}
		else if (c.type == SimCommandType::Custom)
			_stats.unknown++;

		_times[i] = time;
	}
	_stats.total_time = time;
}

size_t SimProgram::command_at_line(int line) const
{
	for (size_t i = 0; i < _commands.size(); i++)
		if (_commands[i].line >= line)
			return i;
	return _commands.size();
}
