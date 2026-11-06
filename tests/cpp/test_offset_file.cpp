// OFFSET FILE TEST - recomputes the offset of every closed contour of a drawing file (.opp) and checks that the result stays near it.
//
// What for:
//   An offset of a closed contour by the radius of the tool must stay within that radius of the contour. A result with big arcs
//   or circles far from the contour is a bug of the offset algorithm (seen on a drawing imported from a DXF: dinausaur.opp).
//
// What is done: the lines and arcs of the file are chained end to end into contours (as the app does when a contour is selected),
//   the closed ones are offset in both directions and on both sides.
//   What is checked: every point of the offset curves is at most 1.5 * radius from the bounding box of the contour.
//
// Build: tests/cpp/build_offset_file.bat   Run: tests/cpp/test_offset_file.exe <file.opp> [radius]
#include <line.h>
#include <arc.h>
#include <curve.h>
#include <offset.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <limits>

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

static bool near(glm::vec2 a, glm::vec2 b) { return glm::distance(a, b) < 0.0005f; }

// the chain of shapes of the application (ModCad::look_for_siblings): the shapes are reversed in place.
// `tolerance` is the distance under which two ends are the same point (1e-6 in the application)
static std::vector<Shape*> look_for_siblings(Shape* s, const std::vector<Shape*>& everything, float tolerance)
{
	std::vector<Shape*> result;
	result.push_back(s);

	std::vector<Shape*> shapes;
	for (Shape* shape : everything)
		if (shape != s)
			shapes.push_back(shape);

	if (shapes.size() > 0)
	{
		glm::vec2 left = s->first(), right = s->last();

		std::vector<Shape*>::iterator it = shapes.begin();
		while (it != shapes.end())
		{
			if (geometry::distance((*it)->last(), right) < tolerance)
			{
				if ((*it)->type() == GraphicType::Line)
					((Line*)(*it))->p2(right);
				(*it)->reverse();
			}

			if (geometry::distance((*it)->first(), left) < tolerance)
			{
				if ((*it)->type() == GraphicType::Line)
					((Line*)(*it))->p1(left);
				(*it)->reverse();
			}

			if (geometry::distance((*it)->first(), right) < tolerance)
			{
				right = (*it)->last();
				result.push_back((*it));
				shapes.erase(it);
				it = shapes.begin();
			}
			else if (geometry::distance((*it)->last(), left) < tolerance)
			{
				left = (*it)->first();
				result.insert(result.begin(), (*it));
				shapes.erase(it);
				it = shapes.begin();
			}
			else
				it++;
		}
	}

	return result;
}

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	if (argc < 2)
	{
		std::puts("usage: test_offset_file <file.opp> [radius]");
		return 2;
	}
	float radius = argc > 2 ? (float)std::atof(argv[2]) : 0.8f;
	const bool trace = std::getenv("OFFSET_TRACE") != nullptr;
	const float tolerance = std::getenv("OFFSET_TOL") ? (float)std::atof(std::getenv("OFFSET_TOL")) : 0.01f;

	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::vector<Shape*> remaining;
	std::ifstream input(argv[1]);
	std::string text;
	bool shapes = false;
	while (std::getline(input, text))
	{
		if (text.size() && text[0] == '{')
		{
			shapes = text == "{SHAPES}";
			continue;
		}
		if (!shapes)
			continue;
		if (text.rfind("103;", 0) == 0)
		{
			Line* l = new Line(&renderer);
			l->read(text, 0.1f);
			remaining.push_back(l);
		}
		else if (text.rfind("102;", 0) == 0)
		{
			Arc* a = new Arc(&renderer);
			a->read(text, 0.1f);
			remaining.push_back(a);
		}
	}
	std::printf("%zu shapes read\n", remaining.size());
	const std::vector<Shape*> everything = remaining;

	int contours = 0, open = 0, run = 0, bad = 0;
	std::vector<Curve> all;
	while (!remaining.empty())
	{
		std::vector<Shape*> chain = look_for_siblings(remaining.front(), everything, tolerance);
		for (Shape* c : chain)
			remaining.erase(std::remove(remaining.begin(), remaining.end(), c), remaining.end());

		bool is_open = !near(chain.front()->first(), chain.back()->last());
		if (is_open)
			open++;
		if (is_open && std::getenv("OFFSET_SKIP_OPEN"))
		{
			open += 0;
			std::printf("  open chain of %zu shapes: from %g,%g to %g,%g (gap %g)\n", chain.size(), chain.front()->first().x, chain.front()->first().y,
				chain.back()->last().x, chain.back()->last().y, glm::distance(chain.front()->first(), chain.back()->last()));
			continue;
		}

		Curve c;
		for (Shape* s : chain)
		{
			if (s->type() == GraphicType::Line)
				c.add(((Line*)s)->p1());
			else
				c.add(((Arc*)s)->start(), ((Arc*)s)->center(), ((Arc*)s)->radius(), ((Arc*)s)->cw());
		}
		if (chain.size() > 1 && geometry::distance(chain.front()->first(), chain.back()->last()) < tolerance)
			c.close();
		else
			c.add(chain.back()->last());	// the end of an open chain, as cad::to_curve does
		contours++;

		std::vector<glm::vec2> outline = c.coordinates();
		all.push_back(c);

		for (int cw = 0; cw < 2; cw++)
			for (int side = -1; side <= 1; side += 2)
			{
				if (trace)
					std::printf("  contour #%d (%zu shapes) cw %d side %d\n", contours, chain.size(), cw, side);
				Curve work = c;
				work.cw(cw != 0);
				auto result = work.offset(radius * side);
				run++;
				float worst = 0;
				for (Curve& r : result)
					for (glm::vec2 v : r.coordinates())
					{
						// the distance to the contour itself
						float nearest = (std::numeric_limits<float>::max)();
						for (size_t i = 0; i + 1 < outline.size(); i++)
						{
							glm::vec2 d = outline[i + 1] - outline[i];
							float l2 = glm::dot(d, d);
							float t = l2 < 1e-12f ? 0.0f : glm::clamp(glm::dot(v - outline[i], d) / l2, 0.0f, 1.0f);
							nearest = (glm::min)(nearest, glm::distance(v, outline[i] + d * t));
						}
						worst = (glm::max)(worst, nearest);
					}
				if (worst > radius * 1.5f)
				{
					bad++;
					std::printf("  FAIL contour #%d (%zu shapes, first point %g,%g) cw %d side %d: %zu curves, %g from the contour (radius %g)\n",
						contours, chain.size(), chain[0]->first().x, chain[0]->first().y, cw, side, result.size(), worst, radius);
				}
			}
	}

	std::printf("%d closed contours, %d open chains, %d offsets computed, %d too far\n", contours, open, run, bad);

	// the path of the application: the contours go in a tree (the nesting gives the direction), and each one is an Offset toolpath
	std::puts("offset toolpaths from the tree");
	TreeCurve* tree = new TreeCurve(all);
	tree->sort();
	tree->cw_alter();
	auto curves = tree->curves();
	std::printf("  %zu curves given, %zu in the tree\n", all.size(), curves.size());
	int toolpaths = 0, wrong = 0;
	for (Curve& c : curves)
	{
		for (int interior = 0; interior < 2; interior++)
		{
			Offset* o = new Offset(&renderer);
			o->radius(radius);
			o->cw(c.level() % 2 == 0 ? true : false);
			o->interior(c.level() % 2 == 0 ? !interior : interior);
			o->add(c);
			o->compute();
			toolpaths++;

			std::vector<glm::vec2> outline = c.coordinates();
			float worst = 0;
			for (Curve& r : o->coordinates())
				for (glm::vec2 v : r.coordinates())
				{
					float nearest = (std::numeric_limits<float>::max)();
					for (size_t i = 0; i + 1 < outline.size(); i++)
					{
						glm::vec2 d = outline[i + 1] - outline[i];
						float l2 = glm::dot(d, d);
						float t = l2 < 1e-12f ? 0.0f : glm::clamp(glm::dot(v - outline[i], d) / l2, 0.0f, 1.0f);
						nearest = (glm::min)(nearest, glm::distance(v, outline[i] + d * t));
					}
					worst = (glm::max)(worst, nearest);
				}
			if (worst > radius * 1.5f)
			{
				wrong++;
				std::printf("  FAIL toolpath: contour level %d, %zu segments, first point %g,%g, interior %d: %g from the contour\n", c.level(), c.size(), c.front().point.x, c.front().point.y, interior, worst);
			}
			delete o;
		}
	}
	std::printf("%d toolpaths, %d wrong\n", toolpaths, wrong);
	bad += wrong;
	return bad == 0 ? 0 : 1;
}
