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
#ifndef _FOLLOW_H
#define _FOLLOW_H

#include "toolpath.h"

class Follow : public Toolpath
{
private:

public:
	GraphicType type() override { return GraphicType::CamFollow; }
	
	Follow(Renderer* r) : Toolpath(r) {}

	bool start_point_allowed() override { return true; }

	void update() override;
	void scaled() override;
	std::vector<Curve> coordinates() override;

	void ui() override;

	// IO
	std::string write() override;
	void read(std::string value, float version = 0) override;

};

#endif