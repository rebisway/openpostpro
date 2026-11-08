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

#include "dxfexport.h"
#include "line.h"
#include "point.h"
#include "circle.h"
#include "arc.h"
#include "ellipse.h"
#include "polyline.h"
#include "spline.h"
#include "text.h"
#include <geometry.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <set>

namespace
{
	const char* EOL = "\r\n";
	const float EPS = 0.0001f;

	// a number with 6 decimals at most, always with a dot
	std::string num(double value)
	{
		char buffer[64];
		auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 6);
		std::string text(buffer, result.ptr);
		if (text.find('.') != std::string::npos)
		{
			while (text.back() == '0')
				text.pop_back();
			if (text.back() == '.')
				text.pop_back();
		}
		if (text == "-0" || text.empty())
			text = "0";
		return text;
	}

	// the text of a DXF text: the characters out of ASCII are written \U+XXXX, which every version reads
	std::string text_value(const std::string& utf8)
	{
		std::string out;
		for (size_t i = 0; i < utf8.size();)
		{
			unsigned char c = utf8[i];
			unsigned int code = c;
			int length = 1;
			if (c >= 0xF0 && i + 3 < utf8.size()) { code = ((c & 0x07) << 18) | ((utf8[i + 1] & 0x3F) << 12) | ((utf8[i + 2] & 0x3F) << 6) | (utf8[i + 3] & 0x3F); length = 4; }
			else if (c >= 0xE0 && i + 2 < utf8.size()) { code = ((c & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) | (utf8[i + 2] & 0x3F); length = 3; }
			else if (c >= 0xC0 && i + 1 < utf8.size()) { code = ((c & 0x1F) << 6) | (utf8[i + 1] & 0x3F); length = 2; }
			i += length;

			if (code < 32)
				out += ' ';		// a line break would cut the line of the file
			else if (code < 127)
				out += (char)code;
			else
			{
				char buffer[16];
				snprintf(buffer, sizeof(buffer), "\\U+%04X", code);
				out += buffer;
			}
		}
		return out;
	}

	struct Writer
	{
		std::string out;
		float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
		bool has_bounds = false;

		void code(int c, const std::string& value)
		{
			out += std::to_string(c);
			out += EOL;
			out += value;
			out += EOL;
		}
		void code(int c, double value) { code(c, num(value)); }
		void code(int c, int value) { code(c, std::to_string(value)); }

		void track(glm::vec2 p, float margin = 0)
		{
			if (!has_bounds)
			{
				min_x = p.x - margin; max_x = p.x + margin;
				min_y = p.y - margin; max_y = p.y + margin;
				has_bounds = true;
			}
			min_x = std::min(min_x, p.x - margin); max_x = std::max(max_x, p.x + margin);
			min_y = std::min(min_y, p.y - margin); max_y = std::max(max_y, p.y + margin);
		}

		void point(int first, glm::vec2 p)
		{
			code(first, p.x);
			code(first + 10, p.y);
			code(first + 20, 0.0);
		}

		void entity(const char* type, const std::string& layer)
		{
			code(0, type);
			code(8, layer);
		}

		void line(const std::string& layer, glm::vec2 p1, glm::vec2 p2)
		{
			entity("LINE", layer);
			point(10, p1);
			point(11, p2);
			track(p1);
			track(p2);
		}

		void circle(const std::string& layer, glm::vec2 center, float radius)
		{
			entity("CIRCLE", layer);
			point(10, center);
			code(40, radius);
			track(center, radius);
		}

		// the arc of DXF goes counter clockwise from its start angle to its end angle: a clockwise arc is written from its stop point
		void arc(const std::string& layer, glm::vec2 start, glm::vec2 center, glm::vec2 stop, bool cw)
		{
			if (cw)
				std::swap(start, stop);
			auto degrees = [&](glm::vec2 p)
			{
				double a = std::atan2(p.y - center.y, p.x - center.x) * 180.0 / glm::pi<double>();
				return a < 0 ? a + 360.0 : a;
			};
			entity("ARC", layer);
			point(10, center);
			code(40, glm::distance(start, center));
			code(50, degrees(start));
			code(51, degrees(stop));
			track(center, glm::distance(start, center));
		}

		void point_entity(const std::string& layer, glm::vec2 p)
		{
			entity("POINT", layer);
			point(10, p);
			track(p);
		}

		void text(const std::string& layer, glm::vec2 p, float height, const std::string& value)
		{
			entity("TEXT", layer);
			point(10, p);
			code(40, height);
			code(1, text_value(value));
			track(p);
			track(glm::vec2(p.x + height * (float)value.size() * 0.6f, p.y + height));
		}

		// a polyline: points with the bulge of the move which starts at each point (tangent of a quarter of the angle, positive counter clockwise)
		void polyline(const std::string& layer, const std::vector<glm::vec2>& points, const std::vector<float>& bulges, bool closed)
		{
			if (points.size() < 2)
				return;
			entity("POLYLINE", layer);
			code(66, 1);
			point(10, glm::vec2(0));
			code(70, closed ? 1 : 0);
			for (size_t i = 0; i < points.size(); i++)
			{
				entity("VERTEX", layer);
				point(10, points[i]);
				float bulge = i < bulges.size() ? bulges[i] : 0.0f;
				if (std::fabs(bulge) > 1e-9f)
					code(42, bulge);
				track(points[i]);
			}
			entity("SEQEND", layer);
		}

		// a polyline without bulges, the closing point is not repeated: the polyline is closed
		void polyline(const std::string& layer, std::vector<glm::vec2> points)
		{
			bool closed = false;
			if (points.size() > 2 && glm::distance(points.front(), points.back()) < EPS)
			{
				points.pop_back();
				closed = true;
			}
			polyline(layer, points, std::vector<float>(), closed);
		}
	};

	// bulge of the arc from p0 to p1 around the center
	float bulge(glm::vec2 p0, glm::vec2 p1, glm::vec2 center, bool cw)
	{
		double a0 = std::atan2(p0.y - center.y, p0.x - center.x);
		double a1 = std::atan2(p1.y - center.y, p1.x - center.x);
		double sweep = a1 - a0;
		const double two_pi = 2.0 * glm::pi<double>();
		if (cw)
		{
			while (sweep > 0) sweep -= two_pi;
			if (sweep > -1e-9) sweep = -two_pi;
		}
		else
		{
			while (sweep < 0) sweep += two_pi;
			if (sweep < 1e-9) sweep = two_pi;
		}
		return (float)std::tan(sweep / 4.0);
	}

	// the shapes made of lines and arcs: one polyline with bulges
	void spline(Writer& w, const std::string& layer, Spline* s)
	{
		Curve& curve = s->curve();
		if (curve.size() < 2)
			return;

		bool closed = curve.closed();
		std::vector<glm::vec2> points;
		std::vector<float> bulges;
		size_t count = curve.size() - 1;		// the last element is the end point of the last move
		for (size_t i = 0; i < count; i++)
		{
			Segment& seg = curve[i];
			glm::vec2 next = curve[i + 1].point;

			if (seg.type == SegmentType::Circle)
			{
				w.circle(layer, seg.center, seg.radius);
				continue;
			}
			float b = 0;
			if (seg.type == SegmentType::Arc)
			{
				b = bulge(seg.point, next, seg.center, seg.cw);
				// a full turn has no bulge: it is cut in two half turns
				if (std::fabs(b) > 1e6f || glm::distance(seg.point, next) < EPS)
				{
					glm::vec2 opposite = seg.center + (seg.center - seg.point);
					points.push_back(seg.point);
					bulges.push_back(seg.cw ? -1.0f : 1.0f);
					points.push_back(opposite);
					bulges.push_back(seg.cw ? -1.0f : 1.0f);
					continue;
				}
			}
			points.push_back(seg.point);
			bulges.push_back(b);
		}

		if (points.empty())
			return;
		if (!closed)
		{
			points.push_back(curve.back().point);
			bulges.push_back(0.0f);
		}
		// an arc to the center of the polyline can have been cut in two: the bounds follow the vertices and the arcs
		for (size_t i = 0; i < points.size(); i++)
		{
			if (std::fabs(bulges[i]) > 1e-9f && (i + 1 < points.size() || closed))
			{
				glm::vec2 a = points[i], b = points[(i + 1) % points.size()];
				float sagitta = glm::distance(a, b) * std::fabs(bulges[i]) / 2.0f;
				w.track(glm::vec2((a.x + b.x) / 2, (a.y + b.y) / 2), sagitta);
			}
		}
		w.polyline(layer, points, bulges, closed);
	}

	void shape(Writer& w, const std::string& layer, Shape* s)
	{
		switch (s->type())
		{
		case GraphicType::Line:
		{
			auto l = (Line*)s;
			w.line(layer, l->p1(), l->p2());
			break;
		}
		case GraphicType::Circle:
		{
			auto c = (Circle*)s;
			w.circle(layer, c->center(), c->radius());
			break;
		}
		case GraphicType::Arc:
		{
			auto a = (Arc*)s;
			w.arc(layer, a->start(), a->center(), a->stop(), a->cw());
			break;
		}
		case GraphicType::Point:
			w.point_entity(layer, ((Point*)s)->p1());
			break;
		case GraphicType::Ellipse:
			// no ellipse in R12: the points which are displayed
			w.polyline(layer, ((Ellipse*)s)->coordinates());
			break;
		case GraphicType::Polyline:
		{
			auto p = (Polyline*)s;
			w.polyline(layer, p->mode() == PolylineMode::Polyline ? p->points() : p->coordinates());
			break;
		}
		case GraphicType::Spline:
			spline(w, layer, (Spline*)s);
			break;
		case GraphicType::Text:
		{
			auto t = (Text*)s;
			if (t->reference() == -1)
				w.text(layer, t->p1(), t->height(), t->text());
			else
				for (auto& outline : t->coordinates())
					w.polyline(layer, outline);
			break;
		}
		default:
			break;
		}
	}
}

std::string DxfExport::layer_name(const std::string& name)
{
	std::string out;
	for (unsigned char c : name)
	{
		bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '-' || c == '_';
		out += ok ? (char)c : '_';
	}
	return out.empty() ? "0" : out;
}

int DxfExport::aci_color(glm::vec4 color)
{
	// the 9 first colors of AutoCAD
	static const float table[9][3] = {
		{ 255, 0, 0 }, { 255, 255, 0 }, { 0, 255, 0 }, { 0, 255, 255 }, { 0, 0, 255 }, { 255, 0, 255 }, { 255, 255, 255 }, { 128, 128, 128 }, { 192, 192, 192 } };
	float r = glm::clamp(color.r, 0.0f, 1.0f) * 255, g = glm::clamp(color.g, 0.0f, 1.0f) * 255, b = glm::clamp(color.b, 0.0f, 1.0f) * 255;
	int best = 7;
	float best_d = 1e30f;
	for (int i = 0; i < 9; i++)
	{
		float d = (r - table[i][0]) * (r - table[i][0]) + (g - table[i][1]) * (g - table[i][1]) + (b - table[i][2]) * (b - table[i][2]);
		if (d < best_d)
		{
			best_d = d;
			best = i + 1;
		}
	}
	return best;
}

std::string DxfExport::to_string(Document* document, bool selection_only)
{
	std::set<Graphic*> selected;
	if (selection_only)
		for (Graphic* g : document->selected())
			selected.insert(g);
	return to_string(document->layers(), document->unit(), selected);
}

std::string DxfExport::to_string(const std::vector<Layer*>& all_layers, Unit unit, const std::set<Graphic*>& selected)
{
	bool selection_only = !selected.empty();

	// the layers (the layer 0 always exists), and the entities
	Writer entities;
	std::vector<std::string> names;
	std::vector<Layer*> layers;
	names.push_back("0");
	layers.push_back(nullptr);
	for (Layer* layer : all_layers)
	{
		std::string name = layer_name(layer->name());
		if (std::find(names.begin(), names.end(), name) == names.end())
		{
			names.push_back(name);
			layers.push_back(layer);
		}
		for (Shape* s : layer->shapes())
			if (!selection_only || selected.count(s))
				shape(entities, name, s);
	}

	Writer w;
	w.code(0, "SECTION");
	w.code(2, "HEADER");
	w.code(9, "$ACADVER");
	w.code(1, "AC1009");
	if (entities.has_bounds)
	{
		w.code(9, "$EXTMIN");
		w.point(10, glm::vec2(entities.min_x, entities.min_y));
		w.code(9, "$EXTMAX");
		w.point(10, glm::vec2(entities.max_x, entities.max_y));
	}
	bool mm = unit == Unit::Millimeter;
	w.code(9, "$INSUNITS");
	w.code(70, mm ? 4 : 1);
	w.code(9, "$MEASUREMENT");
	w.code(70, mm ? 1 : 0);
	w.code(0, "ENDSEC");

	w.code(0, "SECTION");
	w.code(2, "TABLES");
	w.code(0, "TABLE");
	w.code(2, "LTYPE");
	w.code(70, 1);
	w.code(0, "LTYPE");
	w.code(2, "CONTINUOUS");
	w.code(70, 0);
	w.code(3, "Solid line");
	w.code(72, 65);
	w.code(73, 0);
	w.code(40, 0.0);
	w.code(0, "ENDTAB");

	w.code(0, "TABLE");
	w.code(2, "STYLE");
	w.code(70, 1);
	w.code(0, "STYLE");
	w.code(2, "STANDARD");
	w.code(70, 0);
	w.code(40, 0.0);
	w.code(41, 1.0);
	w.code(50, 0.0);
	w.code(71, 0);
	w.code(42, 0.2);
	w.code(3, "txt");
	w.code(4, "");
	w.code(0, "ENDTAB");

	w.code(0, "TABLE");
	w.code(2, "LAYER");
	w.code(70, (int)names.size());
	for (size_t i = 0; i < names.size(); i++)
	{
		Layer* layer = layers[i];
		w.code(0, "LAYER");
		w.code(2, names[i]);
		w.code(70, layer != nullptr && layer->locked() ? 4 : 0);
		int color = layer != nullptr ? aci_color(layer->color()) : 7;
		w.code(62, layer != nullptr && !layer->visible() ? -color : color);		// a negative color is a layer which is off
		w.code(6, "CONTINUOUS");
	}
	w.code(0, "ENDTAB");
	w.code(0, "ENDSEC");

	w.code(0, "SECTION");
	w.code(2, "BLOCKS");
	w.code(0, "ENDSEC");

	w.code(0, "SECTION");
	w.code(2, "ENTITIES");
	w.out += entities.out;
	w.code(0, "ENDSEC");
	w.code(0, "EOF");
	return w.out;
}

bool DxfExport::write(Document* document, const std::string& path, bool selection_only)
{
	std::ofstream file(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
	if (!file)
		return false;
	auto text = to_string(document, selection_only);
	file.write(text.data(), (std::streamsize)text.size());
	return (bool)file;
}
