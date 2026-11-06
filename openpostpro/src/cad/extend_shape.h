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

// Extend on the shapes of the drawing: finds the line or arc under a point and where it would stop.
// The geometry itself is in extend.h; this part only knows how to read the shapes of the layers.

#pragma once
#ifndef _EXTEND_SHAPE_H
#define _EXTEND_SHAPE_H

#include <layer.h>
#include <vector>

namespace cad_extend
{
	// What a click at `point` would extend: the visible line or arc under the point, the end of it which is nearest to
	// the point (at_start), and the point where this end stops (result).
	// Returns 0 if there is no line or arc under the point, 1 if nothing stops the prolongation, 2 if found.
	int find(std::vector<Layer*>& layers, glm::vec2 point, Shape*& target, bool& at_start, glm::vec2& result);
}

#endif
