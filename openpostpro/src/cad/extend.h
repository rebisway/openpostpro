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

// Extend: computes where a line or an arc stops when it is prolonged until it meets other entities.
// Pure geometry, without any shape or window: the entities to meet are given as curves.

#pragma once
#ifndef _EXTEND_H
#define _EXTEND_H

#include <curve.h>
#include <vector>

namespace cad_extend
{
	// Prolongs the line from `fixed` through `end`, beyond `end`, until the nearest intersection with the boundaries.
	// An intersection at `end` itself (the entity already touches it) is ignored.
	// result: the new end of the line. Returns false if nothing is met.
	bool line(glm::vec2 fixed, glm::vec2 end, std::vector<Curve>& boundaries, glm::vec2& result);

	// Prolongs the arc described from `start` to `stop` around `center` (clockwise if cw), beyond `stop`, in the same
	// direction, until the nearest intersection with the boundaries. To prolong the start, give the arc reversed
	// (start and stop swapped, cw inverted). result: the new stop of the arc. Returns false if nothing is met.
	bool arc(glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw, std::vector<Curve>& boundaries, glm::vec2& result);
}

#endif
