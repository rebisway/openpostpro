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

#include <shape.h>
#include <history.h>
#include <imgui.h>
#include <logger.h>
#include <lang.h>
#include "strings.h"

float Shape::precision = PRECISION;

int Shape::tag()
{
	return _tag;
}

void Shape::tag(int value)
{
	_tag = value;
}

std::string Shape::parent()
{
	return _parent;
}

std::string Shape::block()
{
	return _block;
}

void Shape::block(std::string value)
{
	_block = value;
}

void Shape::parent(std::string value)
{
	if (_parent != value )
	{
		_parent = value;
		//if (done())
		//	History::undo(HistoryActionType::Modify, write());
	}
}

bool Shape::over()
{
	return _over;
}

void Shape::over(bool value)
{
	_over = value;
}

geometry::rectangle& Shape::bounds()
{
	return _bounds;
}

glm::vec2& Shape::topLeft()
{
	return _bounds.top_left;
}

glm::vec2& Shape::bottomRight()
{
	return _bounds.bottom_right;
}
