// OpenPostPro - www.openpostpro.org
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

/************************************************************************
* Register history events
*************************************************************************/
#pragma once
#ifndef _HISTORY_H
#define _HISTORY_H

#include <string>
#include <mutex>
#include <vector>

enum class HistoryActionType
{
	Delete,
	Add,
	Modify,
	BeginRecord,
	EndRecord,
	None
};

struct HistoryAction
{
	HistoryActionType type = HistoryActionType::None;
	std::string value = "";

	HistoryAction()
	{
		type = HistoryActionType::Add;
		value = "";
	}

	HistoryAction(HistoryActionType type, std::string value)
	{
		this->type = type;
		this->value = value;
	}

	HistoryAction& operator=(HistoryAction& right)
	{
		type = right.type;
		value = right.value;
		return *this;
	}

	HistoryAction& operator=(HistoryAction right)
	{
		type = right.type;
		value = right.value;
		return *this;
	}
};

class History
{
private:
	static std::mutex _mutex;
	static std::vector<HistoryAction> _undo;
	static std::vector<HistoryAction> _redo;

public:
	static bool has_undo();

	static bool has_redo();

	static size_t undo_count();

	static size_t redo_count();

	/// <summary>
	/// push undo action
	/// </summary>
	static void undo(HistoryActionType type, std::string value, bool clear_redo = true);

	/// <summary>
	/// push undo action
	/// </summary>
	static void undo(HistoryAction action, bool clear_redo=true);

	/// <summary>
	/// push redo action
	/// </summary>
	static void redo(HistoryActionType type, std::string value);

	/// <summary>
	/// push redo action
	/// </summary>
	static void redo(HistoryAction action);

	/// <summary>
	/// start multiple HistoryActionType to be processed
	/// </summary>
	static void begin_undo_record(bool clear_redo = true);

	/// <summary>
	/// end multiple HistoryActionType to be processed
	/// </summary>
	static void end_undo_record(bool clear_redo = true);

	/// <summary>
	/// start multiple HistoryActionType to be processed
	/// </summary>
	static void begin_redo_record();

	/// <summary>
	/// end multiple HistoryActionType to be processed
	/// </summary>
	static void end_redo_record();

	/// <summary>
	/// pop undo action
	/// </summary>
	static HistoryAction undo();

	/// <summary>
	/// pop redo action
	/// </summary>
	static HistoryAction redo();

	/// <summary>
	/// Clear history file
	/// </summary>
	static void clear();
};

#endif