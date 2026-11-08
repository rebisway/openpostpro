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

#include "dxf.h"
#include "file.h"
#include <strings.h>
#include <filesystem>
#include <fstream>
#include <logger.h>

///////////////////////// DXF /////////////////////

	
void Dxf::read(std::string path) {
	if (std::filesystem::exists(path))
	{
		try
		{
			int code = 0;
			std::string line;
			std::ifstream input(path);

			std::getline(input, line); // first line - code 0

			while (std::getline(input, line) && line != "EOF")
			{
				DxfSection* section = NULL;
				if (line == "HEADER") 
					section = (DxfSection*)new DxfHeader(this); 
				else if (line == "CLASSES") 
					section = (DxfSection*)new DxfClasses(this);
				else if (line == "TABLES") { 
					section = (DxfSection*)new DxfTables(this);
					_tables = (DxfTables*)section; 
				} else if (line == "BLOCKS") {
					section = (DxfSection*)new DxfBlocks(this);
					_blocks = (DxfBlocks*)section;
				}
				else if (line == "ENTITIES") { 
					section = (DxfSection*)new DxfEntities(this);
					_entities = (DxfEntities*)section; 
				} else if (line == "OBJECTS") 
					section = (DxfSection*)new DxfObjects(this);
						
				if (section != NULL) {
					section->read(input);
					_sections.push_back(section);
				}
					
				std::getline(input, line);
				code = std::stoi(line);
			}

			//for (DxfEntity* ent : _entities->items())
			//	Logger::log(ent->type());
		}
		catch (const std::exception& e)
		{
			Logger::log(e.what());
		}


		if (_tables == NULL)
			_tables = new DxfTables(this);
	}
}

glm::vec4 Dxf::get_color(int code)
{
	switch (code) {
	case 1: return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // red
	case 2: return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // yellow
	case 3: return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // green
	case 4: return glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // cyan
	case 5: return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // blue
	case 6: return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // magenta
	case 7: return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // black
	case 8: return glm::vec4(0.502f, 0.502f, 0.502f, 1.0f); // gray
	case 9: return glm::vec4(0.827f, 0.827f, 0.827f, 1.0f); // light gray;
	default: return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // white;
	}
}

Dxf::~Dxf()
{
	if (_entities != NULL)
		delete _entities;
	if (_tables != NULL)
		delete _tables;
	if (_blocks != NULL)
		delete _blocks;
}

///////////////////////// CODE /////////////////////

void DxfCode::read_line(std::ifstream& input)
{
	std::string line;
	std::getline(input, line);
	_code = std::stoi(line);
	std::getline(input, _val);
}

///////////////////////// SECTION /////////////////////

void DxfSection::read(std::ifstream& input)
{

}

DxfEntity* DxfSection::create_entity() {
	DxfEntity* entity = new DxfEntity();
	entity->type(_val);
	entity->properties("layer", "Default");
	entity->properties("name", "");
	entity->properties("handle", "");
	entity->properties("color", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	return entity;
}

///////////////////////// HEADER /////////////////////

void DxfHeader::read(std::ifstream& input)
{
	read_line(input);
	while (_val != "ENDSEC") {
		if (_val == "$MEASUREMENT")
		{
			read_line(input);
			if (_code == 70) {
				_properties["unit"] = std::stoi(_val);
			}
		}
		else if (_val == "$INSUNITS")
		{
			read_line(input);
			if (_code == 70) {
				_properties["insunits"] = std::stoi(_val);
			}
		}
		else if (_val == "$LIMMIN")
		{
			read_line(input);
			_properties["limmin.x"] = std::stof(_val);
			read_line(input);
			_properties["limmin.y"] = std::stof(_val);
		} 
		else if (_val == "$LIMMAX")
		{
			read_line(input);
			_properties["limmax.x"] = std::stof(_val);
			read_line(input);
			_properties["limmax.y"] = std::stof(_val);
		}
		else if (_val == "$EXTMIN")
		{
			read_line(input);
			_properties["extmin.x"] = std::stof(_val);
			read_line(input);
			_properties["extmin.y"] = std::stof(_val);
		}
		else if (_val == "$EXTMAX")
		{
			read_line(input);
			_properties["extmax.x"] = std::stof(_val);
			read_line(input);
			_properties["extmax.y"] = std::stof(_val);
		}
		read_line(input);
	}
}

///////////////////////// TABLES /////////////////////

void DxfTables::read(std::ifstream& input) {
	read_line(input);
	while (_val != "ENDSEC") {
		if (_val == "TABLE")
		{
			DxfTable* b = new DxfTable(_parent);
			b->read(input);
			_items.push_back(b);
			read_line(input);
		}
		else
			read_line(input);
	}
}

DxfLayer* DxfTables::layer(std::string name) {
	for(DxfTable* t : _items) {
		for(DxfLayer* l : t->layers()) {
			if (l->name() == name)
				return l;
		}
	}
	return NULL;
}

///////////////////////// TABLE /////////////////////

DxfTable::~DxfTable()
{
	for (DxfLayer* l : _layers)
		delete l;
	_layers.clear();
	for (DxfLineType* l : _ltypes)
		delete l;
	_ltypes.clear();
}

void DxfTable::read(std::ifstream& input)
{
	read_line(input);
	while (_val != "ENDTAB") {
		switch (_code) {
		case 0: // element
			if (_val == "LAYER")
			{
				DxfLayer* l = new DxfLayer();
				_val = l->read(input);
				_layers.push_back(l);
			}
			else if (_val == "LTYPE")
			{
				DxfLineType* lt = new DxfLineType();
				_val = lt->read(input);
				_ltypes.push_back(lt);
			}
			else
			{
				read_line(input);
				while (_code != 0)
					read_line(input);
				break;
			}
			break;
		case 2:		// Table name
			_properties["name"] = _val;
			read_line(input);
			break;
		case 5:		// Handle
			_properties["handle"] = _val;
			read_line(input);
			break;
		case 70: // max number of elements
			_code = std::stoi(_val);
			read_line(input);
			break;
		default:
			read_line(input);
			break;
		}
	}
}

///////////////////////// LAYER /////////////////////

DxfLayer::DxfLayer()
{
	_properties["handle"] = "";
	_properties["color"] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

std::string DxfLayer::read(std::ifstream& input)
{
	read_line(input);

	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 2:		// Layer Name
			_properties["name"] = _val;
			_name = _val;
			break;
		case 6:	// line type
			_properties["line"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 70:	// Block description
			_properties["flag"] = std::stoi(_val);
			break;
		case 370:	// Lineweight
			_properties["lineweight"] = _val;
			break;
		}
		read_line(input);
	}
	return _val;
}

///////////////////////// LINETYPE /////////////////////

DxfLineType::DxfLineType()
{
	_properties["handle"] = "";
	_properties["color"] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

std::string DxfLineType::read(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
			switch (_code) {
			case 5:		// Handle
				_properties["handle"] = _val;
				break;
			case 2:		// Layer Name
				_properties["name"] = _val;
				_name = _val;
				break;
			case 3:		// Layer Name
				_properties["description"] = _val;
				break;
			case 6:	// line type
				_properties["line"] = _val;
				break;
			case 40:	// pattern length
				_properties["length"] = std::stof(_val);
				break;
			case 49:	// pattern length
				_pattern.push_back(std::stof(_val));
				break;
			case 62:	// Color
				_properties["color"] = Dxf::get_color(std::stoi(_val));
				break;
			case 70:	// Block description
				_properties["flag"] = std::stoi(_val);
				break;
			case 73:	// number of elements
				_properties["count"] = std::stoi(_val);
				break;
			}
			read_line(input);
	}
	return _val;
}

///////////////////////// VERTEX /////////////////////

DxfVertex::DxfVertex()
{
	_properties["bulge"] = 0.0f;
	_properties["handle"] = "";
}

///////////////////////// ENTITIES /////////////////////

DxfEntities::~DxfEntities()
{
	for (DxfEntity* e : _items)
		delete e;
	_items.clear();
}

void DxfEntities::read(std::ifstream& input)
{
	read_line(input);
	read_items(input, _items, "ENDSEC");
}

void DxfSection::read_items(std::ifstream& input, std::vector<DxfEntity*>& items, const std::string& stop)
{
	DxfEntity* entity = NULL;
	while (_val != stop) {
		entity = create_entity();
		if (_val == "LINE")
			_val = entity->read_line_entity(input);
		else if (_val == "POINT")
			_val = entity->read_point_entity(input);
		else if (_val == "CIRCLE")
			_val = entity->read_circle_entity(input);
		else if (_val == "ELLIPSE")
			_val = entity->read_ellipse_entity(input);
		else if (_val == "TEXT" || _val == "ATTRIB")
			_val = entity->read_text_entity(input);
		else if (_val == "MTEXT")
			_val = entity->read_mtext_entity(input);
		else if (_val == "ARC")
			_val = entity->read_arc_entity(input);
		else if (_val == "SOLID")
			_val = entity->read_solid_entity(input);
		else if (_val == "INSERT")
			_val = entity->read_insert_entity(input);
		else if (_val == "POLYLINE")
			_val = entity->read_polyline_entity(input);
		else if (_val == "LWPOLYLINE")
			_val = entity->read_wpolyline_entity(input);
		else if (_val == "HATCH")
			_val = entity->read_hatch_entity(input);
		else if (_val == "LEADER")
			_val = entity->read_leader_entity(input);
		else if (_val == "DIMENSION")
			_val = entity->read_dimension_entity(input);
		else if (_val == "SPLINE")
			_val = entity->read_spline_entity(input);
		else
		{
			delete entity;
			entity = NULL;
			std::string et = _val;
			read_line(input);
			while (_code != 0) {
				if (et != "SEQEND" && _code == 5)
					Logger::log("DXF ENTITY " + et + "(" + _val + ") unknown");
				read_line(input);
			}
		}
		if (entity != NULL)
			items.push_back(entity);
	}
}


///////////////////////// BLOCKS /////////////////////

DxfBlock::~DxfBlock()
{
	for (DxfEntity* e : _items)
		delete e;
	_items.clear();
}

DxfBlocks::~DxfBlocks()
{
	for (auto& b : _blocks)
		delete b.second;
	_blocks.clear();
}

DxfBlock* DxfBlocks::block(std::string name)
{
	auto it = _blocks.find(name);
	return it == _blocks.end() ? nullptr : it->second;
}

void DxfBlocks::read(std::ifstream& input)
{
	read_line(input);
	while (_val != "ENDSEC") {
		if (_val != "BLOCK") {
			// nothing else is expected between the blocks: skip to the next item
			read_line(input);
			while (_code != 0 && !input.eof())
				read_line(input);
			if (input.eof())
				return;
			continue;
		}

		DxfBlock* block = new DxfBlock();
		float x = 0.0f, y = 0.0f;
		read_line(input);
		while (_code != 0) {
			if (_code == 2)
				block->name(_val);
			else if (_code == 10)
				x = std::stof(_val);
			else if (_code == 20)
				y = std::stof(_val);
			read_line(input);
		}
		block->base(x, y);
		read_items(input, block->items(), "ENDBLK");

		// the end of the block: its own codes (handle, layer) are skipped
		read_line(input);
		while (_code != 0)
			read_line(input);

		// a block which has no name or which comes twice is dropped
		if (block->name().empty() || _blocks.count(block->name()) > 0)
			delete block;
		else
			_blocks[block->name()] = block;
	}
}

///////////////////////// ENTITY /////////////////////

void DxfEntity::type(std::string value)
{
	_type = value;
	
	if (_type == "TEXT" || _type == "ATTRIB") {
		_properties["justify"] = 0;
		_properties["scale"] = 1.0f;
		_properties["angle"] = 0.0f;
		_properties["horizontal.justify"] = 0;
		_properties["vertical.justify"] = 0;
	}
	if (_type == "MTEXT") {
		_properties["scale"] = 1.0f;
		_properties["angle"] = 0.0f;
	}
	if (_type == "INSERT") {
		_properties["xscale"] = 1.0f;
		_properties["yscale"] = 1.0f;
		_properties["zscale"] = 1.0f;
		_properties["angle"] = 0.0f;
	}
	if (_type == "ELLIPSE") {
		_properties["start"] = 0.0f;
		_properties["end"] = 6.283185307179586f;
	}
}

DxfEntity::DxfEntity()
{
	_properties["layer"] = "0";
	_properties["handle"] = "";
}

DxfEntity::~DxfEntity()
{
	for (DxfVertex* v : _vertices)
		delete v;
	_vertices.clear();
}

std::string DxfEntity::read_line_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	 // Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// Start ref_point X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// Start ref_point Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// Start ref_point Z
			_properties["p1.z"] = std::stof(_val);
			break;
		case 11:	// End ref_point X
			_properties["p2.x"] = std::stof(_val);
			break;
		case 21:	// End ref_point Y
			_properties["p2.y"] = std::stof(_val);
			break;
		case 31:	// End ref_point Z
			_properties["p2.z"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_point_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// Start ref_point X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// Start ref_point Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// Start ref_point Z
			_properties["p1.z"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_circle_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// center ref_point X
			_properties["center.x"] = std::stof(_val);
			break;
		case 20:	// center ref_point Y
			_properties["center.y"] = std::stof(_val);
			break;
		case 30:	// center ref_point Z
			_properties["center.z"] = std::stof(_val);
			break;
		case 40:	// Radius
			_properties["radius"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_ellipse_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// center ref_point X
			_properties["center.x"] = std::stof(_val);
			break;
		case 20:	// center ref_point Y
			_properties["center.y"] = std::stof(_val);
			break;
		case 30:	// center ref_point Z
			_properties["center.z"] = std::stof(_val);
			break;
		case 11:	// Endpoint of major axis X
			_properties["major.x"] = std::stof(_val);
			break;
		case 21:	// Endpoint of major axis Y
			_properties["major.y"] = std::stof(_val);
			break;
		case 31:	// Endpoint of major axis Z
			_properties["major1.z"] = std::stof(_val);
			break;
		case 40:	// Ratio of minor axis to major axis
			_properties["ratio"] = std::stof(_val);
			break;
		case 41:	// Start parameter (this value is 0.0 for a full ellipse)
			_properties["start"] = std::stof(_val);
			break;
		case 42:	// End parameter (this value is 2pi for a full ellipse)
			_properties["end"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_text_entity(std::ifstream& input)
{
	read_line(input);
	_properties["style"] = "STANDARD";
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 7:		// Text Style Name
			_properties["style"] = _val;
			break;
		case 10:	// First alignment ref_point (in OCS) X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// First alignment ref_point (in OCS) Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// First alignment ref_point (in OCS) Z
			_properties["p1.z"] = std::stof(_val);
			break;
		case 72:	// Horizontal text justification type (default = 0)
			_properties["horizontal.justify"] = std::stoi(_val);
			break;
		case 73:	// Vertical text justification type (default = 0)
			_properties["vertical.justify"] = std::stoi(_val);
			break;
		case 11:	// Second alignment ref_point (in OCS) X
			_properties["p2.x"] = std::stof(_val);
			break;
		case 21:	// Second alignment ref_point (in OCS) Y
			_properties["p2.y"] = std::stof(_val);
			break;
		case 31:	// Second alignment ref_point (in OCS) Z
			_properties["p2.z"] = std::stof(_val);
			break;
		case 40:	// Text Height
			_properties["height"] = std::stof(_val);
			break;
		case 41:	// Width Factor
			_properties["widthfactor"] = std::stof(_val);
			break;
		case 50:	// Text Rotation Angle
			_properties["angle"] = std::stof(_val);
			break;
		case 1:		// Default value (the string itself)
			_properties["value"] = _val;
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_mtext_entity(std::ifstream& input)
{
	read_line(input);
	_properties["value"] = "";
	while (_code != 0) {
		switch (_code) {
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 7:		// Text Style Name
			_properties["style"] = _val;
			break;
		case 10:	// First alignment ref_point (in OCS) X
			_properties["first.x"] = std::stof(_val);
			break;
		case 20:	// First alignment ref_point (in OCS) Y
			_properties["first.y"] = std::stof(_val);
			break;
		case 30:	// First alignment ref_point (in OCS) Z
			_properties["first.z"] = std::stof(_val);
			break;
		case 71:	// Attachement
			_properties["attachement"] = std::stoi(_val);
			break;
		case 72:	// Drawing direction
			_properties["direction"] = std::stoi(_val);
			break;
		case 73:	//  Line spacing style
			_properties["line.spacing.style"] = std::stoi(_val);
			break;
		case 11:	// Second alignment ref_point (in OCS) X
			_properties["second.x"] = std::stof(_val);
			break;
		case 21:	// Second alignment ref_point (in OCS) Y
			_properties["second.y"] = std::stof(_val);
			break;
		case 31:	// Second alignment ref_point (in OCS) Z
			_properties["second.z"] = std::stof(_val);
			break;
		case 40:	// Text Height
			_properties["height"] = std::stof(_val);
			break;
		case 41:	// Width Factor
			_properties["widthrectangle"] = std::stof(_val);
			break;
		case 44:	// Line spacing factor
			_properties["line.spacing.factor"] = std::stof(_val);
			break;
		case 50:	// Text Rotation Angle
			_properties["angle"] = std::stof(_val);
			break;
		case 1:		// Default value (the string itself)
		case 3:
			_properties["value"] = std::get<std::string>(_properties["value"]) + _val;
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_arc_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// center ref_point X
			_properties["center.x"] = std::stof(_val);
			break;
		case 20:	// center ref_point Y
			_properties["center.y"] = std::stof(_val);
			break;
		case 30:	// center ref_point Z
			_properties["center.z"] = std::stof(_val);
			break;
		case 40:	// Radius
			_properties["radius"] = std::stof(_val);
			break;
		case 50:	// Start angle
			_properties["start"] = std::stof(_val);
			break;
		case 51:	// End angle
			_properties["end"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_solid_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// First corner X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// First corner Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// First corner Z
			_properties["p1.z"] = std::stof(_val);
			break;
		case 11:	// Second corner X
			_properties["p3.x"] = std::stof(_val);
			break;
		case 21:	// Second corner Y
			_properties["p3.y"] = std::stof(_val);
			break;
		case 31:	// Second corner Z
			_properties["p3.z"] = std::stof(_val);
			break;
		case 12:	// Third corner X
			_properties["p3.x"] = std::stof(_val);
			break;
		case 22:	// Third corner Y
			_properties["p3.y"] = std::stof(_val);
			break;
		case 32:	// Third corner Z
			_properties["p3.z"] = std::stof(_val);
			break;
		case 13:	// Fourth corner X
			_properties["p4.x"] = std::stof(_val);
			break;
		case 23:	// Fourth corner Y
			_properties["p4.y"] = std::stof(_val);
			break;
		case 33:	// Fourth corner Z
			_properties["p4.z"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_insert_entity(std::ifstream& input)
{
	read_line(input);
	_properties["p1.x"] = 0.0f;
	_properties["p1.y"] = 0.0f;
	_properties["p1.z"] = 0.0f;
	_properties["xscale"] = 1.0f;
	_properties["yscale"] = 1.0f;
	_properties["zscale"] = 1.0f;
	_properties["angle"] = 0.0f;
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 2:		// Layer Name
			_properties["block"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// center ref_point X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// center ref_point Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// center ref_point Z
			_properties["p1.z"] = std::stof(_val);
			break;
		case 41:	// X scale factor (optional; default = 1)
			_properties["xscale"] = std::stof(_val);
			break;
		case 42:	// Y scale factor (optional; default = 1)
			_properties["yscale"] = std::stof(_val);
			break;
		case 43:	// Z scale factor (optional; default = 1)
			_properties["zscale"] = std::stof(_val);
			break;
		case 50:	// Rotation angle (optional; default = 0) in radian
			_properties["angle"] = std::stof(_val);
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_polyline_entity(std::ifstream& input)
{
	_properties["flag"] = 0;
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			//					if ( val == "58" )
			//						GG.Trace.Break();
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 70:	// Polyline flag (bit-coded); default is 0
			_properties["flag"] = std::stoi(_val);
			break;
		}
		read_line(input);
	}

	int point = -1;
	while (_code == 0 && _val == "VERTEX")
	{
		read_line(input);
		DxfVertex* vx = new DxfVertex();
		vx->properties("handle", _properties["handle"]);
		_vertices.push_back(vx);
		while (_code != 0) {
			switch (_code) {
			case 10:	// Start ref_point X
				point++;
				//properties["p" + ref_point.ToString() + ".x"] = std::stof(_val);
				vx->properties("x", std::stof(_val));
				break;
			case 20:	// Start ref_point Y
				//properties["p" + ref_point.ToString() + ".y"] = std::stof(_val);
				vx->properties("y", std::stof(_val));
				break;
			case 30:	// Start ref_point Z
				//properties["p" + ref_point.ToString() + ".z"] = std::stof(_val);
				vx->properties("z", std::stof(_val));
				break;
			case 5:	// Handle
				vx->properties("handle", _val);
				break;
			case 42:	// Bulge (optional; default is 0)
				vx->properties("bulge", std::stof(_val));
				break;
			}
			read_line(input);
		}
	}
	_properties["count"] = point + 1;

	return _val;
}

std::string DxfEntity::read_wpolyline_entity(std::ifstream& input)
{
	_properties["flag"] = 0;
	read_line(input);
	int lwp_count = 0;
	while (_code != 10) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 70:	// Polyline flag (bit-coded); default is 0
			_properties["flag"] = std::stoi(_val);
			break;
		case 90:
			_properties["count"] = lwp_count = std::stoi(_val);
			break;
		}
		read_line(input);
	}

	float x = 0, y = 0, bulge = 0;
	DxfVertex* v = NULL;

	int twice = 2;
	do {
		switch (_code) {
		case 10:	// Point X
			twice--;
			if (twice == 0) {
				v = new DxfVertex();
				v->properties("x", x);
				v->properties("y", y);
				v->properties("bulge", bulge);
				v->properties("handle", _properties["handle"]);
				_vertices.push_back(v);
				twice = 1;
				bulge = 0;
			}
			x = std::stof(_val);
			break;
		case 20:	// Point Y
			y = std::stof(_val);
			break;
		case 42:
			bulge = std::stof(_val);
			break;
		}
		read_line(input);
	} while (_code == 10 || _code == 20 || _code == 42);

	v = new DxfVertex();
	v->properties("x", x);
	v->properties("y", y);
	v->properties("bulge", bulge);
	v->properties("handle", _properties["handle"]);
	_vertices.push_back(v);


	while (_code != 0) // ignore the rest
		read_line(input);
	return _val;
}

std::string DxfEntity::read_hatch_entity(std::ifstream& input)
{
	read_line(input);
	_properties["flag"] = 0;
	int hp_count = 0;
	while (_code != 93) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 70:	// hatch solid flag (bit-coded); default is 0
			_properties["flag"] = std::stoi(_val);
			break;
		case 71:	// hatch associativity flag (bit-coded); default is 0
			_properties["associativity"] = std::stoi(_val);
			break;
		case 75:	// hatch style
			_properties["style"] = std::stoi(_val);
			break;
		case 76:	// hatch pattern type
			_properties["pattern type"] = std::stoi(_val);
			break;
		case 52:	// hatch pattern angle
			_properties["pattern angle"] = std::stoi(_val);
			break;
		case 41:	// hatch pattern scale
			_properties["pattern scale"] = std::stoi(_val);
			break;
		case 10:	// Elevation ref_point X
			_properties["elevation.x"] = std::stof(_val);
			break;
		case 20:	// Elevation ref_point Y
			_properties["elevation.y"] = std::stof(_val);
			break;
		case 30:	// Elevation ref_point Z
			_properties["elevation.z"] = std::stof(_val);
			break;
		}
		read_line(input);
	}

	// 93 counts
	_properties["count"] = hp_count = std::stoi(_val);

	for (int lwp_point = 0; lwp_point < hp_count; lwp_point++)
	{
		int twice = 2;
		do {
			switch (_code) {
			case 10:	// Point X
				_properties["p" + std::to_string(lwp_point) + ".x"] = std::stof(_val);
				break;
			case 20:	// Point Y
				_properties["p" + std::to_string(lwp_point) + ".y"] = std::stof(_val);
				break;
			}
			read_line(input);
			twice--;
		} while (twice > 0);
	}

	while (_code != 0) // ignore the rest
		read_line(input);
	return _val;
}

std::string DxfEntity::read_leader_entity(std::ifstream& input)
{
	read_line(input);
	int ldr_count = 0;
	_properties["path"] = 0; // default
	while (_code != 10) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 72:	// Polyline flag (bit-coded); default is 0
			_properties["path"] = std::stoi(_val);
			break;
		case 76:
			_properties["count"] = ldr_count = std::stoi(_val);
			break;
		}
		read_line(input);
	}

	for (int lwp_point = 0; lwp_point < ldr_count; lwp_point++)
	{
		int twice = 2;
		do {
			switch (_code) {
			case 10:	// Point X
				_properties["p" + std::to_string(lwp_point) + ".x"] = std::stof(_val);
				break;
			case 20:	// Point Y
				_properties["p" + std::to_string(lwp_point) + ".y"] = std::stof(_val);
				break;
			case 30:	// Point Z
				_properties["p" + std::to_string(lwp_point) + ".z"] = std::stof(_val);
				break;
			}
			read_line(input);
			twice--;
		} while (twice > 0);
	}

	while (_code != 0) // ignore the rest
		read_line(input);
	return _val;
}

std::string DxfEntity::read_dimension_entity(std::ifstream& input)
{
	read_line(input);
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	// Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 10:	// Definition ref_point (in WCS) X
			_properties["p1.x"] = std::stof(_val);
			break;
		case 20:	// Definition ref_point (in WCS) Y
			_properties["p1.y"] = std::stof(_val);
			break;
		case 30:	// Definition ref_point (in WCS) Z
			_properties["p1.z"] = std::stof(_val);
			break;
		case 13:	// Definition ref_point for linear and angular dimensions (in WCS) X
			_properties["defp3.x"] = std::stof(_val);
			break;
		case 23:	// Definition ref_point for linear and angular dimensions (in WCS) Y
			_properties["defp3.y"] = std::stof(_val);
			break;
		case 33:	// Definition ref_point for linear and angular dimensions (in WCS) Z
			_properties["defp3.z"] = std::stof(_val);
			break;
		case 14:	// Definition ref_point for linear and angular dimensions (in WCS) X
			_properties["defp4.x"] = std::stof(_val);
			break;
		case 24:	// Definition ref_point for linear and angular dimensions (in WCS) Y
			_properties["defp4.y"] = std::stof(_val);
			break;
		case 34:	// Definition ref_point for linear and angular dimensions (in WCS) Z
			_properties["defpoint4.z"] = std::stof(_val);
			break;
		case 53:	// The rotation angle of the dimension text away from its default orientation (the direction of the dimension line)
			_properties["angle"] = std::stof(_val);
			break;
		case 3:		// Dimension style name
			_properties["style"] = _val;
			break;
		case 1:		// Dimension text explicitly entered by the user. Optional; default is the measurement. If null or "<>", the dimension measurement is drawn as the text, if " " (one blank space), the text is suppressed.
			_properties["text"] = _val;
			break;
		case 2:		// Name of the block that contains the entities that make up the dimension picture
			_properties["blockname"] = _val;
			break;
		}
		read_line(input);
	}
	return _val;
}

std::string DxfEntity::read_spline_entity(std::ifstream& input)
{
	read_line(input);
	int si = 0;
	int fi = 0;
	while (_code != 0) {
		switch (_code) {
		case 5:		// Handle
			_properties["handle"] = _val;
			break;
		case 8:		// Layer Name
			_properties["layer"] = _val;
			break;
		case 62:	 // Color
			_properties["color"] = Dxf::get_color(std::stoi(_val));
			break;
		case 6:		// Line Type
			_properties["type"] = _val;
			break;
		case 39:	// Thickness
			_properties["thickness"] = std::stof(_val);
			break;
		case 48:	// Linetype scale
			_properties["scale"] = std::stof(_val);
			break;
		case 70:	// Flag
			_properties["flag"] = std::stoi(_val);
			break;
		case 71:	// Degree
			_properties["degree"] = std::stoi(_val);
			break;
		case 72:	// Number of knots
			_properties["knots"] = std::stoi(_val);
			break;
		case 73:	// Number of control points
			_properties["count"] = std::stoi(_val);
			break;
		case 74:	// Number of fit points
			_properties["fits"] = std::stoi(_val);
			break;
		case 42:	// knot tolerance
			_properties["ktolerance"] = std::stof(_val);
			break;
		case 43:	// Control-ref_point tolerance
			_properties["ctolerance"] = std::stof(_val);
			break;
		case 44:	// Fit-ref_point tolerance
			_properties["ftolerance"] = std::stof(_val);
			break;
		case 40:	// Knot value
			_properties["knot.list"] = (_properties.count("knot.list") ? std::get<std::string>(_properties["knot.list"]) + "," : std::string()) + _val;
			break;
		case 41:	// Weight
			_properties["weight.list"] = (_properties.count("weight.list") ? std::get<std::string>(_properties["weight.list"]) + "," : std::string()) + _val;
			break;
		case 11:	// Fit ref_point X
			_properties["f" + std::to_string(fi) + ".x"] = std::stof(_val);
			break;
		case 21:	// Fit ref_point Y
			_properties["f" + std::to_string(fi) + ".y"] = std::stof(_val);
			break;
		case 31:	// Fit ref_point Z
			fi++;
			break;
		case 10:	// Start ref_point X
			_properties["p" + std::to_string(si) + ".x"] = std::stof(_val);
			break;
		case 20:	// Start ref_point Y
			_properties["p" + std::to_string(si) + ".y"] = std::stof(_val);
			break;
		case 30:	// Start ref_point Z
			_properties["p" + std::to_string(si) + ".z"] = std::stof(_val);
			si++;
			break;
		}
		read_line(input);
	}
	return _val;
}

