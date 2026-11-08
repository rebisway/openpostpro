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
#ifndef _FONT_H
#define _FONT_H

#include <string>
#include <vector>
#include <map>
#include <geometry.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class Glyph
{
private:
	std::vector<std::vector<glm::vec2>> _contours;
	std::vector<glm::vec2> _construction;
	glm::vec2 _first = geometry::vec2_empty;
	int _width = 0;
	int _height = 0;
	int _horiAdvance = 0;
	int _horiBearingX = 0;
	int _horiBearingY = 0;
	int _vertiAdvance = 0;
	int _vertiBearingX = 0;
	int _vertiBearingY = 0;
	int _index = 0;

	FT_Face* _face = nullptr;
	unsigned int _character = 0;
	float _scale = 0.0f;

	void close();

public:

	int width() { return _width; }
	int height() { return _height; }
	int horiAdvance() { return _horiAdvance; }
	int horiBearingX() { return _horiBearingX; }
	int horiBearingY() { return _horiBearingY; }
	int vertiAdvance() { return _vertiAdvance; }
	int vertiBearingX() { return _vertiBearingX; }
	int vertiBearingY() { return _vertiBearingY; }
	int index() { return _index; }
	float scale() { return _scale; }
	std::vector<std::vector<glm::vec2>> contours() { return _contours; };
	glm::vec2 current() { return _construction.back(); }
	void append(glm::vec2 point);
	void append(std::vector<glm::vec2> vertices);
	void move(glm::vec2 point);

	void extract(FT_Face& face, unsigned int character, float height);

	static int move_to(const FT_Vector* to, void* user);
	static int line_to(const FT_Vector* to, void* user);
	static int conic_to(const FT_Vector* control, const FT_Vector* to, void* user);
	static int cubic_to(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user);
};

class Font
{
private:
	std::string _family;
	std::string _path;
	bool _bold = false;
	bool _italic = false;
	FT_Face* _face = nullptr;
	std::map<unsigned int, Glyph*> _glyphs;

	static FT_Library* _library;
	int _error = 0;

	float _height;

public:
	std::string family() { return _family; }
	std::string path() { return _path; }
	bool bold() { return _bold; }
	bool italic() {	return _italic; }

	// index: the face of the file, a .ttc file holds several
	Font(std::string family, std::string path, bool bold, bool italic, int index = 0);
	~Font();


	float height() { return _height; }

	Glyph* get_glyph(unsigned int character);

	int error() { return _error; }
	std::string error_text() { return FT_Error_String(_error); }

	/// <summary>
	/// return a list of polylines representing the glyph
	/// </summary>
	/// <param name="character">character to represent</param>
	/// <param name="scale">desired max character height, if 0.0f no scaling, else the glyph is scaled</param>
	/// <returns>polyline list</returns>
	std::vector<std::vector<glm::vec2>> get_glyph_coordinates(unsigned int character, float height = 0.0f);

	/// <summary>
	/// return a list of polylines representing the glyphs
	/// </summary>
	/// <param name="text">string to represent</param>
	/// <param name="indices">a reference to store table indices of polyline that represent one character.</param>
	/// <param name="scale">desired max character height, if 0.0f no scaling, else the glyph is scaled</param>
	/// <returns>polyline list</returns>
	std::vector<std::vector<glm::vec2>> get_text_coordinates(std::string text, std::vector<int>& indices, float height=0.0f, float inter_char=0.0f);

	/// <summary>
	/// return a list of bounding boxes for each glyph
	/// </summary>
	/// <param name="text"></param>
	/// <param name="height"></param>
	/// <param name="inter_char"></param>
	/// <returns></returns>
	std::vector<geometry::rectangle> get_text_bounds(std::string text, float height = 0.0f, float inter_char = 0.0f);

	glm::vec2 get_face_size(float height);
};

#endif