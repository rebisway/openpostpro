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

/************************************************************************
* Load a dxf file into a Dxf class
*************************************************************************/

#pragma once
#include <vector>
#include <string>
#include <map>
#include <glm/ext/vector_float4.hpp>
#include <variant>

class Dxf;

enum DxfSectionType 
{
	Sections,
	Entities,
	Header,
	Classes,
	Tables,
	Table,
	Blocks,
	Objects
};


class DxfCode 
{
protected:
	int _code = 0;
	std::string _val;
	std::map<std::string, std::variant<int, float, glm::vec4, std::string>> _properties;

public:
	std::map<std::string, std::variant<int, float, glm::vec4, std::string>> properties() { return _properties; }
	std::variant<int, float, glm::vec4, std::string> properties(std::string key) { return _properties[key]; }
	void properties(std::string key, std::variant<int, float, glm::vec4, std::string> value) { _properties[key] = value; }
	void read_line(std::ifstream& input);
};


class DxfVertex : public DxfCode
{
public:
	DxfVertex();
};

class DxfEntity : public DxfCode
{
private:
	std::string _type;
	std::vector<DxfVertex*> _vertices;

public:
	std::string type() { return _type; }
	void type(std::string value);
	std::vector<DxfVertex*> vertices() { return _vertices; }
	DxfVertex* vertice(int i) { return _vertices[i]; }

	DxfEntity();
	~DxfEntity();

	std::string read_line_entity(std::ifstream& input);
	std::string read_point_entity(std::ifstream& input);
	std::string read_circle_entity(std::ifstream& input);
	std::string read_ellipse_entity(std::ifstream& input);
	std::string read_text_entity(std::ifstream& input);
	std::string read_mtext_entity(std::ifstream& input);
	std::string read_arc_entity(std::ifstream& input);
	std::string read_solid_entity(std::ifstream& input);
	std::string read_insert_entity(std::ifstream& input);
	std::string read_polyline_entity(std::ifstream& input);
	std::string read_wpolyline_entity(std::ifstream& input);
	std::string read_hatch_entity(std::ifstream& input);
	std::string read_leader_entity(std::ifstream& input);
	std::string read_dimension_entity(std::ifstream& input);
	std::string read_spline_entity(std::ifstream& input);
};

class DxfSection : public DxfCode
{
protected:
	Dxf* _parent = NULL;
	DxfSectionType _type = DxfSectionType::Sections;
public:
	DxfSection(Dxf* parent) { _parent = parent; }
	DxfSectionType type() { return _type; }

	DxfEntity* create_entity();

	virtual void read(std::ifstream& input);
};


class DxfEntities : public DxfSection
{
private:
	std::vector<DxfEntity*> _items;
public:
	std::vector<DxfEntity*> items() { return _items; }

	DxfEntities(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Entities; }
	~DxfEntities();

	void read(std::ifstream& input) override;
};

class DxfHeader : public DxfSection
{
public:
	DxfHeader(Dxf* parent) : DxfSection(parent) {
		_type = DxfSectionType::Header;
		_properties["unit"] = 0; // 0 - inch ; 1 - millimetre
	}


	void read(std::ifstream& input) override;
};

class DxfClasses : public DxfSection
{
public:
	DxfClasses(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Classes; }
};

class DxfLayer : public DxfCode {
private:
	std::string _name;

public:
	std::string name() { return _name; }

	DxfLayer();

	std::string read(std::ifstream& input);
};

class DxfLineType : public DxfCode
{
private:
	std::string _name;
	std::vector<float> _pattern;

public:
	std::string name() { return _name; }
	std::vector<float> pattern() { return _pattern; }

	DxfLineType();

	std::string read(std::ifstream& input);
};

class DxfTable : public DxfSection {
private:
	std::string _name;
	std::string _type;
	std::vector<DxfLayer*> _layers;
	std::vector<DxfLineType*> _ltypes;

public:
	void name(std::string value) { _name = value; }
	std::string name() { return _name; }
	void type(std::string value) { _type = value; }
	std::string type() { return _type; }
	std::vector<DxfLayer*> layers() { return _layers; }
	std::vector<DxfLineType*> linetypes() { return _ltypes; }

	DxfTable(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Table; }
	~DxfTable();

	void read(std::ifstream& input) override;
};

class DxfTables : public DxfSection
{
private:
	std::vector<DxfTable*> _items;

public:
	std::vector<DxfTable*> items() { return _items; }

	DxfTables(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Tables; }

	void read(std::ifstream& input) override;
	DxfLayer* layer(std::string name);
};

class DxfBlocks : public DxfSection
{
public:
	DxfBlocks(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Blocks; }
};

class DxfObjects : public DxfSection
{
public:
	DxfObjects(Dxf* parent) : DxfSection(parent) { _type = DxfSectionType::Objects; }
};

class Dxf
{
private:
	std::vector<DxfSection*> _sections;
	DxfEntities* _entities = NULL;
	DxfTables* _tables = NULL;

public:
	std::vector<DxfSection*> sections() { return _sections; }
	DxfEntities* entities() { return _entities; }
	DxfTables* tables() { return _tables; }

	void read(std::string path);

	static glm::vec4 get_color(int code);

	~Dxf();
};


