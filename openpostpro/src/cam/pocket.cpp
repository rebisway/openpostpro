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

#include "pocket.h"
#include <imgui.h>
#include <lang.h>

void Pocket::mode(PocketMode value)
{
	if (value != _mode)
	{
		_mode = value;
		compute();
	}
}

void Pocket::finishing(float value)
{
	if (value != _finishing)
	{
		_finishing = value;
		compute();
	}
}

std::vector<Curve> Pocket::zigzag(std::vector<Curve>& curves)
{
	std::vector<Curve> result, temp;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	for (std::vector<Curve> inter : sorted_levels)
	{
		Curve::sort_level(inter);
		auto cw = inter.front().cw();

		// checking iner and outer, the inner contour should exist, otherwise there is an algo issue
		int smaller_level = std::numeric_limits<int>::max();
		int greater_level = std::numeric_limits<int>::max();

		while (inter.size() > 0 && smaller_level == greater_level)
		{
			// sort the list following level, because if different level, we have to make the substrat based on lower level
			Curve::sort_level(inter);

			temp.clear();

			smaller_level = std::numeric_limits<int>::max();
			greater_level = std::numeric_limits<int>::max();

			for (Curve& c : inter)
			{
				auto o = c.offset((c.cw() == cw ? -radius() : radius()));
				smaller_level = glm::min(smaller_level, c.level());
				if (o.size() > 0)
					greater_level = glm::min(greater_level, c.level());
				temp.insert(temp.begin(), o.begin(), o.end());
			}

			if (temp.size() > 1)
			{
				std::vector<Curve>::iterator it1 = temp.begin();
				while (it1 != temp.end() && temp.size() > 1)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != temp.end())
					{
						auto cw1 = (*it1).cw();
						auto cw2 = (*it2).cw();
						std::vector<Curve> merged_curves;

						if ((*it1).level() != (*it2).level())
						{
							if ((*it1).level() < (*it2).level())
								merged_curves = (*it1).boolean_substract((*it2));
							else
								merged_curves = (*it2).boolean_substract((*it1));
						}
						else
							merged_curves = (*it1).boolean_union((*it2));


						// the result keeps the lower level (the outer contour), whatever the order of the two curves
						int merged_level = glm::min((*it1).level(), (*it2).level());
						if (merged = merged_curves.size() > 0)
						{
							for (Curve& m : merged_curves)
							{
								m.close();
								//m.cw(cw1);
								m.level(merged_level);
							}
							temp.erase(it2);
							temp.erase(it1);
							temp.insert(temp.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
						{
							(*it1).cw(cw1);
							(*it1).close();
							(*it2).cw(cw2);
							(*it2).close();
							it2++;
						}
					}
					if (merged)
						it1 = temp.begin();
					else
						it1++;
				}
			}

			auto s = temp.size();

			result.insert(result.end(), temp.begin(), temp.end());
			inter = temp;
		}
	}

	for (auto& c : result)
		c.close();
	return result;
}

std::vector<Curve> Pocket::offset(std::vector<Curve>& curves)
{
	std::vector<Curve> result, temp;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	for (std::vector<Curve> inter : sorted_levels)
	{
		Curve::sort_level(inter);
		auto cw = inter.front().cw();

		// checking iner and outer, the inner contour should exist, otherwise there is an algo issue
		int smaller_level = std::numeric_limits<int>::max();
		int greater_level = std::numeric_limits<int>::max();

		while (inter.size() > 0 && smaller_level == greater_level)
		{
			// sort the list following level, because if different level, we have to make the substrat based on lower level
			Curve::sort_level(inter);

			temp.clear();

			smaller_level = std::numeric_limits<int>::max();
			greater_level = std::numeric_limits<int>::max();

			for (Curve& c : inter)
			{
				auto o = c.offset((c.cw() == cw ? -radius() : radius()));
				smaller_level = glm::min(smaller_level, c.level());
				if (o.size() > 0)
					greater_level = glm::min(greater_level, c.level());
				temp.insert(temp.begin(), o.begin(), o.end());
			}

			if (temp.size() > 1)
			{
				std::vector<Curve>::iterator it1 = temp.begin();
				while (it1 != temp.end() && temp.size() > 1)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != temp.end())
					{
						auto cw1 = (*it1).cw();
						auto cw2 = (*it2).cw();
						std::vector<Curve> merged_curves;

						if ((*it1).level() != (*it2).level())
						{
							if ((*it1).level() < (*it2).level())
								merged_curves = (*it1).boolean_substract((*it2));
							else
								merged_curves = (*it2).boolean_substract((*it1));
						}
						else
							merged_curves = (*it1).boolean_union((*it2));


						// the result keeps the lower level (the outer contour), whatever the order of the two curves
						int merged_level = glm::min((*it1).level(), (*it2).level());
						if (merged = merged_curves.size() > 0)
						{
							for (Curve& m : merged_curves)
							{
								m.close();
								//m.cw(cw1);
								m.level(merged_level);
							}
							temp.erase(it2);
							temp.erase(it1);
							temp.insert(temp.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
						{
							(*it1).cw(cw1);
							(*it1).close();
							(*it2).cw(cw2);
							(*it2).close();
							it2++;
						}
					}
					if (merged)
						it1 = temp.begin();
					else
						it1++;
				}
			}

			auto s = temp.size();

			result.insert(result.end(), temp.begin(), temp.end());
			inter = temp;
		}
	}

	for (auto& c : result)
		c.close();

	return result;
}

std::vector<Curve> Pocket::merge(std::vector<Curve>& curves)
{
	std::vector<Curve> result, temp;

	if (curves.size() == 0)
		return result;

	// we needs to extract curves by couple 1-2, 3-4, 5-6...
	Curve::sort_level(curves);
	int ref = 1;
	std::vector<std::vector<Curve>> sorted_levels;
	sorted_levels.push_back(std::vector<Curve>());
	for (Curve& curve : curves)
	{
		if (curve.level() == ref || curve.level() == ref + 1)
			sorted_levels.back().push_back(curve);
		else
		{
			ref += 2;
			sorted_levels.push_back(std::vector<Curve>());
			sorted_levels.back().push_back(curve);
		}
	}

	for (std::vector<Curve> inter : sorted_levels)
	{
		Curve::sort_level(inter);
		auto cw = inter.front().cw();

		// checking iner and outer, the inner contour should exist, otherwise there is an algo issue
		int smaller_level = std::numeric_limits<int>::max();
		int greater_level = std::numeric_limits<int>::max();

		while (inter.size() > 0 && smaller_level == greater_level)
		{
			// sort the list following level, because if different level, we have to make the substrat based on lower level
			Curve::sort_level(inter);

			temp.clear();

			smaller_level = std::numeric_limits<int>::max();
			greater_level = std::numeric_limits<int>::max();

			for (Curve& c : inter)
			{
				smaller_level = glm::min(smaller_level, c.level());
				temp.push_back(c);
			}

			if (temp.size() > 1)
			{
				std::vector<Curve>::iterator it1 = temp.begin();
				while (it1 != temp.end() && temp.size() > 1)
				{
					bool merged = false;
					std::vector<Curve>::iterator it2 = it1 + 1;
					while (it2 != temp.end())
					{
						auto cw1 = (*it1).cw();
						auto cw2 = (*it2).cw();
						std::vector<Curve> merged_curves;

						if ((*it1).level() != (*it2).level())
						{
							if ((*it1).level() < (*it2).level())
								merged_curves = (*it1).boolean_substract((*it2));
							else
								merged_curves = (*it2).boolean_substract((*it1));
						}
						else
							merged_curves = (*it1).boolean_union((*it2));


						// the result keeps the lower level (the outer contour), whatever the order of the two curves
						int merged_level = glm::min((*it1).level(), (*it2).level());
						if (merged = merged_curves.size() > 0)
						{
							for (Curve& m : merged_curves)
							{
								m.close();
								m.level(merged_level);
							}
							temp.erase(it2);
							temp.erase(it1);
							temp.insert(temp.begin(), merged_curves.begin(), merged_curves.end());
							break;
						}
						else
						{
							(*it1).cw(cw1);
							(*it1).close();
							(*it2).cw(cw2);
							(*it2).close();
							it2++;
						}
					}
					if (merged)
						it1 = temp.begin();
					else
						it1++;
				}
			}

			auto s = temp.size();

			result.insert(result.end(), temp.begin(), temp.end());
			inter = temp;
		}
	}

	for (auto c : result)
		c.close();

	return result;
}

void Pocket::update()
{
	if (_tree != nullptr)
	{
		_computed.clear();
		std::vector<Curve> inner;

		_tree->cw_alter(!cw());

		for (TreeCurve* t : _tree->children)
		{
			bool cw = t->curve->cw();
			_original = t->curves();


			// we first proceed to an offset with the radius
			for (Curve& c : _original)
			{
				c.reduce(0.25f);
				auto curves = c.offset((c.cw() == cw ? -radius() : radius()));
				inner.insert(inner.end(), curves.begin(), curves.end());
			}
			inner = merge(inner);

			// if finishing is set, we offset by finishing
			std::vector<Curve> finish;
			if (_finishing > 0)
			{
				for (Curve& c : inner)
				{
					auto curves = c.offset((c.cw() == cw ? _finishing : -_finishing));
					finish.insert(finish.end(), curves.begin(), curves.end());
				}
			}
			else
				finish.insert(finish.end(), inner.begin(), inner.end());

			std::vector<Curve> process;
			if (finish.size() > 0)
			{
				if (_mode == PocketMode::Zigzag)
					process = zigzag(finish);
				else if (_mode == PocketMode::Offset)
					process = offset(finish);
			}


			_computed.insert(_computed.end(), process.begin(), process.end());
			_computed.insert(_computed.end(), finish.begin(), finish.end());
		}

		generate_data(_computed);
		generate_deco(_computed);
	}
}

void Pocket::scaled()
{
	generate_deco(_computed);
}

std::vector<Curve> Pocket::coordinates()
{
	std::vector<Curve> result;

	for (Curve c : _computed)
	{
		Curve n = c;
		result.push_back(n);
	}
	return result;
}

void Pocket::ui()
{
	//char input[20];

	ImGui::Begin((_name + "###OBJECT").c_str());
	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);


	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("MODE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	const char* items[]{ "Zigzag","Offset" };
	int selected_item = (int)_mode;
	if (ImGui::Combo("##MODE", &selected_item, items, IM_ARRAYSIZE(items)))
	{
		mode((PocketMode)selected_item);
	}


	ImGui::EndTable();
	ImGui::End();
}
