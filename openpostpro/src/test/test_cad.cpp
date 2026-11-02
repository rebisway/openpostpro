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

#include "test_cad.h"
#include <glm/gtc/constants.hpp>
#include <environment.h>
#include "line.h"
#include "polyline.h"
#include <circle.h>
#include <arc.h>
#include <point.h>
#include <ellipse.h>
#include <font.h>
#include <text.h>

void run_test(Document* document)
{
	//Line* line = new Line(document->render());
	//line->name("line 01");
	//line->set(glm::vec2(50.0f, 38.0f), glm::vec2(150.0f, 80.0f));
	//document->layers()[0]->shapes().push_back(line);


	//Polyline* poly = new Polyline(document->render());
	//poly->name("poly 01");
	//poly->points(std::vector<glm::vec2>{glm::vec2(-110.0f, 40.0f), glm::vec2(-140.0f, 40.0f), glm::vec2(-140.0f, 60.0f), glm::vec2(-110.0f, 60.0f)});
	//poly->mode(PolylineMode::Bezier);
	//document->layers()[0]->shapes().push_back(poly);

	//Circle* circle = new Circle(document->render());
	//circle->name("circle 01");
	//circle->radius(500);
	//circle->center(glm::vec2(-150.0f, -150.0f));
	//document->layers()[0]->shapes().push_back(circle);

	//Arc* arc = new Arc(document->render());
	//arc->name("arc 01");
	//float a1 = 45.0f;
	//float a2 = 315.0f;
	//float r = 45.0f;
	//auto ctr = glm::vec2(0.0f, 0.0f);
	//auto src = r * glm::vec2(glm::cos(glm::radians(a1)), glm::sin(glm::radians(a1))) + ctr;
	//auto dst = r * glm::vec2(glm::cos(glm::radians(a2)), glm::sin(glm::radians(a2))) + ctr;
	//bool cw = false;
	//arc->set(src, ctr, dst, cw);
	//document->layers()[0]->shapes().push_back(arc);

	//Ellipse* ellipse = new Ellipse(document->render());
	//ellipse->name("ellipse 01");
	//ellipse->set(glm::vec2(150.0f, 100.0f), 100.0f, 200.0f, 0.0f, glm::two_pi<float>(), 0.0f);
	//document->layers()[0]->shapes().push_back(ellipse);


	//Point* ref_point = new Point(document->render());
	//ref_point->name("ref_point 01");
	//ref_point->ref_point(glm::vec2(-50.0f, 50.0f));
	//document->layers()[0]->shapes().push_back(ref_point);

	//Font f(environment::combine_path(environment::application_path(), std::string("\\fonts\\consola.ttf")), FontStyle::Regular);
	//Font f("C:\\Windows\\Fonts\\Arial.ttf", FontStyle::Regular);

	/*auto vertices = f.get_text_coordinates("Thomas\ngourgnier", 0.3f, 500.0f);
	for (std::vector<glm::vec2> v : vertices)
	{
		Polyline* poly = new Polyline(document->render());
		poly->name("poly 02");
		poly->points(v);
		poly->mode(PolylineMode::Polyline);
		document->layers()[0]->shapes().push_back(poly);
	}
*/
	//auto fonts = environment::fonts();

	//Text* text = new Text(document->render());
	//text->name("text 01");
	//text->ref_point(glm::vec2(100.0f, 100.0f));
	//text->set("Thomas G", "verdana", false, true, 100.0f);
	//document->layers()[0]->shapes().push_back(text);
}