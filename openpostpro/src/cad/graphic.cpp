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
#ifndef _GRAPHIC_H
#define _GRAPHIC_H

#include "graphic.h"



void Graphic::name(std::string n)
{
	//if (!_name.empty() && _name != n) 
	//	History::undo(HistoryActionType::Modify, write()); 
	_name = n;
}

bool Graphic::shape()
{
	return (int)type() >= 100 && (int)type() < 200;
}

bool Graphic::toolpath()
{
	return (int)type() >= 200 && (int)type() < 300;
}

void Graphic::compute()
{
	_needs_update = true;
}

void Graphic::scaled()
{

}

void Graphic::draw()
{
	if (_needs_update)
	{
		_needs_update = false;
		update();
	}
}



#endif
