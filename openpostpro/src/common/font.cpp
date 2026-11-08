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

#include "font.h"
#include <strings.h>

#include FT_OUTLINE_H
#include FT_BBOX_H

FT_Library* Font::_library = nullptr;

// The encoding of a face for the codes of the characters: Unicode, else the encoding of the symbol fonts (Symbol, Webdings, Wingdings:
// their characters are at U+F020..U+F0FF), else the first one. FreeType does not select the symbol encoding by itself.
static void select_charmap(FT_Face face)
{
	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) == 0)
		return;
	if (FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL) == 0)
		return;
	if (face->num_charmaps > 0)
		FT_Set_Charmap(face, face->charmaps[0]);
}

Font::Font(std::string family, std::string path, bool bold, bool italic, int index)
{
	_family = family;
	_path = path;
	_bold = bold;
	_italic = italic;

	if (_library == nullptr)
	{
		_library = new FT_Library();
		if (FT_Init_FreeType(_library) != 0)
			throw "Freetype initialization error";
	}

	_face = new FT_Face();
	_error = FT_New_Face(*_library, _path.c_str(), index, _face);
	if (_error)
	{
		throw error_text();
	}
	select_charmap(*_face);

	_height = 0;
}

Font::~Font()
{
	FT_Done_Face(*_face);
	delete _face;

	//FT_Done_FreeType(*_library),
	//delete _library;

	std::map<unsigned int, Glyph*>::iterator it;
	for (it = _glyphs.begin(); it != _glyphs.end(); it++)
		delete it->second;
	_glyphs.clear();
}

std::vector<unsigned int> Font::list_codes(const std::string& path, int index)
{
	std::vector<unsigned int> result;
	if (_library == nullptr)
	{
		_library = new FT_Library();
		if (FT_Init_FreeType(_library) != 0)
			return result;
	}
	FT_Face face = nullptr;
	if (FT_New_Face(*_library, path.c_str(), index, &face) != 0 || face == nullptr)
		return result;
	select_charmap(face);
	FT_UInt glyph = 0;
	FT_ULong code = FT_Get_First_Char(face, &glyph);
	while (glyph != 0)
	{
		result.push_back((unsigned int)code);
		code = FT_Get_Next_Char(face, code, &glyph);
	}
	FT_Done_Face(face);
	return result;
}

bool Font::render_cells(const std::string& path, int index, const std::vector<unsigned int>& cells, int cell, std::vector<unsigned char>& rgba, int& width, int& height)
{
	width = height = 16 * cell;
	rgba.assign((size_t)width * height * 4, 0);
	if (_library == nullptr)
	{
		_library = new FT_Library();
		if (FT_Init_FreeType(_library) != 0)
			return false;
	}
	FT_Face face = nullptr;
	if (FT_New_Face(*_library, path.c_str(), index, &face) != 0 || face == nullptr)
		return false;
	select_charmap(face);
	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)(cell * 0.6));

	for (int i = 0; i < 256 && i < (int)cells.size(); i++)
	{
		if (cells[i] == 0xFFFFFFFFu)
			continue;
		const FT_UInt glyph = FT_Get_Char_Index(face, cells[i]);
		if (glyph == 0 || FT_Load_Glyph(face, glyph, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP) != 0)
			continue;
		const FT_Bitmap& bitmap = face->glyph->bitmap;
		const int cell_x = (i % 16) * cell, cell_y = (i / 16) * cell;
		// the glyph is centered in its cell, on a baseline at three quarters of the height
		const int origin_x = cell_x + (cell - (int)(face->glyph->advance.x >> 6)) / 2 + face->glyph->bitmap_left;
		const int origin_y = cell_y + (int)(cell * 0.75) - face->glyph->bitmap_top;
		for (unsigned int row = 0; row < bitmap.rows; row++)
		{
			for (unsigned int column = 0; column < bitmap.width; column++)
			{
				const int x = origin_x + (int)column, y = origin_y + (int)row;
				if (x < cell_x || x >= cell_x + cell || y < cell_y || y >= cell_y + cell)
					continue;
				unsigned char gray = 0;
				if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
					gray = bitmap.buffer[row * bitmap.pitch + column];
				else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
					gray = (bitmap.buffer[row * bitmap.pitch + column / 8] & (0x80 >> (column % 8))) ? 255 : 0;
				unsigned char* pixel = &rgba[((size_t)y * width + x) * 4];
				pixel[0] = pixel[1] = pixel[2] = 255;
				pixel[3] = gray;
			}
		}
	}
	FT_Done_Face(face);
	return true;
}

Glyph* Font::get_glyph(unsigned int character)
{
	auto index = _glyphs.find(character);
	if (index == _glyphs.end()) {
		Glyph* g = new Glyph();
		_glyphs[character] = g;
		return g;
	}
	else {
		return index->second;
	}
}

std::vector<std::vector<glm::vec2>> Font::get_glyph_coordinates(unsigned int character, float height)
{
	Glyph* g = get_glyph(character);
	g->extract(*_face, character, height);

	return g->contours();
}

std::vector<std::vector<glm::vec2>> Font::get_text_coordinates(std::string text, std::vector<int>& indices, float height, float inter_char)
{
	std::u32string wc = stringex::to_utf32(text);
	if (wc.empty())
		wc.push_back(0);

	indices.clear();
	auto previous = get_glyph(wc[0]);
	_height = 0; // glm::max(_height, (float)(*_face)->bbox.yMax); /*(float)previous->vertiAdvance()*/
	auto scale = height / (float)((*_face)->bbox.yMax-(*_face)->bbox.yMin);
	std::vector<std::vector<glm::vec2>> vertices;
	std::vector<std::vector<glm::vec2>> first = get_glyph_coordinates(wc[0], scale);
	vertices.insert(vertices.end(), first.begin(), first.end());


	float advance = (float)previous->horiAdvance() + inter_char;
	float line = 0;

	indices.push_back((int)vertices.size() - 1);

	for (int i = 1; i < wc.size(); i++)
	{
		int c = wc[i];
		if (c > 31)
		{
			auto next = get_glyph(c);
			std::vector<std::vector<glm::vec2>> g = get_glyph_coordinates(c, scale);

			FT_Vector akerning;
			FT_Get_Kerning(*_face, previous->index(), next->index(), 0, &akerning);

			advance += akerning.x * scale;

			glm::mat4x4 mat = glm::translate(glm::vec3(advance, line, 0.0f)) /** glm::scale(glm::vec3(scale, scale, 1.0f))*/;
			for (int j = 0; j < g.size(); j++)
				for (int i = 0; i < g[j].size(); i++)
					g[j][i] = mat * glm::vec4(g[j][i].x, g[j][i].y, 1, 1);

			vertices.insert(vertices.end(), g.begin(), g.end());

			advance += (float)next->horiAdvance() + inter_char;
			previous = next;
		
			_height = glm::max(_height, (float)next->vertiAdvance());
		}

		if (indices.size() == 0 || indices.back() != vertices.size()-1)
			indices.push_back((int)vertices.size()-1);
		else
			indices.push_back(-1);

		if (wc[i] == '\n')
		{
			advance = 0;
			line -= (*_face)->height;
		}
	}

	//vertices = glmex::reduce(vertices, 3);

	return vertices;
}

std::vector<geometry::rectangle> Font::get_text_bounds(std::string text, float height, float inter_char)
{
	std::vector<geometry::rectangle> result;

	// one rectangle for each character (a code point), not for each byte of its UTF-8 form
	std::u32string wc = stringex::to_utf32(text);
	if (wc.empty())
		wc.push_back(0);

	auto previous = get_glyph(wc[0]);
	_height = 0; // glm::max(_height, (float)(*_face)->bbox.yMax); /*(float)previous->vertiAdvance()*/
	auto scale = height / (float)((*_face)->bbox.yMax - (*_face)->bbox.yMin);
	get_glyph_coordinates(wc[0], scale);

	result.push_back(geometry::rectangle(0, height, 0, 0));


	float advance = (float)previous->horiAdvance() + inter_char;
	float line = 0;

	for (size_t i = 1; i < wc.size(); i++)
	{
		result.back().bottom_right = glm::vec2(advance, height);
		result.push_back(geometry::rectangle(advance, height, 0, 0));

		char32_t c = wc[i];
		if (c > 31)
		{

			auto next = get_glyph(c);
			get_glyph_coordinates(c, scale);

			FT_Vector akerning;
			FT_Get_Kerning(*_face, previous->index(), next->index(), 0, &akerning);

			advance += akerning.x * scale;
			advance += (float)next->horiAdvance() + inter_char;
			previous = next;
		}

		if (wc[i] == '\n')
		{
			advance = 0;
			line -= (*_face)->height;
		}
	}
	result.back().bottom_right = glm::vec2(advance, height);

	return result;
}

glm::vec2 Font::get_face_size(float height)
{
	auto scale = height / (float)((*_face)->bbox.yMax - (*_face)->bbox.yMin);
	auto width = scale * (float)((*_face)->bbox.xMax - (*_face)->bbox.xMin);

	return glm::vec2(width, height);
}


void Glyph::extract(FT_Face& face, unsigned int character, float scale)
{
	// already extracted
	if (_contours.size() > 0)
		return;

	_index = FT_Get_Char_Index(face, character);
	// a symbol font (Symbol, Webdings, Wingdings) has its characters at U+F000 + the code: a letter which is typed finds its sign there,
	// as it does in the programs of the system
	if (_index == 0 && character < 0x100 && face->charmap != nullptr && face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
		_index = FT_Get_Char_Index(face, 0xF000 + character);
	auto error = FT_Load_Glyph(face, _index, FT_LOAD_DEFAULT);

	_scale = scale;

	_width = (int)(face->glyph->metrics.width * scale);
	_height = (int)(face->glyph->metrics.height * scale);
	_horiAdvance = (int)(face->glyph->metrics.horiAdvance * scale);
	_horiBearingX = (int)(face->glyph->metrics.horiBearingX * scale);
	_horiBearingY = (int)(face->glyph->metrics.horiBearingY * scale);
	_vertiAdvance = (int)(face->glyph->metrics.vertAdvance * scale);
	_vertiBearingX = (int)(face->glyph->metrics.vertBearingX * scale);
	_vertiBearingY = (int)(face->glyph->metrics.vertBearingY * scale);

	FT_Outline_Funcs callbacks;

	callbacks.move_to = move_to;
	callbacks.line_to = line_to;
	callbacks.conic_to = conic_to;
	callbacks.cubic_to = cubic_to;

	callbacks.shift = 0;
	callbacks.delta = 0;


	FT_GlyphSlot slot = face->glyph;
	FT_Outline& outline = slot->outline;

	error = FT_Outline_Decompose(& outline, & callbacks, this);

	close();
}

void Glyph::append(glm::vec2 point)
{
	_construction.push_back(point);
};

void Glyph::append(std::vector<glm::vec2> vertices)
{
	_construction.insert(_construction.end(), vertices.begin()+1, vertices.end());
};

void Glyph::move(glm::vec2 point) 
{ 
	close(); 
	_construction.push_back(point); 
};

void Glyph::close()
{
	if (_construction.size() > 2 && _construction.front() != _construction.back())
		_construction.push_back(glm::vec2(_construction.front()));
	if (_construction.size() > 0)
		_contours.push_back(std::vector<glm::vec2>(_construction));
	_construction.clear();
}

int Glyph::move_to(const FT_Vector* to, void* user)
{
	((Glyph*)user)->move(glm::vec2(to->x, to->y) * ((Glyph*)user)->scale());
	return 0;
}

int Glyph::line_to(const FT_Vector* to, void* user)
{
	((Glyph*)user)->append(glm::vec2(to->x * ((Glyph*)user)->scale(), to->y * ((Glyph*)user)->scale()));
	return 0;
}

int Glyph::conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
	Glyph* g = ((Glyph*)user);
	g->append(geometry::quadratic(g->current(), glm::vec2(control->x, control->y) * ((Glyph*)user)->scale(), glm::vec2(to->x, to->y) * ((Glyph*)user)->scale()));
	return 0;
}

int Glyph::cubic_to(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
	Glyph* g = ((Glyph*)user);
	g->append(geometry::cubic(g->current(), glm::vec2(control1->x, control1->y) * ((Glyph*)user)->scale(), glm::vec2(control2->x, control2->y) * ((Glyph*)user)->scale(), glm::vec2(to->x, to->y) * ((Glyph*)user)->scale()));
	return 0;
}





// ------------------------------------------------------------------------------------------------------ the fonts of the system
#include <environment.h>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <mutex>

namespace environment
{
	namespace
	{
		struct FontList
		{
			std::map<std::string, FontFamily> families;
			std::map<std::string, std::string> aliases;		// the name of the first versions (made of the file name) and the family which has it
		};

		// The name which the first versions gave to the family of a file: made of the name of the file. Kept only to find again the font of
		// a document which was saved with it.
		std::string legacy_name(std::string name)
		{
			name = stringex::to_lower(name);
			auto remove = [&](const std::string& part) {
				size_t i = name.find(part);
				if (i != std::string::npos)
					name.erase(i, part.size());
			};
			auto ends = [&](const std::string& part) { return name.size() >= part.size() && name.compare(name.size() - part.size(), part.size(), part) == 0; };
			remove(".ttf");
			if (name.find("italic") != std::string::npos) remove("italic");
			if (name.find("bold") != std::string::npos) remove("bold");
			if (name.find("bd") != std::string::npos) remove("bd");
			if (name.find("regular") != std::string::npos) remove("regular");
			if (ends("i")) name.pop_back();
			if (ends("b")) name.pop_back();
			if (ends("r")) name.pop_back();
			while (ends("_")) name.pop_back();
			while (ends("-")) name.pop_back();
			return name;
		}

		// the value of an environment variable, empty if there is none (the compiler of Windows refuses getenv)
		std::string variable(const char* name)
		{
#ifdef _WIN32
			char* value = nullptr;
			size_t size = 0;
			std::string result;
			if (_dupenv_s(&value, &size, name) == 0 && value != nullptr)
				result = value;
			free(value);
			return result;
#else
			const char* value = std::getenv(name);
			return value != nullptr ? value : "";
#endif
		}

		std::vector<std::string> font_folders()
		{
			std::vector<std::string> folders;
#ifdef _WIN32
			const std::string windir = variable("WINDIR");
			folders.push_back((windir.empty() ? std::string("c:\\windows") : windir) + "\\Fonts");
			const std::string local = variable("LOCALAPPDATA");		// the fonts which a person installed for herself
			if (!local.empty())
				folders.push_back(local + "\\Microsoft\\Windows\\Fonts");
#elif defined(__APPLE__) || defined(__MACH__)
			folders = { "/Library/Fonts", "/System/Library/Fonts", "/System/Library/Fonts/Supplemental" };
			const std::string home = variable("HOME");
			if (!home.empty())
				folders.push_back(home + "/Library/Fonts");
#else
			folders = { "/usr/share/fonts", "/usr/local/share/fonts" };
			const std::string home = variable("HOME");
			if (!home.empty())
			{
				folders.push_back(home + "/.fonts");
				folders.push_back(home + "/.local/share/fonts");
			}
#endif
			return folders;
		}

		void scan(FT_Library library, const std::string& folder, FontList& list)
		{
			std::error_code error;
			if (!std::filesystem::exists(folder, error))
				return;
			// the fonts of the system are in one folder on Windows; elsewhere they are in sub-folders
			auto visit = [&](const std::filesystem::directory_entry& entry) {
				if (!entry.is_regular_file(error))
					return;
				std::string extension = stringex::to_lower(entry.path().extension().string());
				if (extension != ".ttf" && extension != ".otf" && extension != ".ttc" && extension != ".otc")
					return;
				const std::string path = entry.path().string();

				FT_Face face = nullptr;
				if (FT_New_Face(library, path.c_str(), -1, &face) != 0 || face == nullptr)
					return;
				const long count = face->num_faces;
				FT_Done_Face(face);

				for (long index = 0; index < count; index++)
				{
					face = nullptr;
					if (FT_New_Face(library, path.c_str(), index, &face) != 0 || face == nullptr)
						continue;
					const bool usable = FT_IS_SCALABLE(face) && face->family_name != nullptr && face->family_name[0] != 0;
					std::string name = usable ? face->family_name : "";
					const std::string style = face->style_name != nullptr ? face->style_name : "";
					const bool bold = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
					const bool italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
					FT_Done_Face(face);
					if (!usable)
						continue;

					// A weight between the regular and the bold (Light, Medium, Black, ...) has the name of the family in FreeType: it is a family
					// of its own (Calibri Light), whatever the order of the files, else it would take the place of the regular one.
					std::string core = style;
					auto drop = [&](const char* word) {
						size_t i = stringex::to_lower(core).find(stringex::to_lower(word));
						if (i != std::string::npos)
							core.erase(i, std::string(word).size());
					};
					if (bold) drop("Bold");
					if (italic) { drop("Italic"); drop("Oblique"); drop("Slanted"); }
					drop("Regular");
					drop("Normal");
					drop("Roman");
					drop("Book");
					while (!core.empty() && core.front() == ' ') core.erase(0, 1);
					while (!core.empty() && core.back() == ' ') core.pop_back();
					if (!core.empty() && stringex::to_lower(name).find(stringex::to_lower(core)) == std::string::npos)
						name += " " + core;

					FontFamily& family = list.families[name];
					family.name = name;
					std::string* file = &family.regular_path;
					int* face_index = &family.regular_index;
					if (bold && italic) { file = &family.bold_italic_path; face_index = &family.bold_italic_index; }
					else if (bold) { file = &family.bold_path; face_index = &family.bold_index; }
					else if (italic) { file = &family.italic_path; face_index = &family.italic_index; }
					if (file->empty())
					{
						*file = path;
						*face_index = (int)index;
					}
					list.aliases.emplace(legacy_name(entry.path().filename().string()), name);
				}
			};
#ifdef _WIN32
			for (const auto& entry : std::filesystem::directory_iterator(folder, error))
				visit(entry);
#else
			for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink, error))
				visit(entry);
#endif
		}

		const FontList& font_list()
		{
			static FontList list;
			static std::once_flag once;
			std::call_once(once, [] {
				FT_Library library = nullptr;
				if (FT_Init_FreeType(&library) != 0)
					return;
				for (const std::string& folder : font_folders())
					scan(library, folder, list);
				FT_Done_FreeType(library);
			});
			return list;
		}
	}

	std::map<std::string, FontFamily> fonts()
	{
		return font_list().families;
	}

	FontFamily font_family(const std::string& name)
	{
		const FontList& list = font_list();
		auto exact = list.families.find(name);
		if (exact != list.families.end())
			return exact->second;

		const std::string lower = stringex::to_lower(name);
		for (const auto& entry : list.families)
			if (stringex::to_lower(entry.first) == lower)
				return entry.second;

		auto alias = list.aliases.find(lower);
		if (alias != list.aliases.end())
		{
			auto found = list.families.find(alias->second);
			if (found != list.families.end())
				return found->second;
		}
		return FontFamily();
	}
}
