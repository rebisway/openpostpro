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
#ifndef _MESSAGE_BOX_H
#define _MESSAGE_BOX_H

#include <imgui.h>
#include <string>

enum class MessageBoxStatus
{
	None,
	Ok,
	Cancel,
	Yes,
	No
};

enum class MessageBoxButtons
{
	Ok,
	OkCancel,
	YesNo
};

class MessageBox
{
private:
	bool _enabled=false;
	bool _input=false;

	std::string _title;
	std::string _text;
	std::string _result;

	MessageBoxButtons _buttons = MessageBoxButtons::Ok;

public:
	std::string result();

	void input(std::string title, std::string text, std::string value = "", MessageBoxButtons buttons = MessageBoxButtons::Ok);
	void display(std::string title, std::string text, MessageBoxButtons buttons = MessageBoxButtons::Ok);
	MessageBoxStatus show();
};

#endif