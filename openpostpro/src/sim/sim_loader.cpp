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

#include "sim_loader.h"
#include <chrono>

std::vector<std::string> SimLoader::split_lines(const std::string& text)
{
	std::vector<std::string> lines;
	size_t start = 0;
	while (start <= text.size())
	{
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		std::string line = text.substr(start, end - start);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		// the end of the text does not make an empty line
		if (end == text.size() && line.empty())
			break;
		lines.push_back(line);
		start = end + 1;
	}
	return lines;
}

bool SimLoader::begin(SimTranslator* translator, SimProgram* program, const std::string& text, const std::map<std::string, double>& options)
{
	_translator = translator;
	_program = program;
	_lines = split_lines(text);
	_next = 0;
	_cancelled = false;
	_error.clear();
	_active = false;
	_program->clear();

	std::string error;
	if (!_translator->start(options, error))
	{
		_error = error;
		return false;
	}
	_active = true;
	return true;
}

bool SimLoader::step(double budget)
{
	if (!_active)
		return true;

	using clock = std::chrono::steady_clock;
	auto deadline = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(budget));

	std::vector<SimCommand> commands;
	int count = 0;
	while (_next < _lines.size() && !_cancelled)
	{
		commands.clear();
		_translator->translate(_lines[_next], (int)_next + 1, commands);
		_program->add(commands);
		_next++;

		if ((++count & 15) == 0 && clock::now() >= deadline)
			break;
	}

	if (_cancelled || _next >= _lines.size())
	{
		_translator->close();
		_active = false;
		return true;
	}
	return false;
}

void SimLoader::cancel()
{
	if (_active)
	{
		_cancelled = true;
		_translator->close();
		_active = false;
	}
}
