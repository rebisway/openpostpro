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

// The table of characters of a text: the glyphs of a font, 256 by page (16 x 16, as the tables of the systems), and a click gives the code
// of a glyph to insert in the text. It is for the fonts which have their characters at codes which nobody knows (Symbol, Webdings,
// Wingdings) and for the languages which have thousands of them (Chinese, Japanese). The glyphs are drawn by FreeType in an image of the
// page, so it does not depend on the fonts which the interface has. All in this file: it is only included by text.cpp.

#pragma once
#ifndef _CHARMAP_H
#define _CHARMAP_H

#include <imgui.h>
#include <environment.h>
#include <font.h>
#include <lang.h>
#include <renderer.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

class CharMap
{
private:
	static const int CELL = 48;				// pixels of a glyph in the image of the page

	bool _open = false;
	std::string _path;						// the font file which is shown
	int _index = 0;
	std::vector<unsigned int> _codes;		// what the file can draw
	std::vector<int> _pages;				// the pages (code >> 8) which have a glyph
	int _page = 0;							// the index in _pages
	Texture* _texture = nullptr;
	Renderer* _render = nullptr;
	std::vector<unsigned int> _cells = std::vector<unsigned int>(256, 0xFFFFFFFFu);
	char _goto[12] = "";

	void make_page()
	{
		std::fill(_cells.begin(), _cells.end(), 0xFFFFFFFFu);
		if (_pages.empty())
			return;
		const int page = _pages[_page];
		for (unsigned int code : _codes)
			if ((int)(code >> 8) == page)
				_cells[code & 0xFF] = code;

		if (_texture != nullptr)
		{
			_render->delete_texture(_texture);
			_texture = nullptr;
		}
		std::vector<unsigned char> rgba;
		int width = 0, height = 0;
		if (Font::render_cells(_path, _index, _cells, CELL, rgba, width, height))
			_texture = _render->create_rgba_texture(width, height, rgba.data());
	}

	void load(const std::string& path, int index)
	{
		_path = path;
		_index = index;
		_codes = Font::list_codes(path, index);
		_pages.clear();
		for (unsigned int code : _codes)
			if (_pages.empty() || _pages.back() != (int)(code >> 8))
				_pages.push_back((int)(code >> 8));
		// the first page which has the letters, else the first one
		_page = 0;
		for (int i = 0; i < (int)_pages.size(); i++)
			if (_pages[i] == 0 || _pages[i] == 0xF0)
			{
				_page = i;
				break;
			}
		make_page();
	}

	void go_to_page(int page)
	{
		if (_pages.empty())
			return;
		// the page of the list which is the nearest
		int best = 0;
		for (int i = 0; i < (int)_pages.size(); i++)
			if (std::abs(_pages[i] - page) < std::abs(_pages[best] - page))
				best = i;
		if (best != _page)
		{
			_page = best;
			make_page();
		}
	}

public:
	static CharMap& instance()
	{
		static CharMap map;
		return map;
	}

	void open() { _open = true; }
	bool is_open() const { return _open; }

	// Draws the window for a family. True and the code of the glyph when one was chosen.
	bool draw(Renderer* render, const environment::FontFamily& family, unsigned int& picked)
	{
		if (!_open || render == nullptr)
			return false;
		_render = render;

		// the file of the family: the regular one, else the first which exists
		struct Candidate { const std::string* path; int index; };
		const Candidate candidates[] = { { &family.regular_path, family.regular_index }, { &family.bold_path, family.bold_index },
			{ &family.italic_path, family.italic_index }, { &family.bold_italic_path, family.bold_italic_index } };
		const std::string* path = nullptr;
		int index = 0;
		for (const Candidate& c : candidates)
			if (!c.path->empty())
			{
				path = c.path;
				index = c.index;
				break;
			}
		if (path != nullptr && (*path != _path || index != _index))
			load(*path, index);

		bool chosen = false;
		ImGui::SetNextWindowSize(ImVec2(16 * 44 + 40, 16 * 44 + 120), ImGuiCond_FirstUseEver);
		const std::string title = std::string(Lang::l("FONT_CHARMAP_TITLE")) + " - " + family.name + "###CHARMAP";
		if (ImGui::Begin(title.c_str(), &_open))
		{
			if (path == nullptr || _pages.empty())
			{
				ImGui::TextUnformatted("-");
			}
			else
			{
				// the pages: previous, next, the list, and a code to go to
				if (ImGui::Button("<") && _page > 0)
				{
					_page--;
					make_page();
				}
				ImGui::SameLine();
				if (ImGui::Button(">") && _page + 1 < (int)_pages.size())
				{
					_page++;
					make_page();
				}
				ImGui::SameLine();
				char label[32];
				snprintf(label, sizeof(label), "U+%04X - U+%04X", _pages[_page] << 8, (_pages[_page] << 8) + 255);
				ImGui::SetNextItemWidth(170);
				if (ImGui::BeginCombo("##CHARMAP_PAGE", label))
				{
					for (int i = 0; i < (int)_pages.size(); i++)
					{
						snprintf(label, sizeof(label), "U+%04X - U+%04X", _pages[i] << 8, (_pages[i] << 8) + 255);
						if (ImGui::Selectable(label, i == _page) && i != _page)
						{
							_page = i;
							make_page();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(90);
				if (ImGui::InputText(Lang::l("FONT_CHARMAP_GOTO"), _goto, sizeof(_goto), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
				{
					go_to_page((int)(std::strtoul(_goto, nullptr, 16) >> 8));
					_goto[0] = 0;
				}

				// the glyphs of the page
				const float padding = ImGui::GetStyle().FramePadding.x;
				const ImVec2 size(32.0f, 32.0f);
				const ImVec2 button(size.x + 2 * padding, size.y + 2 * padding);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
				for (int i = 0; i < 256; i++)
				{
					ImGui::PushID(i);
					if (i % 16 != 0)
						ImGui::SameLine();
					if (_cells[i] == 0xFFFFFFFFu || _texture == nullptr)
					{
						ImGui::InvisibleButton("##empty", button);
					}
					else
					{
						const ImVec2 uv0((i % 16) / 16.0f, (i / 16) / 16.0f), uv1((i % 16 + 1) / 16.0f, (i / 16 + 1) / 16.0f);
						if (ImGui::ImageButton("##glyph", _texture->id(), size, uv0, uv1))
						{
							picked = _cells[i];
							chosen = true;
						}
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("U+%04X  (%u)", _cells[i], _cells[i]);
					}
					ImGui::PopID();
				}
				ImGui::PopStyleVar();
			}
		}
		ImGui::End();
		return chosen;
	}
};

#endif
