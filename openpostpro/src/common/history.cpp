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

#include <history.h>

std::mutex History::_mutex;
std::vector<HistoryAction> History::_undo;
std::vector<HistoryAction> History::_redo;

bool History::has_undo() { return _undo.size() > 0; }

bool History::has_redo() { return _redo.size() > 0; }

size_t History::undo_count() { return _undo.size(); }

size_t History::redo_count() { return _redo.size(); }


void History::undo(HistoryActionType type, std::string value, bool clear_redo)
{
	if (type != HistoryActionType::None)
	{
		_mutex.lock();
		_undo.push_back(HistoryAction(type, value));
		if (clear_redo)
			_redo.clear();
		_mutex.unlock();
	}
}

void History::undo(HistoryAction action, bool clear_redo)
{
	if (action.type != HistoryActionType::None)
	{
		_mutex.lock();
		_undo.push_back(action);
		if (clear_redo)
			_redo.clear();
		_mutex.unlock();
	}
}

void History::redo(HistoryActionType type, std::string value)
{
	if (type != HistoryActionType::None)
	{
		_mutex.lock();
		_redo.push_back(HistoryAction(type, value));
		_mutex.unlock();
	}
}

void History::redo(HistoryAction action)
{
	if (action.type != HistoryActionType::None)
	{
		_mutex.lock();
		_redo.push_back(action);
		_mutex.unlock();
	}
}

void History::begin_undo_record(bool clear_redo)
{
	undo(HistoryActionType::BeginRecord, "", clear_redo);
}

void History::end_undo_record(bool clear_redo)
{
	undo(HistoryActionType::EndRecord, "", clear_redo);
}

void History::begin_redo_record()
{
	redo(HistoryActionType::BeginRecord, "");
}

void History::end_redo_record()
{
	redo(HistoryActionType::EndRecord, "");
}

HistoryAction History::undo()
{
	if (_undo.size() > 0)
	{
		_mutex.lock();
		auto action = _undo.back();
		_undo.pop_back();
		_mutex.unlock();
		return action;
	}
	else
		return HistoryAction(HistoryActionType::None, "");
}

HistoryAction History::redo()
{
	if (_redo.size() > 0)
	{
		_mutex.lock();
	auto action = _redo.back();
	_redo.pop_back();
	_mutex.unlock();
	return action;
	}
	else
		return HistoryAction(HistoryActionType::None, "");
}

void History::clear()
{
	_mutex.lock();
	_undo.clear();
	_redo.clear();
	_mutex.unlock();
}
