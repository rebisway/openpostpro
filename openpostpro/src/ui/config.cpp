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

#include "config.h"
#include <lang.h>
#include <geometry.h>
#include <environment.h>

Config config;

void Config::read()
{
	load_colors();


	first = _ini.get_bool(_section, "First", true);
	display_style = _ini.get_int(_section, "Style", 1);
	open_last_file = _ini.get_bool(_section, "OpenLastFile", true);
	last_file_path = _ini.get_string(_section, "LastFilePath");
	small_axe = _ini.get_bool(_section, "SmallAxe", false);
	use_grid = _ini.get_bool(_section, "UseGrid", false);
	inc_grid = _ini.get_float(_section, "IncGrid", 10);
	show_decoration = _ini.get_bool(_section, "ShowDecoration", false);
	show_point_as_cross = _ini.get_bool(_section, "ShowPointAsCross", false);
	show_cam_arrow = _ini.get_bool(_section, "ShowCamArrow", true);
	show_cam_start = _ini.get_bool(_section, "ShowCamStart", true);
	postpro = _ini.get_string(_section, "Postpro", "");
	display_log = _ini.get_bool(_section, "DisplayLog", false);
	display_output = _ini.get_bool(_section, "DisplayOutput", false);

	python_path = _ini.get_string(_section, "PythonPath", "");
	language = _ini.get_string(_section, "Language", "");
	default_unit = _ini.get_int(_section, "Unit", 0);

	anchorFillColor = geometry::from_string(_ini.get_string("COLOR", "AnchorFill", "(1.0;1.0;1.0;0.5)"));
	anchorLineColor = geometry::from_string(_ini.get_string("COLOR", "AnchorLine", "(0.0;1.0;0.0;0.5)"));			//glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	anchorOverFillColor = geometry::from_string(_ini.get_string("COLOR", "AnchorOverFill", "(0.0;1.0;0.0;0.5)"));	//glm::vec4(0.0f, 1.0f, 0.0f, 0.5f);
	anchorOverLineColor = geometry::from_string(_ini.get_string("COLOR", "AnchorOverLine", "(0.0;1.0;0.0;1.0)"));	//glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	decorationColor = geometry::from_string(_ini.get_string("COLOR", "Decoration", "(0.871;0.871;0.871;0.5)"));		//glm::vec4(0.871f, 0.871f, 0.871f, 0.5f);
	mouseOverColor = geometry::from_string(_ini.get_string("COLOR", "MouseOver", "(0.0;1.0;0.0;1.0)"));				//glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	axeColor = geometry::from_string(_ini.get_string("COLOR", "Axe", "(1.0;1.0;1.0;1.0)"));							//glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	gridColor = geometry::from_string(_ini.get_string("COLOR", "Grid", "(1.0;1.0;1.0;0.5)"));						//glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
	background2DColor = geometry::from_string(_ini.get_string("COLOR", "Background", "(0.25;0.25;0.25;0.0)"));		//glm::vec4(0.25f, 0.25f, 0.25f, 0.0f);
	layerColor = geometry::from_string(_ini.get_string("COLOR", "Layer", "(0.278;0.78;1.0;1.0)"));					//glm::vec4(0.278f, 0.78f, 1.0f, 1.0f);
	groupColor = geometry::from_string(_ini.get_string("COLOR", "Group", "(1;0.54902;0.0;1)"));				//glm::vec4(0.173f, 0.271f, 0.631f, 1.0f);
	selectedCadColor = geometry::from_string(_ini.get_string("COLOR", "Selected", "(0.0;0.8;0.0;1.0)"));				//glm::vec4(0.0f, 0.8f, 0.0f, 1.0f);

	chamfer_radius = _ini.get_float("STATE", "ChamferRadius", 1.0f);
}


void Config::write()
{
	_ini.set(_section, "First", false);
	_ini.set(_section, "Style", display_style);
	_ini.set(_section, "OpenLastFile", open_last_file);
	_ini.set(_section, "LastFilePath", last_file_path);
	_ini.set(_section, "SmallAxe", small_axe);
	_ini.set(_section, "UseGrid", use_grid);
	_ini.set(_section, "IncGrid", inc_grid);
	_ini.set(_section, "ShowDecoration", show_decoration);
	_ini.set(_section, "ShowPointAsCross", show_point_as_cross);
	_ini.set(_section, "ShowCamArrow", show_cam_arrow);
	_ini.set(_section, "ShowCamStart", show_cam_start);
	_ini.set(_section, "Postpro", postpro);
	_ini.set(_section, "DisplayLog", display_log);
	_ini.set(_section, "DisplayOutput", display_output);
	_ini.set(_section, "PythonPath", python_path);
	_ini.set(_section, "Language", language);
	_ini.set(_section, "Unit", default_unit);

	_ini.set("COLOR", "AnchorFill", geometry::to_string(anchorFillColor));
	_ini.set("COLOR", "AnchorLine", geometry::to_string(anchorLineColor));
	_ini.set("COLOR", "AnchorOverFill", geometry::to_string(anchorOverFillColor));
	_ini.set("COLOR", "AnchorOverLine", geometry::to_string(anchorOverLineColor));
	_ini.set("COLOR", "Decoration", geometry::to_string(decorationColor));
	_ini.set("COLOR", "MouseOver", geometry::to_string(mouseOverColor));
	_ini.set("COLOR", "Axe", geometry::to_string(axeColor));
	_ini.set("COLOR", "Grid", geometry::to_string(gridColor));
	_ini.set("COLOR", "Background", geometry::to_string(background2DColor));
	_ini.set("COLOR", "Layer", geometry::to_string(layerColor));
	_ini.set("COLOR", "Group", geometry::to_string(groupColor));
	_ini.set("COLOR", "Selected", geometry::to_string(selectedCadColor));

	_ini.set("STATE", "ChamferRadius", chamfer_radius);



	_ini.write();
}


MessageBoxResult Config::drawUI()
{
	MessageBoxResult result = MessageBoxResult::None;

	if (_load_temp)
	{
		write();
		_ini_temp.load_data(_ini.get_data());
		_load_temp = false;
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin(Lang::l("PREFERENCES"), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

	// GENERAL
	if (ImGui::BeginTabBar("PREFERENCES_TAB_BAR", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem(Lang::l("GENERAL")))
		{
			bool b = _ini_temp.get_bool("GENERAL", "OpenLastFile");
			if (ImGui::Checkbox(Lang::l("LOAD_LAST_FILE"), &b))
			{
				_ini_temp.set("GENERAL", "OpenLastFile", b);
			}
			b = _ini_temp.get_bool("GENERAL", "SmallAxe");
			if (ImGui::Checkbox(Lang::l("SMALL_AXE"), &b))
			{
				_ini_temp.set("GENERAL", "SmallAxe", b);
			}
			b = _ini_temp.get_bool("GENERAL", "UseGrid");
			if (ImGui::Checkbox(Lang::l("USE_GRID"), &b))
			{
				_ini_temp.set("GENERAL", "UseGrid", b);
			}
			ImGui::SameLine();
			float f = _ini_temp.get_float("GENERAL", "IncGrid");
			if (ImGui::InputFloat("##INC_GRID", &f))
			{
				_ini_temp.set("GENERAL", "IncGrid", f);
			}
			b = _ini_temp.get_bool("GENERAL", "ShowDecoration");
			if (ImGui::Checkbox(Lang::l("SHOW_DECORATION"), &b))
			{
				_ini_temp.set("GENERAL", "ShowDecoration", b);
			}
			b = _ini_temp.get_bool("GENERAL", "ShowPointAsCross");
			if (ImGui::Checkbox(Lang::l("SHOW_POINT_AS_CROSS"), &b))
			{
				_ini_temp.set("GENERAL", "ShowPointAsCross", b);
			}
			b = _ini_temp.get_bool("GENERAL", "ShowCamArrow");
			if (ImGui::Checkbox(Lang::l("SHOW_CAM_ARROW"), &b))
			{
				_ini_temp.set("GENERAL", "ShowCamArrow", b);
			}
			b = _ini_temp.get_bool("GENERAL", "ShowCamStart");
			if (ImGui::Checkbox(Lang::l("SHOW_CAM_START"), &b))
			{
				_ini_temp.set("GENERAL", "ShowCamStart", b);
			}
			char input[1024];
			sprintf_s(input, "%s", _ini_temp.get_string("GENERAL", "PythonPath").c_str());
			if (ImGui::InputText(Lang::l("PYTHON_PATH"), input, 20 * sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				_ini_temp.set("GENERAL", "PythonPath", std::string(input));
			}

			// language: the code of a file of the lang folder
			std::string current = _ini_temp.get_string("GENERAL", "Language");
			if (current.empty())
				current = Lang::getName();
			if (ImGui::BeginCombo(Lang::l("LANGUAGE"), current.c_str()))
			{
				for (const std::string& code : Lang::get_langs())
				{
					const bool is_selected = (code == current);
					if (ImGui::Selectable(code.c_str(), is_selected))
						_ini_temp.set("GENERAL", "Language", code);
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// unit given to the new documents
			int unit = _ini_temp.get_int("GENERAL", "Unit", 0);
			const char* units[] = { Lang::l("UNIT_MM"), Lang::l("UNIT_INCH") };
			if (ImGui::Combo(Lang::l("DEFAULT_UNIT"), &unit, units, IM_ARRAYSIZE(units)))
				_ini_temp.set("GENERAL", "Unit", unit);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	// COLORS
	if (ImGui::BeginTabBar("PREFERENCES_TAB_BAR", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem(Lang::l("COLOR")))
		{
			glm::vec4 c = geometry::from_string(_ini_temp.get_string("COLOR", "AnchorLine"));
			if (ImGui::ColorEdit4("Anchor", (float*)&c))
			{
				_ini_temp.set("COLOR", "AnchorLine", geometry::to_string(c));
				c.a /= 2;
				_ini_temp.set("COLOR", "AnchorFill", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Decoration"));
			if (ImGui::ColorEdit4("Decoration", (float*)&c))
			{
				_ini_temp.set("COLOR", "Decoration", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "MouseOver"));
			if (ImGui::ColorEdit4("Mouse Over", (float*)&c))
			{
				_ini_temp.set("COLOR", "MouseOver", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Axe"));
			if (ImGui::ColorEdit4("Axe", (float*)&c))
			{
				_ini_temp.set("COLOR", "Axe", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Grid"));
			if (ImGui::ColorEdit4("Grid", (float*)&c))
			{
				_ini_temp.set("COLOR", "Grid", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Background"));
			if (ImGui::ColorEdit4("Background", (float*)&c))
			{
				_ini_temp.set("COLOR", "Background", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Layer"));
			if (ImGui::ColorEdit4("Default layer", (float*)&c))
			{
				_ini_temp.set("COLOR", "Layer", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Group"));
			if (ImGui::ColorEdit4("Default group", (float*)&c))
			{
				_ini_temp.set("COLOR", "Group", geometry::to_string(c));
			}
			c = geometry::from_string(_ini_temp.get_string("COLOR", "Selected"));
			if (ImGui::ColorEdit4("Selected", (float*)&c))
			{
				_ini_temp.set("COLOR", "Selected", geometry::to_string(c));
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}


	//------------------------------ OK / CANCEL --------------------------------------//

	ImGui::NewLine();
	ImGuiStyle& style = ImGui::GetStyle();
	float windowWidth = ImGui::GetWindowSize().x;
	float textWidth = std::max(ImGui::CalcTextSize(Lang::l("OK")).x, ImGui::CalcTextSize(Lang::l("CANCEL")).x);
	ImGui::SetCursorPosX((windowWidth - (2 * textWidth + style.ItemSpacing.x)) * 0.5f);
	if (ImGui::Button("Ok", ImVec2(textWidth, 0.0f)))
	{
		_ini.load_data(_ini_temp.get_data());
		_ini.write();
		_load_temp = true;
		read();
		if (!language.empty() && language != Lang::getName())
			Lang::load(language);
		result = MessageBoxResult::Ok;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(textWidth, 0.0f)))
	{
		_load_temp = true;
		result = MessageBoxResult::Cancel;
	}

	ImGui::End();

	return result;
}


void Config::load_colors()
{
	dxfColorNames.push_back("Black");
	dxfColorNames.push_back("Red");
	dxfColorNames.push_back("Yellow");
	dxfColorNames.push_back("Green");
	dxfColorNames.push_back("Cyan");
	dxfColorNames.push_back("Blue");
	dxfColorNames.push_back("Magenta");
	dxfColorNames.push_back("Black");
	dxfColorNames.push_back("Gray");
	dxfColorNames.push_back("Brown");
	dxfColorNames.push_back("Khaki");
	dxfColorNames.push_back("DarkGreen");
	dxfColorNames.push_back("SteelBlue");
	dxfColorNames.push_back("DarkBlue");
	dxfColorNames.push_back("Purple");
	dxfColorNames.push_back("DarkGray");
	dxfColorNames.push_back("White");
	dxfColorNames.push_back("LightGray");

	vividColorNames.push_back("Red");
	vividColorNames.push_back("Yellow");
	vividColorNames.push_back("White");
	vividColorNames.push_back("Blue");
	vividColorNames.push_back("Aqua");
	vividColorNames.push_back("Dark blue");
	vividColorNames.push_back("Magenta");
	vividColorNames.push_back("Coral");
	vividColorNames.push_back("Fuchsia");
	vividColorNames.push_back("Turquoise");
	vividColorNames.push_back("BlueViolet");
	vividColorNames.push_back("Gainsboro");
	vividColorNames.push_back("Pink");
	vividColorNames.push_back("YellowGreen");

	colors["AliceBlue"] = glm::vec4(0.941176f, 0.972549f, 1.000000f, 1.000000f);
	colors["AntiqueWhite"] = glm::vec4(0.980392f, 0.921569f, 0.843137f, 1.000000f);
	colors["Aqua"] = glm::vec4(0.000000f, 1.000000f, 1.000000f, 1.000000f);
	colors["Aquamarine"] = glm::vec4(0.498039f, 1.000000f, 0.831373f, 1.000000f);
	colors["Azure"] = glm::vec4(0.941176f, 1.000000f, 1.000000f, 1.000000f);
	colors["Beige"] = glm::vec4(0.960784f, 0.960784f, 0.862745f, 1.000000f);
	colors["Bisque"] = glm::vec4(1.000000f, 0.894118f, 0.768627f, 1.000000f);
	colors["Black"] = glm::vec4(0.000000f, 0.000000f, 0.000000f, 1.000000f);
	colors["BlanchedAlmond"] = glm::vec4(1.000000f, 0.921569f, 0.803922f, 1.000000f);
	colors["Blue"] = glm::vec4(0.000000f, 0.000000f, 1.000000f, 1.000000f);
	colors["BlueViolet"] = glm::vec4(0.541176f, 0.168627f, 0.886275f, 1.000000f);
	colors["Brown"] = glm::vec4(0.647059f, 0.164706f, 0.164706f, 1.000000f);
	colors["BurlyWood"] = glm::vec4(0.870588f, 0.721569f, 0.529412f, 1.000000f);
	colors["CadetBlue"] = glm::vec4(0.372549f, 0.619608f, 0.627451f, 1.000000f);
	colors["Chartreuse"] = glm::vec4(0.498039f, 1.000000f, 0.000000f, 1.000000f);
	colors["Chocolate"] = glm::vec4(0.823529f, 0.411765f, 0.117647f, 1.000000f);
	colors["Coral"] = glm::vec4(1.000000f, 0.498039f, 0.313726f, 1.000000f);
	colors["CornflowerBlue"] = glm::vec4(0.392157f, 0.584314f, 0.929412f, 1.000000f);
	colors["Cornsilk"] = glm::vec4(1.000000f, 0.972549f, 0.862745f, 1.000000f);
	colors["Crimson"] = glm::vec4(0.862745f, 0.078431f, 0.235294f, 1.000000f);
	colors["Cyan"] = glm::vec4(0.000000f, 1.000000f, 1.000000f, 1.000000f);
	colors["DarkBlue"] = glm::vec4(0.000000f, 0.000000f, 0.545098f, 1.000000f);
	colors["DarkCyan"] = glm::vec4(0.000000f, 0.545098f, 0.545098f, 1.000000f);
	colors["DarkGoldenrod"] = glm::vec4(0.721569f, 0.525490f, 0.043137f, 1.000000f);
	colors["DarkGray"] = glm::vec4(0.662745f, 0.662745f, 0.662745f, 1.000000f);
	colors["DarkGreen"] = glm::vec4(0.000000f, 0.392157f, 0.000000f, 1.000000f);
	colors["DarkKhaki"] = glm::vec4(0.741176f, 0.717647f, 0.419608f, 1.000000f);
	colors["DarkMagenta"] = glm::vec4(0.545098f, 0.000000f, 0.545098f, 1.000000f);
	colors["DarkOliveGreen"] = glm::vec4(0.333333f, 0.419608f, 0.184314f, 1.000000f);
	colors["DarkOrange"] = glm::vec4(1.000000f, 0.549020f, 0.000000f, 1.000000f);
	colors["DarkOrchid"] = glm::vec4(0.600000f, 0.196078f, 0.800000f, 1.000000f);
	colors["DarkRed"] = glm::vec4(0.545098f, 0.000000f, 0.000000f, 1.000000f);
	colors["DarkSalmon"] = glm::vec4(0.913725f, 0.588235f, 0.478431f, 1.000000f);
	colors["DarkSeaGreen"] = glm::vec4(0.560784f, 0.737255f, 0.560784f, 1.000000f);
	colors["DarkSlateBlue"] = glm::vec4(0.282353f, 0.239216f, 0.545098f, 1.000000f);
	colors["DarkSlateGray"] = glm::vec4(0.184314f, 0.309804f, 0.309804f, 1.000000f);
	colors["DarkTurquoise"] = glm::vec4(0.000000f, 0.807843f, 0.819608f, 1.000000f);
	colors["DarkViolet"] = glm::vec4(0.580392f, 0.000000f, 0.827451f, 1.000000f);
	colors["DeepPink"] = glm::vec4(1.000000f, 0.078431f, 0.576471f, 1.000000f);
	colors["DeepSkyBlue"] = glm::vec4(0.000000f, 0.749020f, 1.000000f, 1.000000f);
	colors["DimGray"] = glm::vec4(0.411765f, 0.411765f, 0.411765f, 1.000000f);
	colors["DodgerBlue"] = glm::vec4(0.117647f, 0.564706f, 1.000000f, 1.000000f);
	colors["Firebrick"] = glm::vec4(0.698039f, 0.133333f, 0.133333f, 1.000000f);
	colors["FloralWhite"] = glm::vec4(1.000000f, 0.980392f, 0.941176f, 1.000000f);
	colors["ForestGreen"] = glm::vec4(0.133333f, 0.545098f, 0.133333f, 1.000000f);
	colors["Fuchsia"] = glm::vec4(1.000000f, 0.000000f, 1.000000f, 1.000000f);
	colors["Gainsboro"] = glm::vec4(0.862745f, 0.862745f, 0.862745f, 1.000000f);
	colors["GhostWhite"] = glm::vec4(0.972549f, 0.972549f, 1.000000f, 1.000000f);
	colors["Gold"] = glm::vec4(1.000000f, 0.843137f, 0.000000f, 1.000000f);
	colors["Goldenrod"] = glm::vec4(0.854902f, 0.647059f, 0.125490f, 1.000000f);
	colors["Gray"] = glm::vec4(0.501961f, 0.501961f, 0.501961f, 1.000000f);
	colors["Green"] = glm::vec4(0.000000f, 0.501961f, 0.000000f, 1.000000f);
	colors["GreenYellow"] = glm::vec4(0.678431f, 1.000000f, 0.184314f, 1.000000f);
	colors["Honeydew"] = glm::vec4(0.941176f, 1.000000f, 0.941176f, 1.000000f);
	colors["HotPink"] = glm::vec4(1.000000f, 0.411765f, 0.705882f, 1.000000f);
	colors["IndianRed"] = glm::vec4(0.803922f, 0.360784f, 0.360784f, 1.000000f);
	colors["Indigo"] = glm::vec4(0.294118f, 0.000000f, 0.509804f, 1.000000f);
	colors["Ivory"] = glm::vec4(1.000000f, 1.000000f, 0.941176f, 1.000000f);
	colors["Khaki"] = glm::vec4(0.941176f, 0.901961f, 0.549020f, 1.000000f);
	colors["Lavender"] = glm::vec4(0.901961f, 0.901961f, 0.980392f, 1.000000f);
	colors["LavenderBlush"] = glm::vec4(1.000000f, 0.941176f, 0.960784f, 1.000000f);
	colors["LawnGreen"] = glm::vec4(0.486275f, 0.988235f, 0.000000f, 1.000000f);
	colors["LemonChiffon"] = glm::vec4(1.000000f, 0.980392f, 0.803922f, 1.000000f);
	colors["LightBlue"] = glm::vec4(0.678431f, 0.847059f, 0.901961f, 1.000000f);
	colors["LightCoral"] = glm::vec4(0.941176f, 0.501961f, 0.501961f, 1.000000f);
	colors["LightCyan"] = glm::vec4(0.878431f, 1.000000f, 1.000000f, 1.000000f);
	colors["LightGoldenrodYellow"] = glm::vec4(0.980392f, 0.980392f, 0.823529f, 1.000000f);
	colors["LightGray"] = glm::vec4(0.827451f, 0.827451f, 0.827451f, 1.000000f);
	colors["LightGreen"] = glm::vec4(0.564706f, 0.933333f, 0.564706f, 1.000000f);
	colors["LightPink"] = glm::vec4(1.000000f, 0.713726f, 0.756863f, 1.000000f);
	colors["LightSalmon"] = glm::vec4(1.000000f, 0.627451f, 0.478431f, 1.000000f);
	colors["LightSeaGreen"] = glm::vec4(0.125490f, 0.698039f, 0.666667f, 1.000000f);
	colors["LightSkyBlue"] = glm::vec4(0.529412f, 0.807843f, 0.980392f, 1.000000f);
	colors["LightSlateGray"] = glm::vec4(0.466667f, 0.533333f, 0.600000f, 1.000000f);
	colors["LightSteelBlue"] = glm::vec4(0.690196f, 0.768627f, 0.870588f, 1.000000f);
	colors["LightYellow"] = glm::vec4(1.000000f, 1.000000f, 0.878431f, 1.000000f);
	colors["Lime"] = glm::vec4(0.000000f, 1.000000f, 0.000000f, 1.000000f);
	colors["LimeGreen"] = glm::vec4(0.196078f, 0.803922f, 0.196078f, 1.000000f);
	colors["Linen"] = glm::vec4(0.980392f, 0.941176f, 0.901961f, 1.000000f);
	colors["Magenta"] = glm::vec4(1.000000f, 0.000000f, 1.000000f, 1.000000f);
	colors["Maroon"] = glm::vec4(0.501961f, 0.000000f, 0.000000f, 1.000000f);
	colors["MediumAquamarine"] = glm::vec4(0.400000f, 0.803922f, 0.666667f, 1.000000f);
	colors["MediumBlue"] = glm::vec4(0.000000f, 0.000000f, 0.803922f, 1.000000f);
	colors["MediumOrchid"] = glm::vec4(0.729412f, 0.333333f, 0.827451f, 1.000000f);
	colors["MediumPurple"] = glm::vec4(0.576471f, 0.439216f, 0.858824f, 1.000000f);
	colors["MediumSeaGreen"] = glm::vec4(0.235294f, 0.701961f, 0.443137f, 1.000000f);
	colors["MediumSlateBlue"] = glm::vec4(0.482353f, 0.407843f, 0.933333f, 1.000000f);
	colors["MediumSpringGreen"] = glm::vec4(0.000000f, 0.980392f, 0.603922f, 1.000000f);
	colors["MediumTurquoise"] = glm::vec4(0.282353f, 0.819608f, 0.800000f, 1.000000f);
	colors["MediumVioletRed"] = glm::vec4(0.780392f, 0.082353f, 0.521569f, 1.000000f);
	colors["MidnightBlue"] = glm::vec4(0.098039f, 0.098039f, 0.439216f, 1.000000f);
	colors["MintCream"] = glm::vec4(0.960784f, 1.000000f, 0.980392f, 1.000000f);
	colors["MistyRose"] = glm::vec4(1.000000f, 0.894118f, 0.882353f, 1.000000f);
	colors["Moccasin"] = glm::vec4(1.000000f, 0.894118f, 0.709804f, 1.000000f);
	colors["NavajoWhite"] = glm::vec4(1.000000f, 0.870588f, 0.678431f, 1.000000f);
	colors["Navy"] = glm::vec4(0.000000f, 0.000000f, 0.501961f, 1.000000f);
	colors["OldLace"] = glm::vec4(0.992157f, 0.960784f, 0.901961f, 1.000000f);
	colors["Olive"] = glm::vec4(0.501961f, 0.501961f, 0.000000f, 1.000000f);
	colors["OliveDrab"] = glm::vec4(0.419608f, 0.556863f, 0.137255f, 1.000000f);
	colors["Orange"] = glm::vec4(1.000000f, 0.647059f, 0.000000f, 1.000000f);
	colors["OrangeRed"] = glm::vec4(1.000000f, 0.270588f, 0.000000f, 1.000000f);
	colors["Orchid"] = glm::vec4(0.854902f, 0.439216f, 0.839216f, 1.000000f);
	colors["PaleGoldenrod"] = glm::vec4(0.933333f, 0.909804f, 0.666667f, 1.000000f);
	colors["PaleGreen"] = glm::vec4(0.596078f, 0.984314f, 0.596078f, 1.000000f);
	colors["PaleTurquoise"] = glm::vec4(0.686275f, 0.933333f, 0.933333f, 1.000000f);
	colors["PaleVioletRed"] = glm::vec4(0.858824f, 0.439216f, 0.576471f, 1.000000f);
	colors["PapayaWhip"] = glm::vec4(1.000000f, 0.937255f, 0.835294f, 1.000000f);
	colors["PeachPuff"] = glm::vec4(1.000000f, 0.854902f, 0.725490f, 1.000000f);
	colors["Peru"] = glm::vec4(0.803922f, 0.521569f, 0.247059f, 1.000000f);
	colors["Pink"] = glm::vec4(1.000000f, 0.752941f, 0.796078f, 1.000000f);
	colors["Plum"] = glm::vec4(0.866667f, 0.627451f, 0.866667f, 1.000000f);
	colors["PowderBlue"] = glm::vec4(0.690196f, 0.878431f, 0.901961f, 1.000000f);
	colors["Purple"] = glm::vec4(0.501961f, 0.000000f, 0.501961f, 1.000000f);
	colors["Red"] = glm::vec4(1.000000f, 0.000000f, 0.000000f, 1.000000f);
	colors["RosyBrown"] = glm::vec4(0.737255f, 0.560784f, 0.560784f, 1.000000f);
	colors["RoyalBlue"] = glm::vec4(0.254902f, 0.411765f, 0.882353f, 1.000000f);
	colors["SaddleBrown"] = glm::vec4(0.545098f, 0.270588f, 0.074510f, 1.000000f);
	colors["Salmon"] = glm::vec4(0.980392f, 0.501961f, 0.447059f, 1.000000f);
	colors["SandyBrown"] = glm::vec4(0.956863f, 0.643137f, 0.376471f, 1.000000f);
	colors["SeaGreen"] = glm::vec4(0.180392f, 0.545098f, 0.341176f, 1.000000f);
	colors["SeaShell"] = glm::vec4(1.000000f, 0.960784f, 0.933333f, 1.000000f);
	colors["Sienna"] = glm::vec4(0.627451f, 0.321569f, 0.176471f, 1.000000f);
	colors["Silver"] = glm::vec4(0.752941f, 0.752941f, 0.752941f, 1.000000f);
	colors["SkyBlue"] = glm::vec4(0.529412f, 0.807843f, 0.921569f, 1.000000f);
	colors["SlateBlue"] = glm::vec4(0.415686f, 0.352941f, 0.803922f, 1.000000f);
	colors["SlateGray"] = glm::vec4(0.439216f, 0.501961f, 0.564706f, 1.000000f);
	colors["Snow"] = glm::vec4(1.000000f, 0.980392f, 0.980392f, 1.000000f);
	colors["SpringGreen"] = glm::vec4(0.000000f, 1.000000f, 0.498039f, 1.000000f);
	colors["SteelBlue"] = glm::vec4(0.274510f, 0.509804f, 0.705882f, 1.000000f);
	colors["Tan"] = glm::vec4(0.823529f, 0.705882f, 0.549020f, 1.000000f);
	colors["Teal"] = glm::vec4(0.000000f, 0.501961f, 0.501961f, 1.000000f);
	colors["Thistle"] = glm::vec4(0.847059f, 0.749020f, 0.847059f, 1.000000f);
	colors["Tomato"] = glm::vec4(1.000000f, 0.388235f, 0.278431f, 1.000000f);
	colors["Transparent"] = glm::vec4(1.000000f, 1.000000f, 1.000000f, 0.000000f);
	colors["Turquoise"] = glm::vec4(0.250980f, 0.878431f, 0.815686f, 1.000000f);
	colors["Violet"] = glm::vec4(0.933333f, 0.509804f, 0.933333f, 1.000000f);
	colors["Wheat"] = glm::vec4(0.960784f, 0.870588f, 0.701961f, 1.000000f);
	colors["White"] = glm::vec4(1.000000f, 1.000000f, 1.000000f, 1.000000f);
	colors["WhiteSmoke"] = glm::vec4(0.960784f, 0.960784f, 0.960784f, 1.000000f);
	colors["Yellow"] = glm::vec4(1.000000f, 1.000000f, 0.000000f, 1.000000f);
	colors["YellowGreen"] = glm::vec4(0.603922f, 0.803922f, 0.196078f, 1.000000f);

	colorNames.push_back("AliceBlue");
	colorNames.push_back("AntiqueWhite");
	colorNames.push_back("Aqua");
	colorNames.push_back("Aquamarine");
	colorNames.push_back("Azure");
	colorNames.push_back("Beige");
	colorNames.push_back("Bisque");
	colorNames.push_back("Black");
	colorNames.push_back("BlanchedAlmond");
	colorNames.push_back("Blue");
	colorNames.push_back("BlueViolet");
	colorNames.push_back("Brown");
	colorNames.push_back("BurlyWood");
	colorNames.push_back("CadetBlue");
	colorNames.push_back("Chartreuse");
	colorNames.push_back("Chocolate");
	colorNames.push_back("Coral");
	colorNames.push_back("CornflowerBlue");
	colorNames.push_back("Cornsilk");
	colorNames.push_back("Crimson");
	colorNames.push_back("Cyan");
	colorNames.push_back("DarkBlue");
	colorNames.push_back("DarkCyan");
	colorNames.push_back("DarkGoldenrod");
	colorNames.push_back("DarkGray");
	colorNames.push_back("DarkGreen");
	colorNames.push_back("DarkKhaki");
	colorNames.push_back("DarkMagenta");
	colorNames.push_back("DarkOliveGreen");
	colorNames.push_back("DarkOrange");
	colorNames.push_back("DarkOrchid");
	colorNames.push_back("DarkRed");
	colorNames.push_back("DarkSalmon");
	colorNames.push_back("DarkSeaGreen");
	colorNames.push_back("DarkSlateBlue");
	colorNames.push_back("DarkSlateGray");
	colorNames.push_back("DarkTurquoise");
	colorNames.push_back("DarkViolet");
	colorNames.push_back("DeepPink");
	colorNames.push_back("DeepSkyBlue");
	colorNames.push_back("DimGray");
	colorNames.push_back("DodgerBlue");
	colorNames.push_back("Firebrick");
	colorNames.push_back("FloralWhite");
	colorNames.push_back("ForestGreen");
	colorNames.push_back("Fuchsia");
	colorNames.push_back("Gainsboro");
	colorNames.push_back("GhostWhite");
	colorNames.push_back("Gold");
	colorNames.push_back("Goldenrod");
	colorNames.push_back("Gray");
	colorNames.push_back("Green");
	colorNames.push_back("GreenYellow");
	colorNames.push_back("Honeydew");
	colorNames.push_back("HotPink");
	colorNames.push_back("IndianRed");
	colorNames.push_back("Indigo");
	colorNames.push_back("Ivory");
	colorNames.push_back("Khaki");
	colorNames.push_back("Lavender");
	colorNames.push_back("LavenderBlush");
	colorNames.push_back("LawnGreen");
	colorNames.push_back("LemonChiffon");
	colorNames.push_back("LightBlue");
	colorNames.push_back("LightCoral");
	colorNames.push_back("LightCyan");
	colorNames.push_back("LightGoldenrodYellow");
	colorNames.push_back("LightGray");
	colorNames.push_back("LightGreen");
	colorNames.push_back("LightPink");
	colorNames.push_back("LightSalmon");
	colorNames.push_back("LightSeaGreen");
	colorNames.push_back("LightSkyBlue");
	colorNames.push_back("LightSlateGray");
	colorNames.push_back("LightSteelBlue");
	colorNames.push_back("LightYellow");
	colorNames.push_back("Lime");
	colorNames.push_back("LimeGreen");
	colorNames.push_back("Linen");
	colorNames.push_back("Magenta");
	colorNames.push_back("Maroon");
	colorNames.push_back("MediumAquamarine");
	colorNames.push_back("MediumBlue");
	colorNames.push_back("MediumOrchid");
	colorNames.push_back("MediumPurple");
	colorNames.push_back("MediumSeaGreen");
	colorNames.push_back("MediumSlateBlue");
	colorNames.push_back("MediumSpringGreen");
	colorNames.push_back("MediumTurquoise");
	colorNames.push_back("MediumVioletRed");
	colorNames.push_back("MidnightBlue");
	colorNames.push_back("MintCream");
	colorNames.push_back("MistyRose");
	colorNames.push_back("Moccasin");
	colorNames.push_back("NavajoWhite");
	colorNames.push_back("Navy");
	colorNames.push_back("OldLace");
	colorNames.push_back("Olive");
	colorNames.push_back("OliveDrab");
	colorNames.push_back("Orange");
	colorNames.push_back("OrangeRed");
	colorNames.push_back("Orchid");
	colorNames.push_back("PaleGoldenrod");
	colorNames.push_back("PaleGreen");
	colorNames.push_back("PaleTurquoise");
	colorNames.push_back("PaleVioletRed");
	colorNames.push_back("PapayaWhip");
	colorNames.push_back("PeachPuff");
	colorNames.push_back("Peru");
	colorNames.push_back("Pink");
	colorNames.push_back("Plum");
	colorNames.push_back("PowderBlue");
	colorNames.push_back("Purple");
	colorNames.push_back("Red");
	colorNames.push_back("RosyBrown");
	colorNames.push_back("RoyalBlue");
	colorNames.push_back("SaddleBrown");
	colorNames.push_back("Salmon");
	colorNames.push_back("SandyBrown");
	colorNames.push_back("SeaGreen");
	colorNames.push_back("SeaShell");
	colorNames.push_back("Sienna");
	colorNames.push_back("Silver");
	colorNames.push_back("SkyBlue");
	colorNames.push_back("SlateBlue");
	colorNames.push_back("SlateGray");
	colorNames.push_back("Snow");
	colorNames.push_back("SpringGreen");
	colorNames.push_back("SteelBlue");
	colorNames.push_back("Tan");
	colorNames.push_back("Teal");
	colorNames.push_back("Thistle");
	colorNames.push_back("Tomato");
	colorNames.push_back("Transparent");
	colorNames.push_back("Turquoise");
	colorNames.push_back("Violet");
	colorNames.push_back("Wheat");
	colorNames.push_back("White");
	colorNames.push_back("WhiteSmoke");
	colorNames.push_back("Yellow");
	colorNames.push_back("YellowGreen");

}

Config::Config()
{
	output_path = environment::combine_path(environment::combine_path(environment::user_documents(), "OpenPostPro"), "output");
}
