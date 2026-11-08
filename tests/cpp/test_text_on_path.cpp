// TEXT ON A PATH TESTS - the letters of a text linked to a curve (cad/text.cpp, reference_path and reference_pos), without any window.
//
// What for:
//   A text can be linked to a line, an arc, a circle or any chain of shapes: the letters follow the curve. This test measures where the
//   letters really are (the bounds of each contour of the outlines) to see how the feature behaves in the usual cases and in the
//   limit cases: a text longer than the curve, a closed curve, the position along the curve.
//
// Run: test_text_on_path.exe   (it prints what it measures; the exit code is 0 when all the checks pass)
#include <text.h>
#include <renderer.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

struct Box
{
	glm::vec2 min, max;
	glm::vec2 center() const { return (min + max) * 0.5f; }
};

static std::vector<Box> boxes(Text& t)
{
	std::vector<Box> result;
	for (auto& contour : t.coordinates())
	{
		if (contour.empty())
			continue;
		Box b{ contour[0], contour[0] };
		for (auto& p : contour)
		{
			b.min = glm::min(b.min, p);
			b.max = glm::max(b.max, p);
		}
		result.push_back(b);
	}
	return result;
}

static Text* make(Renderer* r, const std::string& text, const std::vector<glm::vec2>& path, bool closed, float pos, float height = 10.0f)
{
	Text* t = new Text(r);
	t->set(text, "arial", false, false, height);
	t->point(glm::vec2(0, 0));
	t->reference(1);
	t->reference_path(path, closed);
	t->reference_pos(pos);
	t->compute();
	return t;
}

static std::vector<glm::vec2> line(float length)
{
	return { glm::vec2(0, 0), glm::vec2(length, 0) };
}

static std::vector<glm::vec2> circle(float radius, int points = 360)
{
	std::vector<glm::vec2> path;
	for (int i = 0; i <= points; i++)
		path.push_back(glm::vec2(radius * std::cos(glm::two_pi<float>() * i / points), radius * std::sin(glm::two_pi<float>() * i / points)));
	return path;
}

// width of the text without path
static float natural_width(Renderer* r, const std::string& text, float height)
{
	Text t(r);
	t.set(text, "arial", false, false, height);
	t.point(glm::vec2(0, 0));
	float right = 0;
	for (auto& b : boxes(t))
		right = std::max(right, b.max.x);
	return right;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	float width = natural_width(&renderer, "HELLO", 10.0f);
	std::printf("width of HELLO at height 10: %.2f\n", width);
	CHECK(width > 20 && width < 60, "the font is available");

	std::puts("a straight path, the text at the start");
	{
		Text* t = make(&renderer, "HELLO", line(200), false, 0);
		auto b = boxes(*t);
		CHECK(b.size() >= 5, "contours of the letters");
		float left = 1e9f, right = -1e9f, bottom = 1e9f, top = -1e9f;
		for (auto& x : b)
		{
			left = std::min(left, x.min.x); right = std::max(right, x.max.x);
			bottom = std::min(bottom, x.min.y); top = std::max(top, x.max.y);
		}
		std::printf("  extent x %.2f .. %.2f, y %.2f .. %.2f\n", left, right, bottom, top);
		CHECK(left > -1.0f && left < 2.0f, "the text starts at the start of the path");
		CHECK(std::fabs((right - left) - width) < 2.0f, "the text keeps its width on a straight path");
		CHECK(top - bottom < 12.0f, "the text keeps its height");
		delete t;
	}

	std::puts("the position along the path");
	{
		// the start of the text for a position of 0, 30 and 100
		for (float pos : { 0.0f, 20.0f, 30.0f, 100.0f })
		{
			Text* t = make(&renderer, "HELLO", line(200), false, pos);
			float left = 1e9f;
			for (auto& x : boxes(*t))
				left = std::min(left, x.min.x);
			std::printf("  position %.0f -> the text starts at x = %.2f (width of the text %.2f)\n", pos, left, width);
			delete t;
		}
		Text* t = make(&renderer, "HELLO", line(200), false, 100);
		float left = 1e9f;
		for (auto& x : boxes(*t))
			left = std::min(left, x.min.x);
		CHECK(std::fabs(left - 100.0f) < 3.0f, "a position of 100 puts the start of the text at 100 along the path");
		delete t;
	}

	std::puts("a text which is longer than the path");
	{
		Text* t = make(&renderer, "HELLO WORLD", line(30), false, 0);
		auto b = boxes(*t);
		float right = -1e9f;
		for (auto& x : b)
			right = std::max(right, x.max.x);
		// the letters which do not fit must go on in a straight line, not pile up on the last position: the outlines which are not
		// the inside of a letter (the hole of an O) must follow each other
		std::vector<Box> outer;
		for (auto& x : b)
		{
			bool inside = !outer.empty() && x.min.x >= outer.back().min.x && x.max.x <= outer.back().max.x && x.min.y >= outer.back().min.y && x.max.y <= outer.back().max.y;
			if (!inside)
				outer.push_back(x);
		}
		int piled = 0;
		for (size_t i = 0; i + 1 < outer.size(); i++)
			if (outer[i + 1].center().x - outer[i].center().x < 1.0f)
				piled++;
		float natural = natural_width(&renderer, "HELLO WORLD", 10.0f);
		std::printf("  path 30, text %.2f wide: right end at %.2f, %d letters not in order, %zu letters\n", natural, right, piled, outer.size());
		CHECK(outer.size() == 10, "ten letters");
		CHECK(piled == 0, "the letters follow each other");
		CHECK(std::fabs(right - natural) < 2.0f, "the text keeps its width beyond the end of the path");
		delete t;
	}

	std::puts("a circle");
	{
		float radius = 50.0f;
		auto path = circle(radius);
		float length = glm::two_pi<float>() * radius;
		Text* t = make(&renderer, "HELLO", path, true, 0);
		bool on_circle = true;
		for (auto& b : boxes(*t))
		{
			float d = glm::length(b.center());
			// the letters stand on the circle, inside or outside according to the direction of the circle
			if (d < radius - 12.0f || d > radius + 12.0f)
				on_circle = false;
		}
		CHECK(on_circle, "the letters stand on the circle");
		delete t;

		// a text which starts near the end of the circle must go on at the start: it is a closed curve
		Text* wrap = make(&renderer, "HELLO", path, true, length - 20.0f);
		int outside = 0;
		for (auto& b : boxes(*wrap))
		{
			float d = glm::length(b.center());
			if (d < radius - 12.0f || d > radius + 12.0f)
				outside++;
		}
		std::printf("  a start 20 before the end of the circle: %d letters out of the circle\n", outside);
		CHECK(outside == 0, "the letters go on at the start of a closed curve");
		delete wrap;
	}

	std::puts("the curve is moved: the application gives the new path and the same position (refresh)");
	{
		Text* t = make(&renderer, "HELLO", line(200), false, 10);
		// the curve moved by (30, 20): the path of the same length, the position is kept
		std::vector<glm::vec2> moved = { glm::vec2(30, 20), glm::vec2(230, 20) };
		float position = t->reference_pos();
		t->reference_path(moved, false);
		t->reference_pos(position);
		t->compute();
		float left = 1e9f, bottom = 1e9f;
		for (auto& x : boxes(*t))
		{
			left = std::min(left, x.min.x);
			bottom = std::min(bottom, x.min.y);
		}
		std::printf("  after the move the text starts at (%.2f, %.2f)\n", left, bottom);
		CHECK(std::fabs(left - 40.59f) < 1.0f && std::fabs(bottom - 20.0f) < 1.0f, "the text is on the moved curve, at the same position along it");
		delete t;
	}

	std::puts("the position of the files written before: it was the end of the text");
	{
		Text* t = make(&renderer, "HELLO", line(200), false, 100);
		// the file of an older version: the position is the end of the text, and there is no marker after the path
		std::string written = t->write();
		std::vector<std::string> fields;
		size_t from = 0;
		for (size_t at; (at = written.find(';', from)) != std::string::npos; from = at + 1)
			fields.push_back(written.substr(from, at - from));
		CHECK(fields.size() > 18 && fields.back() == "1", "a marker is written after the path");
		fields.pop_back();
		fields[12] = std::to_string(100.0f + width);
		std::string old;
		for (auto& f : fields)
			old += f + ";";
		Text* legacy = new Text(&renderer);
		legacy->read(old);
		float left = 1e9f;
		for (auto& x : boxes(*legacy))
			left = std::min(left, x.min.x);
		std::printf("  the old file puts the start of the text at x = %.2f\n", left);
		CHECK(std::fabs(left - 100.0f) < 3.0f, "an old position is the end of the text: the start is the same as before");
		// the new file keeps the position as it is
		Text* again = new Text(&renderer);
		again->read(t->write());
		left = 1e9f;
		for (auto& x : boxes(*again))
			left = std::min(left, x.min.x);
		CHECK(std::fabs(left - 100.0f) < 3.0f, "a file written now is read back with the same position");
		delete t;
		delete legacy;
		delete again;
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
