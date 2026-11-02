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

#include "dxfloader.h"
#include "dxf.h"
#include "line.h"
#include "point.h"
#include "circle.h"
#include "arc.h"
#include "polyline.h"
#include <geometry.h>
#include <glm/gtc/constants.hpp>
#include "window.h"
#include <logger.h>

int DxfLoader::_count = 0;

void DxfLoader::read(std::string path, Document* document)
{
	Dxf dxf;

	dxf.read(path);

	document->clear();
	
	std::map<std::string, DxfLayer*> layers;
	for(DxfTable* t : dxf.tables()->items()) {
		for(DxfLayer* l : t->layers()) {
			layers[l->name()] = l;
		}
	}

	// read entities
	for(DxfSection* section : dxf.sections()) {
		if (section->type() == DxfSectionType::Entities) {
			DxfEntities* entities = (DxfEntities*)section;
			for(DxfEntity* e : entities->items()) {
				std::string layer_name = std::get<std::string>(e->properties("layer"));
				
				Layer* layer = document->layer(layer_name);
				auto dl = layers[layer_name];
				if (dl != nullptr)
				{
					layer->color(std::get<glm::vec4>(dl->properties("color")));
				}
				std::vector<Shape*> shapes = add_entity(e, document->render());
				for (Shape* s : shapes)
				{
					layer->shapes().push_back(s);
					s->parent(layer->name());
				}
			}
		}
	}
}


std::vector<Shape*> DxfLoader::add_entity(DxfEntity* e, Renderer* r)
{
	std::vector<Shape*> s;
	_count++;

	auto name = std::get<std::string>(e->properties("name"));
	auto handle = std::get<std::string>(e->properties("handle"));
	
	if (name == "")
		name = e->type() + "_" + handle;
	
	if (name[name.size()-1] == '_')
		name += std::to_string(_count);
	
	if (e->type() == "LINE")
		s = add_line(e, r);
	else if (e->type() == "POINT")
		s = add_point(e, r);
	else if (e->type() == "CIRCLE")
		s = add_circle(e, r);
	else if (e->type() == "ARC")
		s = add_arc(e, r);
	else if (e->type() == "ELLIPSE")
		s = add_ellipse(e, r);
	else if (e->type() == "LWPOLYLINE" || e->type() == "POLYLINE")
		s = add_polyline(e, r, name);
	
	if (s.size() == 1)
		s[0]->name(name);


	return s;
}

std::vector<Shape*> DxfLoader::add_line(DxfEntity* e, Renderer* r)
{
	Line* l = new Line(r);
	auto p1 = glm::vec2(std::get<float>(e->properties("p1.x")), std::get<float>(e->properties("p1.y")));
	auto p2 = glm::vec2(std::get<float>(e->properties("p2.x")), std::get<float>(e->properties("p2.y")));
	l->set(p1, p2);
		
	return std::vector<Shape*>({ l });
}

std::vector<Shape*> DxfLoader::add_point(DxfEntity* e, Renderer* r)
{
	Point* p = new Point(r);
	auto p1 = glm::vec2(std::get<float>(e->properties("p1.x")), std::get<float>(e->properties("p1.y")));
	p->p1(p1);

	return std::vector<Shape*>({ p });
}

std::vector<Shape*> DxfLoader::add_circle(DxfEntity* e, Renderer* r)
{
	Circle* c = new Circle(r);
	auto center = glm::vec2(std::get<float>(e->properties("center.x")), std::get<float>(e->properties("center.y")));
	auto radius = std::get<float>(e->properties("radius"));
	c->set(center, radius);

	return std::vector<Shape*>({ c });
}

std::vector<Shape*> DxfLoader::add_arc(DxfEntity* e, Renderer* r)
{
	Arc* a = new Arc(r);
	// center
	auto center = glm::vec2(std::get<float>(e->properties("center.x")), std::get<float>(e->properties("center.y")));

	// p1
	auto radius = std::get<float>(e->properties("radius"));
	auto start = std::get<float>(e->properties("start"));
	glm::vec2 sv = geometry::position(glm::radians(start), radius) + center;

	// p2
	auto end = std::get<float>(e->properties("end"));
	glm::vec2 ev = geometry::position(glm::radians(end), radius) + center;

	a->set(sv, center, ev, false);

	return std::vector<Shape*>({ a });
}

std::vector<Shape*> DxfLoader::add_ellipse(DxfEntity* e, Renderer* r)
{
	return std::vector<Shape*>();
}



std::vector<Shape*> DxfLoader::add_polyline(DxfEntity* e, Renderer* r, std::string name)
{
	std::vector<Shape*> shapes;
	Shape* s;
	std::vector<glm::vec2> points;
	auto p1 = glm::vec2(std::get<float>(e->vertice(0)->properties("x")), std::get<float>(e->vertice(0)->properties("y")));
	int pcount = 1;
	bool add_last = true;
	float bulge = 0;
	points.push_back(p1);
	for (int i = 1; i < e->vertices().size(); i++) {
		auto p2 = glm::vec2(std::get<float>(e->vertice(i)->properties("x")), std::get<float>(e->vertice(i)->properties("y")));

		bulge = std::get<float>(e->vertices().at(i-1)->properties("bulge")); // attention, bulge to be taken from previous vertice
		if (bulge == 0) { // not an arc
			points.push_back(p2); // so polyline
			pcount++;
			add_last = true;
		}
		else {
			if (pcount > 1) { // if arc, check whether the polygon built so far has at least 2 points
				// if so, add the polygon
				if (points.size() > 2)
				{
					s = new Polyline(r);
					((Polyline*)s)->points(points);
				}
				else
				{
					s = new Line(r);
					((Line*)s)->point(points[0]);
					((Line*)s)->point(points[1]);
				}
				s->name(name + '_' + std::to_string(_count++));
				shapes.push_back(s);
				// create a new temporary polygon
			}
			// add the arc
			shapes.push_back(add_arc(p1, p2, bulge, e, r, name + '_' + std::to_string(_count++)));
			add_last = false;

			points.clear();
			points.push_back(p2);
			pcount = 1;
		}
		p1 = p2;
	}

	if ((std::get<int>(e->properties("flag")) & 1) == 1) { // closed polyline

		add_last = true;
		auto p2 = glm::vec2(std::get<float>(e->vertice(0)->properties("x")), std::get<float>(e->vertice(0)->properties("y")));

		bulge = std::get<float>(e->vertice((int)e->vertices().size() - 1)->properties("bulge")); // attention, bulge to be taken from previous vertice
		if (bulge == 0) { // not an arc
			points.push_back(p2); // so polyline
			pcount++;
			add_last = true;
		}
		else {
			if (pcount > 1) { // if arc, check whether the polygon built so far has at least 2 points
				// if so, add the polygon
				if (points.size() > 2)
				{
					s = new Polyline(r);
					((Polyline*)s)->points(points);
				}
				else
				{
					s = new Line(r);
					((Line*)s)->point(points[0]);
					((Line*)s)->point(points[1]);
				}
				s->name(name + '_' + std::to_string(_count++));
				shapes.push_back(s);
				// create a new temporary polygon
			}
			// add the arc
			shapes.push_back(add_arc(p1, p2, bulge, e, r, name + '_' + std::to_string(_count++)));
			add_last = false;

			points.clear();
			points.push_back(p2);
			pcount = 1;
		}
	}

	if (add_last)
	{
		if (points.size() > 2)
		{
			s = new Polyline(r);
			((Polyline*)s)->points(points);
		}
		else
		{
			s = new Line(r);
			((Line*)s)->point(points[0]);
			((Line*)s)->point(points[1]);
		}
		s->name(name + '_' + std::to_string(_count++));
		shapes.push_back(s);
	}

	return shapes;
}


Shape* DxfLoader::add_arc(glm::vec2 p1, glm::vec2 p2, float bulge, DxfEntity* e, Renderer* r, std::string name)
{
	Arc* a = new Arc(r);

	float C;            // chord length
	float H;            // triangle height
	float R;            // rayon
	float alpha2;       // quarter of the arc angle
	float beta;         // segment orientation
	float i, j;

	// The bulge is the tangent of one fourth the
	// included angle for an arc segment, made negative if the arc goes
	// clockwise from the start ref_point to the endpoint.
	// A bulge of 0 indicates a straight segment,
	// and a bulge of 1 is a semicircle

	// we have the chord and the tangent of the chord angle (0=North)
	// We have the cord and the tangent of the arc radius
	// C=2R sin (Alpha/2)
	C = glm::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
	alpha2 = glm::atan(bulge) * 2;
	R = glm::abs(((C / (2 * glm::sin(alpha2)))));
	H = glm::sqrt(R * R - (C / 2) * (C / 2));

	if (bulge > 1 || (bulge < 0 && bulge > -1))
		alpha2 = alpha2 + glm::pi<float>();

	if (p1.x != p2.x) {
		beta = glm::atan((p2.y - p1.y) / (p2.x - p1.x));
		if (p2.x < p1.x)
			beta = beta + glm::pi<float>();
	}
	else {
		if (p2.y < p1.y)
			beta = -glm::half_pi<float>();
		else
			beta = glm::half_pi<float>();
	}

	if ((bulge > 1) || ((bulge < 0) && (bulge > -1))) {
		i = (p2.x - p1.x) / 2 + (glm::cos(beta - glm::half_pi<float>()) * H);
		j = (p2.y - p1.y) / 2 + (glm::sin(beta - glm::half_pi<float>()) * H);
	}
	else {
		i = (p2.x - p1.x) / 2 - (glm::cos(beta - glm::half_pi<float>()) * H);
		j = (p2.y - p1.y) / 2 - (glm::sin(beta - glm::half_pi<float>()) * H);
	}

	a->name(name);

	auto center =glm::vec2(p1.x + i, p1.y + j);
	a->set(p1, center, p2, bulge <= 0);

	return a;
}

/*
void add_entity(Entity e, Layer l) {
	bool adding = false;
	Shape s = null;
	count++;
	//			if ( handle == "58" )
	//				GG.Trace.Break();
	get_color(e);
	switch (e.Type) {
	case "LINE":
		try {
			s = new LineShape(name, manager);
			s.AddPoint(All.Double(e.Properties["p1.x"], 0), All.Double(e.Properties["p1.y"], 0));
			s.AddPoint(All.Double(e.Properties["p2.x"], 0), All.Double(e.Properties["p2.y"], 0));
			adding = lines;
		}
		catch (Exception l_ex) {
			GG.Trace.Line("LINE : " + l_ex.Message);
		}
		break;
	case "POINT":
		try {
			s = new PointShape(name, manager);
			s.AddPoint(All.Double(e.Properties["p1.x"], 0), All.Double(e.Properties["p1.y"], 0));
			adding = points;
		}
		catch (Exception l_ex) {
			GG.Trace.Line("POINT : " + l_ex.Message);
		}
		break;
	case "CIRCLE":
		try {
			adding = lines;
			CircleShape ci = (s = new CircleShape(name, manager)) as CircleShape;
			ci.AddPoint(All.Double(e.Properties["center.x"], 0), All.Double(e.Properties["center.y"], 0));
			ci.Radius = All.Double(e.Properties["radius"], 0);
		}
		catch (Exception l_ex) {
			GG.Trace.Line("CIRCLE : " + l_ex.Message);
		}
		break;
	case "ARC":
		try {
			adding = lines;
			ArcShape a = (s = new ArcShape(name, manager)) as ArcShape;
			// center
			Point3D center = new Point3D(All.Double(e.Properties["center.x"], 0), All.Double(e.Properties["center.y"], 0), 0);

			// p1
			double radius = All.Double(e.Properties.Get("radius"), 0);
			double start = All.Double(e.Properties.Get("start"), 0);
			Point3D sv = center.Position(-start, radius);

			// p2
			double end = All.Double(e.Properties.Get("end"), 0);

			a.AddPoint(sv);
			a.AddPoint(center);
			a.AddPoint(center.Position(-end, radius));

			// radius
			a.Radius = radius;

			// rotation
			a.CW = false;
		}
		catch (Exception l_ex) {
			GG.Trace.Line("ARC : " + l_ex.Message);
		}
		break;
	case "ELLIPSE":
		try {
			adding = lines;
			EllipseShape el = (s = new EllipseShape(name, manager)) as EllipseShape;
			Point3D el_center = new Point3D(All.Double(e.Properties["center.x"], 0), All.Double(e.Properties["center.y"], 0), 0);
			// center
			el.AddPoint(el_center);

			if ((string)e.Properties["handle"] == "11AF")
				GG.Trace.Break();

			Point3D el_major = new Point3D(All.Double(e.Properties["major.x"], 0) + el_center.X, All.Double(e.Properties["major.y"], 0) + el_center.Y, 0);
			double major = el_center.Length(el_major) * 2;
			double minor = All.Double(e.Properties["ratio"], 0) * major;

			// minor
			el.Minor = minor;

			// major
			el.Major = major;

			// angle
			el.Angle = 360 - el_center.Angle(el_major);
			//if ( el.Angle == 360 ) el.Angle = 0;

			el.Start = All.Double(e.Properties["start"], 0);

			el.End = All.Double(e.Properties["end"], 6.283185307179586);

		}
		catch (Exception l_ex) {
			GG.Trace.Line("ELLIPSE : " + l_ex.Message);
		}
		break;
	case "LWPOLYLINE":
		try {
			adding = lines;
			add_polyline(e, l);
		}
		catch (Exception l_ex) {
			GG.Trace.Line("LWPOLYLINE : " + l_ex.Message);
		}
		break;
	case "POLYLINE":
		try {
			adding = lines;
			add_polyline(e, l);
		}
		catch (Exception l_ex) {
			GG.Trace.Line("POLYLINE : " + l_ex.Message);
		}
		break;
	case "TEXT":
	case "ATTRIB":
		try {
			adding = true;
			TextShape tx = (s = new TextShape(name, manager)) as TextShape;
			text = analyseFont(All.String(e.Properties.Get("value")));

			// p1
			fontsize = (double)e.Properties.Get("height");

			tx.AddPoint((double)e.Properties["p1.x"], (double)e.Properties.Get("p1.y"), 0);

			// fontname
			if (e.Properties.ContainsKey("style"))
				fontname = (string)e.Properties["style"];
			tx.Family = fontname;

			// height
			tx.Height = fontsize * (double)e.Properties["scale"];

			// angle
			tx.Angle = (double)e.Properties["angle"];

			// text
			tx.Text = text;

			tx.UseDefaultFont = false;

			int ha = 0, va = 0;
			if (e.Properties.ContainsKey("horizontal.justify"))
				ha = (int)e.Properties["horizontal.justify"];
			if (e.Properties.ContainsKey("vertical.justify"))
				va = (int)e.Properties["vertical.justify"];

			switch (ha) {
			case 1: tx.HorizontalAlignment = System.Windows.HorizontalAlignment.Center; break;
			case 2: tx.HorizontalAlignment = System.Windows.HorizontalAlignment.Right; break;
			case 4:
				tx.HorizontalAlignment = System.Windows.HorizontalAlignment.Center;
				tx.VerticalAlignment = System.Windows.VerticalAlignment.Center;
				va = 2;
				break;
			}

			switch (va) {
			case 0: tx.VerticalAlignment = System.Windows.VerticalAlignment.Stretch; break;
			case 1: tx.VerticalAlignment = System.Windows.VerticalAlignment.Top; break;
			case 2: tx.VerticalAlignment = System.Windows.VerticalAlignment.Center; break;
			case 3: tx.VerticalAlignment = System.Windows.VerticalAlignment.Bottom; break;
			}

			if ((va > 0 || ha > 0) && e.Properties.ContainsKey("p2.x")) {
				tx.Points[0] = new Point3D((double)e.Properties["p2.x"], (double)e.Properties.Get("p2.y"), 0);
			}

			if (fcolor != -1)
				e.Properties["color"] = fcolor;
			get_color(e);
		}
		catch (Exception l_ex) {
			GG.Trace.Line("TEXT : " + l_ex.Message);
		}
		adding = texts;
		break;
	case "MTEXT":
		try {
			adding = true;
			TextShape tx2 = (s = new TextShape(name, manager)) as TextShape;

			text = analyseFont(All.String(e.Properties.Get("value")));

			fontsize = (double)e.Properties.Get("height");

			// p1
			tx2.AddPoint((double)e.Properties["first.x"], (double)e.Properties["first.y"], 0);

			// text
			tx2.Text = text;

			// fontname
			tx2.Family = fontname;

			// fontsize
			if (e.Properties.ContainsKey("height"))
				tx2.Height = fontsize * (double)e.Properties["scale"];

			// fontbold
			tx2.Bold = bold;

			// fontitalic
			tx2.Italic = italic;

			// angle
			tx2.Angle = (double)e.Properties["angle"];

			if (tx2.Angle == 0) {
				double x = 0, y = 0;
				if (e.Properties.ContainsKey("second.x"))
					x = (double)e.Properties["second.x"];
				if (e.Properties.ContainsKey("second.y"))
					y = (double)e.Properties["second.y"];
				if (x != 0 || y != 0) {
					Point3D vec_mtext = new Point3D(tx2.Points[0].X + x, tx2.Points[0].Y + y, 0);
					tx2.Angle = 360 - tx2.Points[0].Angle(vec_mtext);
				}
			}

			int ha = 0;
			if (e.Properties.ContainsKey("attachement"))
				ha = (int)e.Properties["attachement"];
			//						int va = 0;
			//						if ( e.Properties.ContainsKey("vertical.justify") )
			//							va = (int)e.Properties["vertical.justify"];

			switch (ha) {
			case 1:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Bottom;
				break;
			case 2:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Center;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Bottom;
				break;
			case 3:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Right;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Bottom;
				break;
			case 4:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Center;
				break;
			case 5:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Center;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Center;
				break;
			case 6:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Right;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Center;
				break;
			case 7:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Top;
				break;
			case 8:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Center;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Top;
				break;
			case 9:
				tx2.HorizontalAlignment = System.Windows.HorizontalAlignment.Right;
				tx2.VerticalAlignment = System.Windows.VerticalAlignment.Top;
				break;
			}

			//						switch( va ) {
			//							case 0: tx2.VerticalAlignment = System.Windows.VerticalAlignment.Stretch; break;
			//							case 1: tx2.VerticalAlignment = System.Windows.VerticalAlignment.Top; break;
			//							case 2: tx2.VerticalAlignment = System.Windows.VerticalAlignment.Center; break;
			//							case 3: tx2.VerticalAlignment = System.Windows.VerticalAlignment.Bottom; break;
			//						}

			//						if ( (va > 0 || ha > 0) && e.Properties.ContainsKey("p2.x") ) {
			//							tx2.Points[0] = new Point3D((double)e.Properties["p2.x"], (double)e.Properties.Get("p2.y"), 0);
			//						}

			tx2.UseDefaultFont = false;

			if (fcolor != -1)
				e.Properties["color"] = fcolor;
			get_color(e);
		}
		catch (Exception l_ex) {
			GG.Trace.Line("MTEXT : " + l_ex.Message);
		}
		adding = texts;
		break;
	case "INSERT":
		try {
			double i_x = (double)e.Properties["p1.x"];
			double i_y = (double)e.Properties["p1.y"];
			double i_z = (double)e.Properties["p1.z"];
			double s_x = (double)e.Properties["xscale"];
			double s_y = (double)e.Properties["yscale"];
			double s_z = (double)e.Properties["zscale"];
			double angle = (double)e.Properties["angle"];
			string blockname = (string)e.Properties["block"];
			if (blocks.Items.ContainsKey(blockname)) {
				foreach(Entity b_e in blocks.Items[blockname].Items) {
					Entity i_e = b_e.Clone();
					//if ( e.Properties.ContainsKey("handle") && i_e.Properties.ContainsKey("handle") )
					//	i_e.Properties["handle"] = e.Properties["handle"] + "_" + i_e.Properties["handle"];
					if (e.Properties.ContainsKey("layer"))
						i_e.Properties["layer"] = e.Properties["layer"];
					if (i_e.Type == "INSERT") {
						i_e.Properties["xscale"] = s_x;
						i_e.Properties["yscale"] = s_y;
						i_e.Properties["zscale"] = s_z;
						i_e.Properties["angle"] = angle;
					}

					Dictionary<string, Point3D> vectors = new Dictionary<string, Point3D>();
					foreach(string key in i_e.Properties.Keys) {
						if (!vectors.ContainsKey(key) && key.EndsWith(".x")) {
							if (!vectors.ContainsKey(All.Left(key, ".x")))
								vectors[All.Left(key, ".x")] = new Point3D((double)(i_e.Properties[key]), 0, 0);
							else {
								Point3D p = (Point3D)(vectors[All.Left(key, ".x")]);
								p.X = (double)(i_e.Properties[key]);
								(vectors[All.Left(key, ".x")]) = p;
							}
						}
						if (!vectors.ContainsKey(key) && key.EndsWith(".y")) {
							if (!vectors.ContainsKey(All.Left(key, ".y")))
								vectors[All.Left(key, ".y")] = new Point3D((double)(i_e.Properties[key]), 0, 0);
							else {
								Point3D p = (Point3D)(vectors[All.Left(key, ".y")]);
								p.Y = (double)(i_e.Properties[key]);
								(vectors[All.Left(key, ".y")]) = p;
							}
						}
					}

					//if ( i_e.Type == "TEXT" && i_e.Properties["value"].ToString().Contains("SRM") )
					//	GG.Trace.Line("");

					Matrix3D m = Matrix3D.Identity;
					m.Scale(new Vector3D(s_x, s_y, s_z));
					m.Rotate(new Quaternion(new Vector3D(0, 0, 1), angle));
					m.Translate(new Vector3D(i_x, i_y, i_z));
					//if ( i_e.Type != "TEXT" && i_e.Type != "MTEXT" ) {
					foreach(KeyValuePair<string, Point3D> pair in vectors) {
						Point3D p = pair.Value * m;

						i_e.Properties[pair.Key + ".x"] = (double)p.X;
						i_e.Properties[pair.Key + ".y"] = (double)p.Y;
					}
					//}

					if (i_e.Type == "TEXT" || i_e.Type == "MTEXT") {
						i_e.Properties["angle"] = (double)i_e.Properties["angle"] + angle;
						i_e.Properties["scale"] = System.Math.Abs(s_x);
						//i_e.Properties["p1.x"] = (double)i_e.Properties["p1.x"] + i_x;
						//i_e.Properties["p1.y"] = (double)i_e.Properties["p1.y"] + i_y;
						//i_e.Properties["p1.z"] = (double)i_e.Properties["p1.z"] + i_z;
					}
					if (i_e.Type == "CIRCLE") {
						i_e.Properties["radius"] = (double)i_e.Properties["radius"] * System.Math.Abs(s_x);
					}
					if (i_e.Type == "ARC") {
						i_e.Properties["radius"] = (double)i_e.Properties["radius"] * System.Math.Abs(s_x);
						i_e.Properties["start"] = (double)i_e.Properties["start"] - angle;
						i_e.Properties["end"] = (double)i_e.Properties["end"] - angle;
					}
					if (i_e.Properties.ContainsKey("layer"))
						add_entity(i_e, engine.Layers[(string)i_e.Properties["layer"]]);
					else
						add_entity(i_e, l);
				}
			}
		}
		catch (Exception l_ex) {
			GG.Trace.Line("INSERT : " + l_ex.Message);
		}
		break;
	case "SPLINE":
		try {
			adding = lines;
			SplineShape sp = new SplineShape(name, manager);
			s = sp;
			sp.SplineType = SplineType.Bezier;

			int flag = (int)e.Properties["flag"];

			if ((flag & 8) == 8)
				sp.SplineType = SplineType.BSpline;

			int spcount = (int)e.Properties.Get("count");

			for (int pi = 0; pi < spcount; pi++) {
				Point3D pp = new Point3D(All.Double(e.Properties["p" + pi.ToString() + ".x"], 0), All.Double(e.Properties.Get("p" + pi.ToString() + ".y")), All.Double(e.Properties.Get("p" + pi.ToString() + ".z")));
				s.AddPoint(pp);
			}
			if ((flag & 1) == 1) { // closed
				((SplineShape)s).IsClosed = true;
			}

		}
		catch (Exception l_ex) {
			GG.Trace.Line("SPLINE : " + l_ex.Message);
		}
		break;
	default:
		break;
	}

	if (s != null) {
		if (adding) {
			GG.Wait.DoEvents();
			add_shape(l, s);
		}
	}
}

private void add_polyline(Entity e, Layer l) {
	if (e.Vertexes.Count > 0) {
		string name = (string)e.Properties.Get("name");
		if (name == null)
			name = e.Type + "_" + (count++).ToString();
		Shape s = new PolygonShape(name, manager);

		int ecount = (int)e.Properties.Get("count");

		bool add_last = true;
		Point3D v1 = new Point3D((double)e.Vertexes[0].Properties["x"], (double)e.Vertexes[0].Properties["y"], 0);
		Point3D v2 = new Point3D();
		s.AddPoint(v1);
		int pcount = 1;
		double bulge = 0;
		for (int i = 1; i < e.Vertexes.Count; i++) {
			v2 = new Point3D((double)e.Vertexes[i].Properties["x"], (double)e.Vertexes[i].Properties["y"], 0);
			bulge = (double)e.Vertexes[i - 1].Properties["bulge"]; // warning: the bulge must be taken from the previous vertex
			if (bulge == 0) { // not an arc
				s.AddPoint(v2); // add the point to the polygon
				pcount++;
				add_last = true;
			}
			else {
				if (pcount > 1) { // if arc, check whether the polygon built so far has at least 2 points
					// if so, add the polygon
					add_shape(l, s);

					// create a new temporary polygon
					name = (string)e.Properties.Get("name");
					if (name == null)
						name = e.Type + "_" + (count++).ToString();
					s = new PolygonShape(name, manager);
					s.AddPoint(v2);
					pcount = 1;
				}
				else
					s.Points[0] = v2;
				// add the arc
				add_arc(l, v1, v2, bulge, e, (string)e.Vertexes[0].Properties["handle"]);
				add_last = false;
			}
			v1 = v2;

			if (i > 0 && i % 500 == 0)
				GG.WPF.Timer.DoEvents();
		}

		if (((int)e.Properties["flag"] & 1) == 1) { // closed polyline

			add_last = true;
			v2 = new Point3D((double)e.Vertexes[0].Properties["x"], (double)e.Vertexes[0].Properties["y"], 0);
			bulge = (double)e.Vertexes[e.Vertexes.Count - 1].Properties["bulge"];
			if (bulge == 0) {
				s.AddPoint(v2);
				pcount++;
				add_last = true;
			}
			else {
				if (pcount > 1) {
					add_shape(l, s);

					// create a new temporary polygon
					name = (string)e.Properties.Get("name");
					if (name == null)
						name = e.Type + "_" + (count++).ToString();
					s = new PolygonShape(name, manager);
					s.AddPoint(v2);
					pcount = 1;
				}
				else
					s.AddPoint(v2);
				add_arc(l, v1, v2, bulge, e, (string)e.Vertexes[0].Properties["handle"]);
				add_last = false;
			}
		}

		if (add_last)
			add_shape(l, s);
	}
}

private void add_lwpolyline(Entity e, Layer l) {
	if (e.Vertexes.Count > 0) {
		string name = (string)e.Properties.Get("name");
		if (name == null)
			name = e.Type + "_" + (count++).ToString();
		Shape s = new PolygonShape(name, manager);

		//					s = new PolygonShape(name, manager);
		//						
		//					int lpcount = (int)e.Properties.Get("count");
		//					
		//					double x = 0, y = 0;
		//					for(int pi = 0; pi < lpcount; pi++) {
		//						if ( e.Properties.ContainsKey("p" + pi.ToString() + ".x") )
		//							x = (double)e.Properties["p" + pi.ToString() + ".x"];
		//						if ( e.Properties.ContainsKey("p" + pi.ToString() + ".y") )
		//							y = (double)e.Properties["p" + pi.ToString() + ".y"];
		//
		//						s.AddPoint(new Point3D(x, y, 0));
		//						if ( pi % 500 == 0 )
		//							GG.WPF.Timer.DoEvents();
		//							
		//					}
		//					if ( ((int)e.Properties["flag"] & 1) == 1 ) { // closed
		//						((PolygonShape)s).IsClosed = true;
		//					}

		int ecount = (int)e.Properties.Get("count");

		bool add_last = true;
		Point3D v1 = new Point3D((double)e.Vertexes[0].Properties["x"], (double)e.Vertexes[0].Properties["y"], 0);
		Point3D v2 = new Point3D();
		s.AddPoint(v1);
		int pcount = 1;
		double bulge = 0;
		for (int i = 1; i < e.Vertexes.Count; i++) {
			v2 = new Point3D((double)e.Vertexes[i].Properties["x"], (double)e.Vertexes[i].Properties["y"], 0);
			bulge = (double)e.Vertexes[i - 1].Properties["bulge"]; // warning: the bulge must be taken from the previous vertex
			if (bulge == 0) { // not an arc
				s.AddPoint(v2); // add the point to the polygon
				pcount++;
				add_last = true;
			}
			else {
				if (pcount > 1) { // if arc, check whether the polygon built so far has at least 2 points
					// if so, add the polygon
					add_shape(l, s);

					// create a new temporary polygon
					name = (string)e.Properties.Get("name");
					if (name == null)
						name = e.Type + "_" + (count++).ToString();
					s = new PolygonShape(name, manager);
					s.AddPoint(v2);
					pcount = 1;
				}
				else
					s.Points[0] = v2;
				// add the arc
				add_arc(l, v1, v2, bulge, e, (string)e.Vertexes[0].Properties["handle"]);
				add_last = false;
			}
			v1 = v2;

			if (i > 0 && i % 500 == 0)
				GG.WPF.Timer.DoEvents();
		}

		if (((int)e.Properties["flag"] & 1) == 1) { // closed polyline

			add_last = true;
			v2 = new Point3D((double)e.Vertexes[0].Properties["x"], (double)e.Vertexes[0].Properties["y"], 0);
			bulge = (double)e.Vertexes[e.Vertexes.Count - 1].Properties["bulge"];
			if (bulge == 0) {
				s.AddPoint(v2);
				pcount++;
				add_last = true;
			}
			else {
				if (pcount > 1) {
					add_shape(l, s);

					// create a new temporary polygon
					name = (string)e.Properties.Get("name");
					if (name == null)
						name = e.Type + "_" + (count++).ToString();
					s = new PolygonShape(name, manager);
					s.AddPoint(v2);
					pcount = 1;
				}
				else
					s.AddPoint(v2);
				add_arc(l, v1, v2, bulge, e, (string)e.Vertexes[0].Properties["handle"]);
				add_last = false;
			}
		}

		if (add_last)
			add_shape(l, s);
	}
}

private void add_arc(Layer l, Point3D p1, Point3D p2, double bulge, Entity e, string handle) {
	double C;            // chord length
	double H;            // triangle height
	double R;            // rayon
	double alpha2;       // quarter of the arc angle
	double beta;         // segment orientation
	double i, j;

	// The bulge is the tangent of one fourth the
	// included angle for an arc segment, made negative if the arc goes
	// clockwise from the start ref_point to the endpoint.
	// A bulge of 0 indicates a straight segment,
	// and a bulge of 1 is a semicircle

	// we have the chord and the tangent of the chord angle (0=North)
	// We have the cord and the tangent of the arc radius
	// C=2R sin (Alpha/2)
	C = System.Math.Sqrt((p2.X - p1.X) * (p2.X - p1.X) + (p2.Y - p1.Y) * (p2.Y - p1.Y));
	alpha2 = System.Math.Atan(bulge) * 2;
	R = System.Math.Abs(((C / (2 * System.Math.Sin(alpha2)))));
	H = System.Math.Sqrt(R * R - (C / 2) * (C / 2));

	if (bulge > 1 || (bulge < 0 && bulge > -1))
		alpha2 = alpha2 + System.Math.PI;

	if (!p1.X.Equals(p2.X)) {
		beta = System.Math.Atan((p2.Y - p1.Y) / (p2.X - p1.X));
		if (p2.X < p1.X)
			beta = beta + System.Math.PI;
	}
	else {
		if (p2.Y < p1.Y)
			beta = -System.Math.PI / 2;
		else
			beta = System.Math.PI / 2;
	}

	if ((bulge > 1) || ((bulge < 0) && (bulge > -1))) {
		i = (p2.X - p1.X) / 2 + (System.Math.Cos(beta - System.Math.PI / 2) * H);
		j = (p2.Y - p1.Y) / 2 + (System.Math.Sin(beta - System.Math.PI / 2) * H);
	}
	else {
		i = (p2.X - p1.X) / 2 - (System.Math.Cos(beta - System.Math.PI / 2) * H);
		j = (p2.Y - p1.Y) / 2 - (System.Math.Sin(beta - System.Math.PI / 2) * H);
	}

	ArcShape s = new ArcShape("arc_" + (count++).ToString(), manager);

	Point3D center = new Point3D(p1.X + (double)i, p1.Y + (double)j, 0);
	s.AddPoint(p1);
	s.AddPoint(center);
	s.AddPoint(p2);

	// rotation
	s.CW = bulge <= 0;

	add_shape(l, s);
}
*/