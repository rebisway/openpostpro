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
#include "feeds.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace feeds
{
	const float diameters[4] = { 3.0f, 6.0f, 12.0f, 20.0f };

	// fz for 3, 6, 12 and 20 mm. The ratio of the pass is for a slot at full width: it is smaller for the hard materials.
	const std::vector<FeedMaterial>& materials()
	{
		static const std::vector<FeedMaterial> table =
		{
			{ "ALUMINIUM",  250, { 0.020f, 0.050f, 0.080f, 0.100f }, 0.50f, 0.4f, CoolingType::Mist },
			{ "BRASS",      150, { 0.015f, 0.040f, 0.070f, 0.090f }, 0.50f, 0.4f, CoolingType::None },
			{ "STEEL",      100, { 0.010f, 0.030f, 0.050f, 0.070f }, 0.25f, 0.3f, CoolingType::Flood },
			{ "STAINLESS",   60, { 0.008f, 0.020f, 0.040f, 0.050f }, 0.20f, 0.3f, CoolingType::Flood },
			{ "HARDWOOD",   300, { 0.030f, 0.070f, 0.120f, 0.150f }, 1.00f, 0.5f, CoolingType::None },
			{ "SOFTWOOD",   400, { 0.040f, 0.090f, 0.150f, 0.180f }, 1.00f, 0.5f, CoolingType::None },
			{ "MDF",        350, { 0.040f, 0.090f, 0.150f, 0.180f }, 1.00f, 0.5f, CoolingType::None },
			{ "PLASTIC",    250, { 0.030f, 0.060f, 0.100f, 0.120f }, 0.50f, 0.4f, CoolingType::None },
		};
		return table;
	}

	float feed_per_tooth(const FeedMaterial& material, float diameter)
	{
		if (diameter <= diameters[0])
			return material.fz[0];
		for (int i = 1; i < 4; i++)
			if (diameter <= diameters[i])
			{
				float t = (diameter - diameters[i - 1]) / (diameters[i] - diameters[i - 1]);
				return material.fz[i - 1] + (material.fz[i] - material.fz[i - 1]) * t;
			}
		return material.fz[3];
	}

	Result compute(const FeedMaterial& material, float diameter, int flutes, float max_rpm)
	{
		Result r;
		if (diameter <= 0 || flutes <= 0)
			return r;
		float n = material.cutting_speed * 1000.0f / (glm::pi<float>() * diameter);
		if (max_rpm > 0 && n > max_rpm)
		{
			n = max_rpm;
			r.limited = true;
		}
		r.spindle_speed = std::round(n);
		r.feed = std::round(n * flutes * feed_per_tooth(material, diameter));
		r.plunge_feed = std::round(r.feed * material.plunge_ratio);
		r.pass = std::round(diameter * material.pass_ratio * 100.0f) / 100.0f;
		r.cooling = material.cooling;
		return r;
	}

	void apply(const Result& result, Tool& tool)
	{
		tool.spindle_speed = result.spindle_speed;
		tool.feed = result.feed;
		tool.plunge_feed = result.plunge_feed;
		tool.pass = result.pass;
		tool.cooling = result.cooling;
	}
}
