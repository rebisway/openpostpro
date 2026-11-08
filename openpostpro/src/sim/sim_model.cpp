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

#include "sim_model.h"
#include "sim_model_grid.h"
#include "sim_model_tiles.h"

// The methods, in the order of the choice of the interface
int SimModel::method_count()
{
	return 2;
}

const char* SimModel::method_name(int method)
{
	switch (method)
	{
	case 1: return "Tiles";
	default: return "Grid";
	}
}

std::unique_ptr<SimModel> SimModel::create(int method)
{
	switch (method)
	{
	case 1: return std::make_unique<SimModelTiles>();
	default: return std::make_unique<SimModelGrid>();
	}
}
