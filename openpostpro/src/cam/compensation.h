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

// Tool radius compensation (G41 / G42), the way it is used for a milling cutter or for the kerf of a plasma torch.
// The user tells on which side of the contour the tool is (outside or inside the shape). The machine needs the side of the
// tool relative to the direction of the movement (left or right). The second one is deduced from the first one and from
// the direction of the path.

#pragma once
#ifndef _COMPENSATION_H
#define _COMPENSATION_H

// Property of a toolpath: where the tool is, relative to the shape which is followed
enum class ToolSide
{
	Centered,	// the tool follows the line: no compensation
	Outside,	// the tool is outside of the shape
	Inside		// the tool is inside of the shape
};

// What the post-processor asks to the machine: the side of the tool relative to the direction of the movement
enum class ToolRadiusCompensation
{
	None,		// no compensation
	Left,		// the tool is on the left of the movement (G41)
	Right		// the tool is on the right of the movement (G42)
};

// The compensation for a tool outside or inside of a shape followed in the given direction (seen from above, Y up).
// An open path is considered closed by a straight line to get its direction, as the follow direction does.
//   counter-clockwise: the inside of the shape is on the left of the movement, the outside on the right
//   clockwise: the inside of the shape is on the right of the movement, the outside on the left
inline ToolRadiusCompensation compensation_for(ToolSide side, bool clockwise)
{
	switch (side)
	{
	case ToolSide::Outside:
		return clockwise ? ToolRadiusCompensation::Left : ToolRadiusCompensation::Right;
	case ToolSide::Inside:
		return clockwise ? ToolRadiusCompensation::Right : ToolRadiusCompensation::Left;
	default:
		return ToolRadiusCompensation::None;
	}
}

#endif
