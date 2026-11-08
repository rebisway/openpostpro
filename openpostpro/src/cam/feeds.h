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
#include <string>
#include <vector>
#include "tool.h"

// Cutting data of an end mill by the material machined: a starting point for the feed and speed of a tool, not a guarantee. The values are
// the ones of the catalogues of the makers of carbide end mills for a slot or a pocket (rough), in millimeters. They depend on the
// machine, the rigidity and the tool: the person changes them in the table of the tools after the computation.
struct FeedMaterial
{
	const char* key;			// name for the translation (lang file: MATERIAL_<key>)
	float cutting_speed;		// Vc, in m/min
	float fz[4];				// feed per tooth in mm, for the diameters of FeedTable::diameters
	float pass_ratio;			// depth of a pass, as a part of the diameter
	float plunge_ratio;			// plunge feed, as a part of the cutting feed
	CoolingType cooling;
};

namespace feeds
{
	// diameters of the columns of fz
	extern const float diameters[4];

	const std::vector<FeedMaterial>& materials();

	// the result of the computation
	struct Result
	{
		float spindle_speed = 0;	// rpm
		float feed = 0;				// mm/min
		float plunge_feed = 0;
		float pass = 0;				// mm
		CoolingType cooling = CoolingType::None;
		bool limited = false;		// the speed of the spindle was limited by the maximum of the machine
	};

	// feed per tooth for a diameter (linear between the columns, the first and the last one outside)
	float feed_per_tooth(const FeedMaterial& material, float diameter);

	// n = Vc * 1000 / (pi * D) limited to max_rpm, F = n * z * fz. A diameter or a number of teeth which is not positive gives zeros.
	Result compute(const FeedMaterial& material, float diameter, int flutes, float max_rpm = 24000);

	// applies the result to a tool (type mill)
	void apply(const Result& result, Tool& tool);
}
