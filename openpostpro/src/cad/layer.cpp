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

#include "layer.h"
#include <line.h>
#include <geometry.h>
#include <config.h>
#include <lang.h>
#include <strings.h>

void Layer::add(Shape* value)
{
	if (value->name().empty() || shape(value->name()) != nullptr)
	{
		std::string n;
		switch (value->type())
		{
		case GraphicType::Arc:
			n = Lang::t("CAD_ARC");
			break;
		case GraphicType::Circle:
			n = Lang::t("CAD_CIRCLE");
			break;
		case GraphicType::Ellipse:
			n = Lang::t("CAD_ELLIPSE");
			break;
		case GraphicType::Line:
			n = Lang::t("CAD_LINE");
			break;
		case GraphicType::Polyline:
			n = Lang::t("CAD_POLYLINE");
			break;
		case GraphicType::Point:
			n = Lang::t("CAD_POINT");
			break;
		case GraphicType::Text:
			n = Lang::t("CAD_TEXT");
			break;
		case GraphicType::Spline:
			n = Lang::t("CAD_SPLINE");
			break;
		}

		n += "_" + std::to_string(_shapes.size() + 1);
		value->name(n);
	}
	_shapes.push_back(value);
	value->parent(_name);
}

void Layer::remove(Shape* value)
{
	if (value != nullptr)
	{
		auto it = std::find(_shapes.begin(), _shapes.end(), value);
		if (it != _shapes.end())
			_shapes.erase(it);
	}
}

void Layer::remove(std::string name)
{
	Shape* shape = nullptr;
	for (Shape* s : _shapes)
	{
		if (s->name() == name)
		{
			shape = s;
			break;
		}
	}
	remove(shape);
}

void Layer::remove(unsigned int id)
{
	Shape* shape = nullptr;
	for (Shape* s : _shapes)
	{
		if (s->id() == id)
		{
			shape = s;
			break;
		}
	}
	remove(shape);
}

Shape* Layer::shape(std::string name)
{
	for (Shape* s : _shapes)
		if (s->name() == name)
			return s;
	return nullptr;
}

Shape* Layer::shape(unsigned int id)
{
	for (Shape* s : _shapes)
		if (s->id() == id)
			return s;
	return nullptr;
}

Layer::Layer(Renderer* r) : Graphic(r)
{
	_color = config.layerColor;
}

Layer::~Layer()
{
	for (Shape* shape : _shapes)
	{
		delete shape;
	}
	_shapes.clear();
}

void Layer::reset(Renderer* r)
{
	Graphic::reset(r);

	for (Shape* shape : _shapes)
	{
		shape->reset(r);
	}
}

void Layer::scaled()
{
	for (Shape* shape : _shapes)
	{
		shape->scaled();
	}
}

void Layer::draw()
{
	if (!_visible)
		return;

	// get clip rectangle
	auto top_left = _render->camera()->getPosition(0.0f, 0.0f);
	auto bottom_right = _render->camera()->getPosition((float)_render->width(), (float)_render->height());

	// start drawing
	_render->set_uniform("inner_color", _color);
	for(Shape* shape : _shapes)
	{
		// if shape out of the scope, not drawn
		if (geometry::rectangle_intersect(top_left, bottom_right, shape->topLeft(), shape->bottomRight()))
		{
			if (shape->over() && !shape->selected())
				_render->set_uniform("inner_color", config.mouseOverColor);
			else if (shape->selected())
				_render->set_uniform("inner_color", config.selectedCadColor);
			else
				_render->set_uniform("inner_color", _color);
			shape->draw();
		}
	}
}

void Layer::ui()
{
	ImGui::Begin((_name + "###OBJECT").c_str());

	ImGui::BeginTable("##TABLE_OBJECT", 2, ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL0", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##COL1", ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("VISIBLE"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool v = _visible;
	if (ImGui::Checkbox("##VISIBLE", &v))
	{
		visible(v);
	}

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("LOCKED"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	bool l = _locked;
	if (ImGui::Checkbox("##LOCKED", &l))
	{
		locked(l);
	}

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Spacing();
	ImGui::Text(Lang::l("COLOR"));
	ImGui::TableSetColumnIndex(1);
	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::ColorEdit4("##COLOR", &_color.r);

	ImGui::EndTable();
	ImGui::End();
}

void Layer::draw_anchors()
{
	if (!_visible)
		return;
	
	// get clip rectangle
	auto top_left = _render->camera()->getPosition(0.0f, 0.0f);
	auto bottom_right = _render->camera()->getPosition((float)_render->width(), (float)_render->height());

	for (Shape* shape : _shapes)
	{
		// if shape out of the scope, not drawn
		if (geometry::rectangle_intersect(top_left, bottom_right, shape->topLeft(), shape->bottomRight()))
		{
			shape->draw_anchors();
		}
	}
}

std::string Layer::write()
{
	std::string result =
		std::to_string((short)type()) + ';' +
		std::to_string(id()) + ';' +
		_name + ';' +
		geometry::to_string(_color, 0, ':') + ';' +
		std::to_string(_visible) + ';' +
		std::to_string(_locked);

	return result;
}

void Layer::read(std::string value, float version)
{
	auto data = stringex::split(value, ';');
	// 5 fields in the files written before the lock existed
	if (data.size() == 5 || data.size() == 6)
	{
		// data[0] = type
		_id = (unsigned int)std::stoi(data[1]);
		_name = data[2];
		_color = geometry::from_string(data[3], ':');
		_visible = (bool)std::stoi(data[4]);
		_locked = data.size() == 6 && (bool)std::stoi(data[5]);
	}
}
