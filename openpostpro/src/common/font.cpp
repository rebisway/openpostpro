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

Font::Font(std::string family, std::string path, bool bold, bool italic)
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
	_error = FT_New_Face(*_library, _path.c_str(), 0, _face);
	if (_error)
	{
		throw error_text();
	}

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
	auto wc = stringex::to_wide_char(text);

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

	auto previous = get_glyph(text[0]);
	_height = 0; // glm::max(_height, (float)(*_face)->bbox.yMax); /*(float)previous->vertiAdvance()*/
	auto scale = height / (float)((*_face)->bbox.yMax - (*_face)->bbox.yMin);
	get_glyph_coordinates(text[0], scale);

	result.push_back(geometry::rectangle(0, height, 0, 0));


	float advance = (float)previous->horiAdvance() + inter_char;
	float line = 0;

	for (int i = 1; i < text.size(); i++)
	{
		result.back().bottom_right = glm::vec2(advance, height);
		result.push_back(geometry::rectangle(advance, height, 0, 0));

		char c = text[i];
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

		if (text[i] == '\n')
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




