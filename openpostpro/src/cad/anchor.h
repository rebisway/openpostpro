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

#pragma once
#include <graphic.h>
#include <geometry.h>

#define ANCHOR_SIZE 2.5f

class Anchor : public Graphic
{
private:
	glm::vec2 _point = geometry::vec2_empty;
	float _scale;
	Buffer* _triangles;
	bool _is_over = false;

public:
	glm::vec2 point() { return _point; }
	void point(glm::vec2 value) { _point = value; update(); }

	Anchor(Renderer* r);
	~Anchor();

	bool is_over(glm::vec2 point);

	void update() override;
	void draw() override;
};