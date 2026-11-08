// DXF EXPORT TESTS - checks the text written by DxfExport on the real shape classes (export/dxfexport.cpp).
//
// What for:
//   The export must give a file that other CAD programs read: right entities, right coordinates, arcs in the right
//   direction (DXF arcs go counter clockwise only), bulges of the polylines with arcs, the layers and their colors.
//
// What is checked: the sections and the layer table; a line, a circle, a point; a counter clockwise and a clockwise
//   arc (the clockwise one is written from its stop point); a polyline (open, closed); a spline of lines and arcs as a
//   polyline with bulges (the sign follows the direction); an ellipse as a polyline; the layer names; the colors; only
//   the selected shapes; the unit in the header.
//
// Build: tests/cpp/build_dxf_export.bat   Run: tests/cpp/test_dxf_export.exe   (exit code 0 = all checks pass)
#include <dxfexport.h>
#include <layer.h>
#include <line.h>
#include <arc.h>
#include <circle.h>
#include <point.h>
#include <ellipse.h>
#include <polyline.h>
#include <spline.h>
#include <text.h>
#include <cmath>
#include <cstdio>
#include <sstream>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
#define CHECK_NEAR(a, b, tol, msg) do { g_run++; double _a = (a), _b = (b); if (std::fabs(_a - _b) > (tol)) { g_fail++; std::printf("  FAIL line %d: %s (got %g, expected %g)\n", __LINE__, msg, _a, _b); } } while (0)

// the outlines of the texts are not needed here: the font code is not linked
std::vector<std::vector<glm::vec2>> Text::coordinates() { return std::vector<std::vector<glm::vec2>>(); }

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

// one entity of the file: its type and its (code, value) pairs
struct Entity
{
	std::string type;
	std::vector<std::pair<int, std::string>> codes;
	std::string get(int code, int index = 0) const
	{
		for (auto& c : codes)
			if (c.first == code && index-- == 0)
				return c.second;
		return "";
	}
	double number(int code, int index = 0) const { return std::atof(get(code, index).c_str()); }
};

// the entities of a section (HEADER, TABLES, ENTITIES...). The first pairs of a section, before any entity, are in an entity without type.
static std::vector<Entity> section(const std::string& text, const std::string& name)
{
	std::vector<std::pair<int, std::string>> pairs;
	std::istringstream in(text);
	std::string a, b;
	while (std::getline(in, a) && std::getline(in, b))
	{
		if (!a.empty() && a.back() == '\r') a.pop_back();
		if (!b.empty() && b.back() == '\r') b.pop_back();
		pairs.push_back(std::make_pair(std::atoi(a.c_str()), b));
	}
	std::vector<Entity> result;
	bool inside = false;
	for (size_t i = 0; i < pairs.size(); i++)
	{
		if (pairs[i].first == 0 && pairs[i].second == "SECTION" && i + 1 < pairs.size() && pairs[i + 1].second == name)
		{
			inside = true;
			result.push_back(Entity());
			i++;
			continue;
		}
		if (pairs[i].first == 0 && pairs[i].second == "ENDSEC")
			inside = false;
		if (!inside)
			continue;
		if (pairs[i].first == 0)
		{
			Entity e;
			e.type = pairs[i].second;
			result.push_back(e);
		}
		else
			result.back().codes.push_back(pairs[i]);
	}
	return result;
}

static std::vector<Entity> of_type(const std::vector<Entity>& all, const std::string& type)
{
	std::vector<Entity> r;
	for (auto& e : all)
		if (e.type == type)
			r.push_back(e);
	return r;
}

// value which follows a header variable
static std::string header_value(const std::vector<Entity>& header, const std::string& variable)
{
	for (auto& h : header)
		for (size_t k = 0; k + 1 < h.codes.size(); k++)
			if (h.codes[k].second == variable)
				return h.codes[k + 1].second;
	return "";
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	Layer layer(&renderer);
	layer.name("Cut line");		// a space is not allowed in a DXF layer name
	layer.color(glm::vec4(1, 0, 0, 1));
	Layer second(&renderer);
	second.name("Holes");
	second.color(glm::vec4(0, 0, 1, 1));
	second.visible(false);
	second.locked(true);

	Line* line = new Line(&renderer);
	line->set(glm::vec2(1, 2), glm::vec2(10.5f, -2));
	layer.add(line);

	Circle* circle = new Circle(&renderer);
	circle->set(glm::vec2(5, 5), 2.5f);
	second.add(circle);

	Point* point = new Point(&renderer);
	point->p1(glm::vec2(3, 4));
	layer.add(point);

	// counter clockwise quarter of the circle of radius 10, from (10, 0) to (0, 10)
	Arc* ccw = new Arc(&renderer);
	ccw->set(glm::vec2(10, 0), glm::vec2(0, 0), glm::vec2(0, 10), false);
	layer.add(ccw);
	// clockwise quarter, from (0, 10) to (10, 0): the same arc as the first one
	Arc* cw = new Arc(&renderer);
	cw->set(glm::vec2(0, 10), glm::vec2(0, 0), glm::vec2(10, 0), true);
	layer.add(cw);

	Polyline* open = new Polyline(&renderer);
	open->points({ glm::vec2(0, 0), glm::vec2(5, 0), glm::vec2(5, 5) });
	layer.add(open);
	Polyline* closed = new Polyline(&renderer);
	closed->points({ glm::vec2(0, 0), glm::vec2(4, 0), glm::vec2(4, 4), glm::vec2(0, 0) });
	layer.add(closed);

	// a slot: two lines and two half circles, counter clockwise
	Spline* slot = new Spline(&renderer);
	slot->add(glm::vec2(0, 0));
	slot->add(glm::vec2(10, 0), glm::vec2(10, 5), 5, false, SegmentType::Arc);
	slot->add(glm::vec2(10, 10));
	slot->add(glm::vec2(0, 10), glm::vec2(0, 5), 5, false, SegmentType::Arc);
	slot->add(glm::vec2(0, 0));
	layer.add(slot);

	Ellipse* ellipse = new Ellipse(&renderer);
	ellipse->set(glm::vec2(0, 0), 2, 4, 0, glm::two_pi<float>(), 0);
	ellipse->compute();
	layer.add(ellipse);

	std::vector<Layer*> layers = { &layer, &second };
	auto text = DxfExport::to_string(layers, Unit::Millimeter);

	std::puts("header and tables");
	{
		auto header = section(text, "HEADER");
		CHECK(header_value(header, "$ACADVER") == "AC1009", "R12 version");
		CHECK(header_value(header, "$INSUNITS") == "4", "millimeters in the header");
		auto tables = section(text, "TABLES");
		auto layer_entries = of_type(tables, "LAYER");
		CHECK(layer_entries.size() == 3, "layers 0, Cut_line and Holes");
		if (layer_entries.size() == 3)
		{
			CHECK(layer_entries[1].get(2) == "Cut_line", "the space of the layer name is replaced");
			CHECK(layer_entries[1].number(62) == 1, "red layer: color 1");
			CHECK(layer_entries[2].number(62) == -5, "hidden blue layer: color -5");
			CHECK(layer_entries[2].number(70) == 4, "locked layer");
		}
		CHECK(text.find("EOF") != std::string::npos, "end of file");
	}

	auto entities = section(text, "ENTITIES");

	std::puts("line, circle, point");
	{
		auto lines = of_type(entities, "LINE");
		CHECK(lines.size() == 1, "one line");
		if (lines.size() == 1)
		{
			CHECK(lines[0].get(8) == "Cut_line", "layer of the line");
			CHECK_NEAR(lines[0].number(10), 1, 1e-6, "x1");
			CHECK_NEAR(lines[0].number(20), 2, 1e-6, "y1");
			CHECK_NEAR(lines[0].number(11), 10.5, 1e-6, "x2");
			CHECK_NEAR(lines[0].number(21), -2, 1e-6, "y2");
		}
		auto circles = of_type(entities, "CIRCLE");
		CHECK(circles.size() == 1 && circles[0].get(8) == "Holes", "circle on its layer");
		if (circles.size() == 1)
			CHECK_NEAR(circles[0].number(40), 2.5, 1e-6, "radius");
		CHECK(of_type(entities, "POINT").size() == 1, "one point");
	}

	std::puts("arcs go counter clockwise");
	{
		auto arcs = of_type(entities, "ARC");
		CHECK(arcs.size() == 2, "two arcs");
		for (auto& a : arcs)
		{
			CHECK_NEAR(a.number(40), 10, 1e-4, "radius");
			CHECK_NEAR(a.number(50), 0, 1e-3, "start angle 0");
			CHECK_NEAR(a.number(51), 90, 1e-3, "end angle 90");
		}
	}

	std::puts("polylines");
	{
		auto polys = of_type(entities, "POLYLINE");
		CHECK(polys.size() == 4, "open, closed, slot and ellipse");
		// the vertices of each polyline follow it
		std::vector<std::vector<Entity>> vertices;
		for (auto& e : entities)
		{
			if (e.type == "POLYLINE") vertices.emplace_back();
			else if (e.type == "VERTEX" && !vertices.empty()) vertices.back().push_back(e);
		}
		if (polys.size() == 4 && vertices.size() == 4)
		{
			CHECK(polys[0].number(70) == 0, "open polyline");
			CHECK(polys[1].number(70) == 1, "closed polyline");
			CHECK(polys[2].number(70) == 1, "the slot is closed");
			CHECK(vertices[0].size() == 3, "3 vertices in the open polyline");
			CHECK(vertices[1].size() == 3, "the closing point is not repeated");
			// the slot: two half circles of 180 degrees, bulge = tan(45 degrees) = 1 counter clockwise
			auto& s = vertices[2];
			CHECK(s.size() == 4, "4 vertices in the slot");
			if (s.size() == 4)
			{
				CHECK_NEAR(s[0].number(42), 0, 1e-9, "line: no bulge");
				CHECK_NEAR(s[1].number(42), 1, 1e-4, "half circle counter clockwise: bulge 1");
				CHECK_NEAR(s[3].number(42), 1, 1e-4, "second half circle: bulge 1");
			}
			CHECK(vertices[3].size() > 20, "ellipse: many vertices");
		}
	}

	std::puts("a clockwise slot has negative bulges, inches in the header");
	{
		Layer l2(&renderer);
		l2.name("A");
		Spline* s = new Spline(&renderer);
		s->add(glm::vec2(0, 0));
		s->add(glm::vec2(0, 10), glm::vec2(5, 10), 5, true, SegmentType::Arc);
		s->add(glm::vec2(10, 10));
		s->add(glm::vec2(10, 0), glm::vec2(5, 0), 5, true, SegmentType::Arc);
		s->add(glm::vec2(0, 0));
		l2.add(s);
		auto out = DxfExport::to_string(std::vector<Layer*>{ &l2 }, Unit::Inch);
		auto vertex = of_type(section(out, "ENTITIES"), "VERTEX");
		CHECK(vertex.size() == 4, "4 vertices");
		if (vertex.size() == 4)
			CHECK_NEAR(vertex[1].number(42), -1, 1e-4, "clockwise half circle: bulge -1");
		CHECK(header_value(section(out, "HEADER"), "$INSUNITS") == "1", "inches in the header");
	}

	std::puts("selection");
	{
		std::set<Graphic*> selected = { line };
		auto only = section(DxfExport::to_string(layers, Unit::Millimeter, selected), "ENTITIES");
		CHECK(of_type(only, "LINE").size() == 1 && only.size() == 2, "only the selected line");
	}

	std::puts("names and colors");
	{
		CHECK(DxfExport::layer_name("a/b c") == "a_b_c", "layer name without special characters");
		CHECK(DxfExport::layer_name("") == "0", "empty name is layer 0");
		CHECK(DxfExport::aci_color(glm::vec4(0.95f, 0.9f, 0.05f, 1)) == 2, "yellow");
		CHECK(DxfExport::aci_color(glm::vec4(0.5f, 0.5f, 0.5f, 1)) == 8, "grey");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
