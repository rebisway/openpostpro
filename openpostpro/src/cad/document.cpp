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

#include "document.h"
#include <chrono>
#include <future>

#include <logger.h>
#include <circle.h>
#include <geometry.h>
#include <lang.h>
#include "strings.h"
#include <arc.h>
#include <ellipse.h>
#include <line.h>
#include <point.h>
#include <polyline.h>
#include <text.h>
#include <spline.h>
#include <offset.h>
#include <moveTo.h>
#include <follow.h>
#include <pocket.h>
#include <spiral.h>
#include <drill.h>
#include <parallel.h>

Document::Document()
{
}

Document::~Document()
{
	clear();
}

void Document::check()
{
	if (_layers.size() == 0)
		layer();

	//for (Layer* l : _layers)
	//{
	//	if (l->name().empty())
	//	{
	//		auto name = Lang::t("LAYER") + "_" + std::to_string(_layers.size() + 1);
	//		l->name(name);
	//	}
	//	for (Shape* s : l->shapes())
	//		s->parent(l->name());
	//}

	if (_groups.size() == 0)
		group("");

	for (Group* g : _groups)
	{
		if (g->name().empty())
		{
			auto name = Lang::t("GROUP") + "_" + std::to_string(_groups.size() + 1);
			g->name(name);
		}
		for (Toolpath* m : g->toolpaths())
			m->parent(g->name());
	}
}


Layer* Document::layer(std::string name)
{
	for (Layer* l : _layers)
		if (l->name() == name)
			return l;

	if (name.empty())
	{
		auto count = _layers.size() + 1;
		bool found = false;
		do
		{
			name = Lang::t("LAYER") + "_" + std::to_string(count++);
			for (Layer* c : _layers)
			{
				if (found = (c->name() == name))
					break;
			}
		} while (found);
	}

	glm::vec4 colors[17]{
		glm::vec4(1.000000f, 1.000000f, 0.000000f, 1.000000f), // yellow
		glm::vec4(0.000000f, 1.000000f, 1.000000f, 1.000000f), // cyan
		glm::vec4(0.000000f, 0.000000f, 1.000000f, 1.000000f), // blue
		glm::vec4(1.000000f, 0.000000f, 1.000000f, 1.000000f), // magenta
		glm::vec4(0.000000f, 0.000000f, 0.000000f, 1.000000f), // black
		glm::vec4(0.501961f, 0.501961f, 0.501961f, 1.000000f), // gray
		glm::vec4(0.647059f, 0.164706f, 0.164706f, 1.000000f), // brown
		glm::vec4(0.941176f, 0.901961f, 0.549020f, 1.000000f), // khaki
		glm::vec4(0.000000f, 0.392157f, 0.000000f, 1.000000f), // dark green
		glm::vec4(0.274510f, 0.509804f, 0.705882f, 1.000000f), // steel blue
		glm::vec4(0.000000f, 0.000000f, 0.545098f, 1.000000f), // dark blue
		glm::vec4(0.501961f, 0.000000f, 0.501961f, 1.000000f), // purple
		glm::vec4(0.662745f, 0.662745f, 0.662745f, 1.000000f), // dark gray
		glm::vec4(1.000000f, 1.000000f, 1.000000f, 1.000000f), // white
		glm::vec4(0.827451f, 0.827451f, 0.827451f, 1.000000f), // light gray
		glm::vec4(1.000000f, 0.000000f, 0.000000f, 1.000000f), // red
		glm::vec4(0.000000f, 0.501961f, 0.000000f, 1.000000f)  // green
	};
	int index = (int)(_layers.size() > 17 ? 0 : _layers.size());

	Layer* l = new Layer(_render);
	l->name(name);
	l->color(colors[index]);
	l->document(this);
	_layers.push_back(l);
	if (_current_layer == nullptr)
		_current_layer = l;
	return l;
}

Layer* Document::layer_of(Shape* shape)
{
	// the layer of a shape is never created here: a shape without a valid parent (a file written by an older version) would get a
	// new layer at each call, and the layers would multiply
	for (Layer* l : _layers)
		if (l->name() == shape->parent())
			return l;
	for (Layer* l : _layers)
		if (std::find(l->shapes().begin(), l->shapes().end(), shape) != l->shapes().end())
			return l;
	return _layers.size() > 0 ? _layers[0] : layer();
}

Group* Document::group(std::string name)
{
	for (Group* g : _groups)
		if (g->name() == name)
			return g;
	if (name.empty())
	{
		auto count = _groups.size() + 1;
		bool found = false;
		do
		{
			name = Lang::t("GROUP") + "_" + std::to_string(count++);
			for (Group* c : _groups)
			{
				if (found = (c->name() == name))
					break;
			}
		} while (found);
	}

	glm::vec4 colors[5]{
		glm::vec4(1.000000f, 0.639000f, 0.000000f, 1.000000f), // orange
		glm::vec4(0.988000f, 0.173000f, 0.000000f, 1.000000f), // red
		glm::vec4(0.988000f, 0.000000f, 0.290000f, 1.000000f), // light magenta
		glm::vec4(0.988000f, 0.000000f, 0.514000f, 1.000000f), // fushia
		glm::vec4(0.988000f, 0.000000f, 0.839000f, 1.000000f), // pink
	};
	int index = (int)(_groups.size() > 5 ? 0 : _groups.size());

	Group* g = new Group(_render);
	g->name(name);
	g->color(colors[index]);
	_groups.push_back(g);
	if (_current_group == nullptr)
		_current_group = g;
	return g;
}

std::vector<glm::vec2> Document::reference(Shape* r)
{
	std::vector<glm::vec2> vertices;

	if (r != nullptr)
	{
		if (r->type() == GraphicType::Circle)
		{
			auto points = geometry::circle(((Circle*)r)->radius());
			auto c = ((Circle*)r)->center();
			for (glm::vec2 v : points)
				vertices.push_back(glm::vec2(v.x + c.x, v.y + c.y));
		}
	}

	return vertices;
}

void Document::remove_layer(Layer* value)
{
	auto it = std::find(_layers.begin(), _layers.end(), value);
	if (it != _layers.end())
	{
		(*it)->selected(false);
		// its shapes leave the document
		for (Shape* shape : (*it)->shapes())
			block_remove(shape);
		(*it)->document(nullptr);
		bool current = _current_layer == *it;
		// the layer which takes the place: the previous one, or the next one if it was the first
		size_t index = it - _layers.begin();
		_layers.erase(it);
		if (current)
			{
			_current_layer = _layers.empty() ? nullptr : _layers[index > 0 ? index - 1 : 0];
			// a document always has a layer
			if (_current_layer == nullptr)
				layer();
		}
	}
}

void Document::remove_layer(std::string name)
{
	remove_layer(layer(name));
}

void Document::remove_group(Group* value)
{
	auto it = std::find(_groups.begin(), _groups.end(), value);
	if (it != _groups.end())
	{
		(*it)->selected(false);
		bool current = _current_group == *it;
		size_t index = it - _groups.begin();
		_groups.erase(it);
		if (current)
			{
			_current_group = _groups.empty() ? nullptr : _groups[index > 0 ? index - 1 : 0];
			// a document always has a group
			if (_current_group == nullptr)
				group();
		}
	}
}

void Document::remove_group(std::string name)
{
	remove_group(group(name));
}

void Document::up(Graphic* g)
{
	if (g->type() == GraphicType::Group)
	{
		auto index = std::find(_groups.begin(), _groups.end(), (Group*)g);
		if (index != _groups.begin())
		{
			auto previous = std::prev(index);
			_groups.erase(index);
			_groups.insert(previous, (Group*)g);
		}
	}
	if (g->toolpath())
	{
		auto gp = group(((Toolpath*)g)->parent());
		auto index = std::find(gp->toolpaths().begin(), gp->toolpaths().end(), (Toolpath*)g);
		if (index != gp->toolpaths().begin())
		{
			auto previous = std::prev(index);
			gp->toolpaths().erase(index);
			gp->toolpaths().insert(previous, (Toolpath*)g);
		}
	}
	if (g->type() == GraphicType::Layer)
	{
		auto index = std::find(_layers.begin(), _layers.end(), (Layer*)g);
		if (index != _layers.begin())
		{
			auto previous = std::prev(index);
			_layers.erase(index);
			_layers.insert(previous, (Layer*)g);
		}
	}
	if (g->shape())
	{
		auto gp = layer_of((Shape*)g);
		auto index = std::find(gp->shapes().begin(), gp->shapes().end(), (Shape*)g);
		if (index != gp->shapes().begin())
		{
			auto previous = std::prev(index);
			gp->shapes().erase(index);
			gp->shapes().insert(previous, (Shape*)g);
		}
	}
}

void Document::down(Graphic* g)
{
	if (g->type() == GraphicType::Group)
	{
		auto index = std::find(_groups.begin(), _groups.end(), (Group*)g);
		if (index != _groups.end() - 1)
		{
			auto next = *std::next(index);
			_groups.erase(index);
			auto it = std::find(_groups.begin(), _groups.end(), next);
			if (it == _groups.end() - 1)
				_groups.push_back((Group*)g);
			else
				_groups.insert(std::next(it), (Group*)g);
		}
	}
	if (g->toolpath())
	{
		auto gp = group(((Toolpath*)g)->parent());
		auto index = std::find(gp->toolpaths().begin(), gp->toolpaths().end(), (Toolpath*)g);
		if (index != gp->toolpaths().end() - 1)
		{
			auto next = *std::next(index);
			gp->toolpaths().erase(index);
			auto it = std::find(gp->toolpaths().begin(), gp->toolpaths().end(), next);
			if (it == gp->toolpaths().end() - 1)
				gp->toolpaths().push_back((Toolpath*)g);
			else
				gp->toolpaths().insert(std::next(it), (Toolpath*)g);
		}
	}
	if (g->type() == GraphicType::Layer)
	{
		auto index = std::find(_layers.begin(), _layers.end(), (Layer*)g);
		if (index != _layers.end() - 1)
		{
			auto next = *std::next(index);
			_layers.erase(index);
			auto it = std::find(_layers.begin(), _layers.end(), next);
			if (it == _layers.end() - 1)
				_layers.push_back((Layer*)g);
			else
				_layers.insert(std::next(it), (Layer*)g);
		}
	}
	if (g->shape())
	{
		auto gp = layer_of((Shape*)g);
		auto index = std::find(gp->shapes().begin(), gp->shapes().end(), (Shape*)g);
		if (index != gp->shapes().end() - 1)
		{
			auto next = *std::next(index);
			gp->shapes().erase(index);
			auto it = std::find(gp->shapes().begin(), gp->shapes().end(), next);
			if (it == gp->shapes().end() - 1)
				gp->shapes().push_back((Shape*)g);
			else
				gp->shapes().insert(std::next(it), (Shape*)g);
		}
	}
}

void Document::select_all()
{
	_selected.clear();
	for (Layer* l : _layers)
	{
		if (!l->selectable())
			continue;
		for (Shape* s : l->shapes())
		{
			s->selected(true);
			_selected.push_back(s);
		}
	}
}

void Document::unselect_all()
{
	for (Graphic* g : _selected)
		g->selected(false);
	_selected.clear();
}

const std::vector<Shape*>& Document::block_shapes(const std::string& name)
{
	static const std::vector<Shape*> none;
	auto it = _block_index.find(name);
	return it == _block_index.end() ? none : it->second;
}

void Document::block_add(Shape* shape)
{
	if (shape->block().empty())
		return;
	auto it = _block_index.find(shape->block());
	if (it == _block_index.end())
	{
		it = _block_index.emplace(shape->block(), std::vector<Shape*>()).first;
		_block_names.push_back(shape->block());
	}
	if (std::find(it->second.begin(), it->second.end(), shape) == it->second.end())
		it->second.push_back(shape);
}

void Document::block_remove(Shape* shape)
{
	auto it = _block_index.find(shape->block());
	if (it == _block_index.end())
		return;
	auto& shapes = it->second;
	shapes.erase(std::remove(shapes.begin(), shapes.end(), shape), shapes.end());
	if (shapes.empty())
	{
		_block_names.erase(std::remove(_block_names.begin(), _block_names.end(), it->first), _block_names.end());
		_block_index.erase(it);
	}
}

void Document::block_set(Shape* shape, const std::string& name)
{
	block_remove(shape);
	shape->block(name);
	block_add(shape);
}

std::string Document::unique_block(const std::string& name)
{
	auto names = blocks();
	std::string result = name;
	for (int i = 2; std::find(names.begin(), names.end(), result) != names.end(); i++)
		result = name + "_" + std::to_string(i);
	return result;
}

void Document::select(Graphic* g)
{
	auto add = [this](Graphic* x)
	{
		if (std::find(_selected.begin(), _selected.end(), x) == _selected.end())
		{
			x->selected(true);
			_selected.push_back(x);
		}
	};
	add(g);
	// a shape of a block selects the whole block
	if (g->shape() && !((Shape*)g)->block().empty())
		for (Shape* s : block_shapes(((Shape*)g)->block()))
			add(s);
}

void Document::select(std::vector<Graphic*> list)
{
	for (Graphic* g : list)
		select(g);
}

void Document::unselect(Graphic* g)
{
	auto remove = [this](Graphic* x)
	{
		auto it = std::find(_selected.begin(), _selected.end(), x);
		if (it != _selected.end())
		{
			(*it)->selected(false);
			_selected.erase(it);
		}
	};
	remove(g);
	if (g->shape() && !((Shape*)g)->block().empty())
		for (Shape* s : block_shapes(((Shape*)g)->block()))
			remove(s);
}

void Document::unselect(std::vector<Graphic*> list)
{
	for (Graphic* g : list)
		unselect(g);
}

void Document::reset()
{
	if (_layers.size() == 0)
	{
		layer();
	}
	else
	{
		for (Layer* layer : _layers)
		{
			layer->reset(_render);
		}
	}

	if (_groups.size() == 0)
	{
		auto group = new Group(_render);
		group->name("001");

		_groups.push_back(group);
	}
	else
	{
		for (Group* group : _groups)
		{
			group->reset(_render);
		}
	}
}

void Document::draw()
{
	for (Layer* layer : _layers)
	{
		layer->draw();
	}

	// all the toolpaths can be hidden at once, each group keeps its own visibility
	if (_show_toolpaths)
	{
		for (Group* group : _groups)
		{
			group->draw();
		}
	}
}

void Document::anchors()
{
	if (_selected.size() == 1)
	{
		for (Layer* layer : _layers)
		{
			layer->draw_anchors();
		}
	}
}

void Document::clear()
{
	auto t = std::chrono::high_resolution_clock::now();

	_selected.clear();
	_block_index.clear();
	_block_names.clear();

	for (Layer* layer : _layers)
	{
		delete layer;
	}
	_layers.clear();

	for (Group* group : _groups)
	{
		delete group;
	}
	_groups.clear();

	_current_layer = nullptr;
	_current_group = nullptr;

	environment::next_id(1);

	Logger::log("Unloading file (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
}

void Document::init()
{
	if (_layers.size() == 0)
	{
		layer("");
	}
	if (_groups.size() == 0)
	{
		group("");
	}
}

std::string Document::write(float version)
{
	std::string buffer = "{OPENPOSTPRO}\n"; 

	buffer += "0;version;" + stringex::to_string(version) + '\n';

	buffer += "0;output;" + _output + '\n';

	if (_has_machine)
		buffer += "0;machine;" + _machine.write() + '\n';

	buffer += "0;unit;" + std::to_string((int)_unit) + '\n';

	buffer += "0;camera;" + std::to_string(_render->camera()->scale()) + ";" + 
		std::to_string(_render->camera()->znear()) + ";" +
		std::to_string(_render->camera()->zfar()) + ";" +
		std::to_string(_render->camera()->xcenter()) + ";" +
		std::to_string(_render->camera()->ycenter());

	// the zoom steps and the point at the middle of the screen: the view does not depend on the size of the screen
	{
		Camera* c = _render->camera();
		glm::vec2 middle = c->getPosition(c->width() / 2.0f, c->height() / 2.0f);
		buffer += ";" + std::to_string(c->zoom_step()) + ";" + stringex::to_string(middle.x) + ";" + stringex::to_string(middle.y);
	}
	buffer += '\n';

	buffer += "{LAYERS}\n";

	for (Layer* layer : _layers)
	{
		buffer += layer->write() + "\n";
	}

	buffer += "{GROUPS}\n";

	for (Group* group : _groups)
	{
		buffer += group->write() + "\n";
	}

	buffer += "{SHAPES}\n";

	for (Layer* layer : _layers)
	{
		for (Shape* shape : layer->shapes())
		{
			// the shape is in this layer, whatever its parent says
			shape->parent(layer->name());
			buffer += shape->write() + "\n";
		}
	}

	buffer += "{TOOLPATH}\n";

	for (Group* group : _groups)
	{
		for (Toolpath* toolpath : group->toolpaths())
			buffer += toolpath->write() + "\n";
	}

	return buffer;
}

void Document::read(std::string value)
{
	clear();

	// documents saved before the unit existed are in millimeters
	_unit = Unit::Millimeter;
	_has_view = false;
	_has_machine = false;

	auto data = stringex::split(value, '\n');
	float version = 0.0f;
	std::vector<std::shared_future<Graphic*>> futures;
	unsigned int id = 1;

	std::vector<int> refs;

	// a shape which has no layer (a file written by an older version) goes to the first one: asking for a layer without name would
	// create a new layer for each shape, and the shapes of a contour would not be seen together any more
	auto shape_layer = [this](Shape* shape) -> Layer*
	{
		// a shape which has no layer goes to the first one
		if (shape->parent().empty() && _layers.size() > 0)
			return _layers[0];
		return layer(shape->parent());
	};

	// the pockets are computed together when all the document is read (see below), not one by one while it is read
	struct DeferPockets
	{
		DeferPockets() { Pocket::defer_read = true; }
		~DeferPockets() { Pocket::defer_read = false; }
	} defer_pockets;

	for (std::string s : data)
	{
		if (s.size() > 0 && s[0] != '{')
		{
			Graphic* g = nullptr;
			GraphicType code = (GraphicType)std::stoi(stringex::left(s, ';'));
			std::vector<std::string> values;
			switch (code)
			{
			// global document data
			case GraphicType::None:
				values = stringex::split(s, ';');
				if (values[1] == "version")
					version = std::stof(values[2]);
				// split() drops empty fields: a document without output file ("0;output;") has no third value
				if (values[1] == "output")
					_output = values.size() > 2 ? values[2] : std::string();
				if (values[1] == "machine" && values.size() > 2)
				{
					// the fields of the machine are the ones of its line of the table, after "0;machine;"
					std::string line;
					for (size_t i = 2; i < values.size(); i++)
						line += (i > 2 ? ";" : "") + values[i];
					_machine = Machine::read(line);
					_has_machine = true;
				}
				if (values[1] == "unit")
					_unit = (Unit)std::stoi(values[2]);
				if (values[1] == "camera")
				{
					_render->camera()->scale(std::stof(values[2]));
					_render->camera()->znear(std::stof(values[3]));
					_render->camera()->zfar(std::stof(values[4]));
					_render->camera()->xcenter(std::stof(values[5]));
					_render->camera()->ycenter(std::stof(values[6]));
					_scale = std::stof(values[2]);
					// the files written before the view was saved have no more than the values above
					if (values.size() > 9)
					{
						_has_view = true;
						_view_zoom = std::stoi(values[7]);
						_view_middle = glm::vec2(std::stof(values[8]), std::stof(values[9]));
					}
				}
				break;
			case GraphicType::Layer:
				g = layer("");
				g->read(s, version);
				break;
			case GraphicType::Group:
				g= group("");
				g->read(s, version);
				break;
			case GraphicType::Arc:
				g = new Arc(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Circle:
				g = new Circle(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Ellipse:
				g = new Ellipse(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Line:
				g = new Line(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Point:
				g = new Point(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Polyline:
				g = new Polyline(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::Text:
				g = new Text(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				if (((Text*)g)->reference() != -1)
					refs.push_back(((Text*)g)->reference());
				break;
			case GraphicType::Spline:
				g = new Spline(_render);
				g->read(s, version);
				shape_layer((Shape*)g)->add((Shape*)g);
				break;
			case GraphicType::CamMoveTo:
				g = new MoveTo(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			case GraphicType::CamFollow:
				g = new Follow(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			case GraphicType::CamOffset:
				g = new Offset(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			case GraphicType::CamPocket:
				g = new Pocket(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			case GraphicType::CamSpiral:
				g = new Spiral(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			case GraphicType::CamDrill:
				g = new Drill(_render);
				g->read(s, version);
				group(((Toolpath*)g)->parent())->add(((Toolpath*)g));
				break;
			}
			if (g != nullptr)
				id = std::max(id, g->id());
		}
	}

	// the pockets (the long part of the opening of a document) are calculated on all the cores, then drawn from this thread
	{
		std::vector<Pocket*> pockets;
		for (Group* g : _groups)
			for (Toolpath* t : g->toolpaths())
				if (t->type() == GraphicType::CamPocket && !((Pocket*)t)->calculated())
					pockets.push_back((Pocket*)t);
		parallel_for(pockets.size(), [&](size_t i) { pockets[i]->calculate(); });
		for (Pocket* p : pockets)
			p->generate();
	}

	// multitask coordinates computing
	for (Layer* l : _layers)
		for (Graphic* s : l->shapes())
			futures.push_back(std::async(std::launch::async, [s]() { s->compute(); return s; }));
	for (int i = 0; i < futures.size(); i++)
		futures[i].wait();

	//for (Group* g : _groups)
	//	for (Graphic* m : g->machines())
	//		m->update();

	if (_layers.size() > 0 )
		_current_layer = _layers[0];
	if (_groups.size() > 0)
		_current_group = _groups[0];

	environment::next_id(id + 1);

	check();
}
