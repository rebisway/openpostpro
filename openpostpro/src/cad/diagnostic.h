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

// Diagnostic: is the drawing ready for machining? It looks for what makes a toolpath wrong or impossible:
// - a shape of length zero (nothing to machine),
// - two shapes at the same place (the toolpath would be machined twice),
// - a free end: an end of a line, arc, polyline or spline which touches no other shape (an open contour).

#pragma once
#ifndef _DIAGNOSTIC_H
#define _DIAGNOSTIC_H

#include <layer.h>
#include <vector>

namespace cad_diag
{
	enum class IssueType
	{
		ZeroLength,
		Duplicate,
		FreeEnd
	};

	struct Issue
	{
		IssueType type = IssueType::FreeEnd;
		Shape* shape = nullptr;		// the shape with the problem (for a duplicate, the second one: the one to delete)
		glm::vec2 point;			// where to look: the end, or the first point of the shape
	};

	// Looks at the shapes of the visible layers. `tolerance` is the distance under which two points are the same point.
	std::vector<Issue> check(std::vector<Layer*>& layers, float tolerance = 0.01f);

	// The contour of a selection: the shapes of the selection and every shape of the visible layers which is connected to them, directly or
	// through other shapes (an end of one touches an end of the other, within `tolerance`). The shapes are not modified.
	// The selection comes first, in its order. Shapes without ends (circles, ellipses, points, texts) are only returned when selected.
	std::vector<Shape*> contour(const std::vector<Shape*>& selection, std::vector<Layer*>& layers, float tolerance = 0.01f);

	// One line of text with the number of problems of each type ("" if there is none)
	// (the texts of the types are given by the caller, in the language of the user)
	std::string summary(const std::vector<Issue>& issues, std::string zero_length, std::string duplicate, std::string free_end);
}

#endif
