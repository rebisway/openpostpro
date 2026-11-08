// POST-PROCESSOR MACHINE TEST - the real post-processor (generic_mill.py, with the python of the system) on a machine with reversed axes.
//
// What for:
//   A machine whose X counts from the right to the left needs a program with the opposite X values, and arcs which turn the other way
//   (G2 and G3 swapped, I and J with the opposite sign). A mistake gives a part in mirror, or an arc which goes the long way round.
//   This test runs the whole post-processor, as the application does, and reads the G-code.
//
// What is checked: the same document is post-processed on a machine without reversed axis, then with X reversed, Y reversed, both
//   reversed and Z reversed. Line by line: the words X, Y, Z, I, J of the lines are the same or have the opposite sign, the arcs keep
//   or swap their direction (only one of X and Y reversed swaps it), the feeds and the other words are unchanged. A path out of the
//   area of the machine gives a warning in the file.
//
// Build: tests/cpp/build_postpro_machine.bat   Run: tests/cpp/test_postpro_machine.exe   (exit code 0 = all checks pass)
// The python of the system is used (python3.dll found by the PATH), the data folder is a temporary one (OPP_PROFILE).
#include <postpro.h>
#include <document.h>
#include <follow.h>
#include <script.h>
#include <environment.h>
#include <lang.h>
#include <file.h>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

// a line of the G-code: its words, letter then number (a comment has none)
struct Words
{
	std::string text;
	std::vector<std::pair<char, double>> words;
};

static std::vector<Words> parse(const std::string& gcode)
{
	std::vector<Words> lines;
	std::istringstream in(gcode);
	std::string line;
	while (std::getline(in, line))
	{
		Words w;
		w.text = line;
		if (line.empty() || line[0] == '(' || line[0] == '%')
		{
			lines.push_back(w);
			continue;
		}
		for (size_t i = 0; i < line.size();)
		{
			if (std::isalpha((unsigned char)line[i]))
			{
				size_t end = i + 1;
				while (end < line.size() && (std::isdigit((unsigned char)line[end]) || line[end] == '.' || line[end] == '-'))
					end++;
				try { w.words.push_back({ line[i], std::stod(line.substr(i + 1, end - i - 1)) }); } catch (const std::exception&) {}
				i = end;
			}
			else
				i++;
		}
		lines.push_back(w);
	}
	return lines;
}

static std::string run_document(Renderer* renderer, const std::filesystem::path& folder, const Machine* machine, bool circle, std::string* log_text = nullptr)
{
	Document doc;
	doc.render(renderer);
	doc.path((folder / "part.opp").string());
	if (machine)
		doc.machine(*machine);

	Group* g = doc.group("Group_1");
	g->depth(-2);
	g->pass(1);
	g->safe(5);
	g->feed(800);
	g->plung_feed(300);

	Curve c;
	if (circle)
		c.add(glm::vec2(30, 20), 15.0f, false);
	else
	{
		c.add(glm::vec2(10, 10)); c.add(glm::vec2(40, 10)); c.add(glm::vec2(40, 30)); c.add(glm::vec2(10, 30)); c.add(glm::vec2(10, 10));
	}
	Follow* f = new Follow(renderer);
	f->add(c);
	f->update();
	g->add(f);

	Postpro post;
	if (!post.initialize("generic_mill", "en-us"))
	{
		std::printf("  the post-processor does not start: %s\n", post.error().c_str());
		return "";
	}
	post.run(&doc, (folder / "out").string(), "test");
	std::string path = (folder / "out" / ("part." + post.extention())).string();
	return std::filesystem::exists(path) ? file::read_all_text(path) : "";
}

static bool near(double a, double b) { return std::fabs(a - b) < 0.0011; }

// compares the program of the reversed machine with the one of the plain machine
static void compare(const std::string& plain, const std::string& reversed, bool rx, bool ry, bool rz, const char* title)
{
	auto a = parse(plain), b = parse(reversed);
	CHECK(!a.empty() && a.size() == b.size(), title);
	if (a.size() != b.size())
	{
		std::printf("  %zu lines against %zu\n", a.size(), b.size());
		for (size_t i = 0; i < (std::min)(a.size(), b.size()); i++)
			if (a[i].text != b[i].text)
			{
				std::printf("  first difference, line %zu: [%s] and [%s]\n", i + 1, a[i].text.c_str(), b[i].text.c_str());
				break;
			}
		return;
	}
	int moves = 0, arcs = 0, bad = 0;
	for (size_t i = 0; i < a.size(); i++)
	{
		if (a[i].words.size() != b[i].words.size())
		{
			bad++;
			std::printf("  line %zu: [%s] and [%s] have not the same words\n", i + 1, a[i].text.c_str(), b[i].text.c_str());
			continue;
		}
		bool flipped = rx != ry;
		for (size_t k = 0; k < a[i].words.size(); k++)
		{
			char letter = a[i].words[k].first;
			double va = a[i].words[k].second, vb = b[i].words[k].second;
			bool ok = a[i].words[k].first == b[i].words[k].first;
			if (letter == 'X' || letter == 'I') { ok &= near(vb, rx ? -va : va); moves += letter == 'X'; arcs += letter == 'I'; }
			else if (letter == 'Y' || letter == 'J') ok &= near(vb, ry ? -va : va);
			else if (letter == 'Z') ok &= near(vb, rz ? -va : va);
			else if (letter == 'G' && (va == 2 || va == 3))
				ok &= near(vb, flipped ? 5 - va : va);
			else ok &= near(vb, va);
			if (!ok)
			{
				bad++;
				std::printf("  line %zu: [%s] became [%s]\n", i + 1, a[i].text.c_str(), b[i].text.c_str());
			}
		}
	}
	CHECK(bad == 0, title);
	CHECK(moves > 0, "the program has moves");
}

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	std::filesystem::path profile = std::filesystem::temp_directory_path() / "opp_test_postpro_machine";
	std::filesystem::remove_all(profile);
	std::filesystem::create_directories(profile);
	_putenv_s("OPP_PROFILE", profile.string().c_str());

	Lang::load("en-us");
	Script::initialize_python();
	Script::add_module_path(POSTPRO_DIR);

	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	for (int shape = 0; shape < 2; shape++)
	{
		const bool circle = shape == 1;
		std::printf(circle ? "a circle (arcs)\n" : "a square (lines)\n");

		Machine plain;
		plain.name = "plain";
		plain.width = 100; plain.depth = 100;
		std::string base = run_document(&renderer, profile / (circle ? "c0" : "s0"), &plain, circle);
		CHECK(base.find("G1") != std::string::npos || base.find("G01") != std::string::npos, "the program has cutting moves");
		if (circle)
			CHECK(base.find("G2") != std::string::npos || base.find("G3") != std::string::npos || base.find("G02") != std::string::npos || base.find("G03") != std::string::npos, "the program has arcs");

		// without machine: the same program as a plain machine
		std::string none = run_document(&renderer, profile / (circle ? "c1" : "s1"), nullptr, circle);
		compare(base, none, false, false, false, "no machine is a plain machine");

		struct Case { bool x, y, z; const char* name; };
		const Case cases[] = { { true, false, false, "X reversed" }, { false, true, false, "Y reversed" }, { true, true, false, "X and Y reversed" }, { false, false, true, "Z reversed" } };
		int index = 2;
		for (const Case& k : cases)
		{
			Machine m = plain;
			m.invert_x = k.x; m.invert_y = k.y; m.invert_z = k.z;
			std::string out = run_document(&renderer, profile / ((circle ? "c" : "s") + std::to_string(index++)), &m, circle);
			compare(base, out, k.x, k.y, k.z, k.name);
		}
	}

	std::puts("a path out of the area");
	{
		Machine small;
		small.name = "small";
		small.width = 20; small.depth = 20;
		std::string out = run_document(&renderer, profile / "out_of_area", &small, false);
		CHECK(out.find("WARNING") != std::string::npos, "a warning is written in the program");
		Machine big;
		big.name = "big";
		big.width = 200; big.depth = 200;
		std::string inside = run_document(&renderer, profile / "in_area", &big, false);
		CHECK(inside.find("WARNING") == std::string::npos, "no warning when the path is inside the area");
	}

	Script::finalize_python();
	// a document saved without output file has the line "0;output;" (split drops the empty field): it must be read, not crash
	std::puts("a document without output file");
	{
		Document doc;
		doc.render(&renderer);
		bool read = true;
		try
		{
			doc.read("{OPENPOSTPRO}\n0;version;0.1\n0;output;\n0;unit;0\n{LAYERS}\n1;1;Layer_1;(1:1:0:1);1;0\n{GROUPS}\n{SHAPES}\n{TOOLPATH}\n");
		}
		catch (const std::exception&)
		{
			read = false;
		}
		CHECK(read, "a document with an empty output line is read");
		CHECK(doc.output().empty(), "its output file is empty");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	if (argc < 2)
		std::filesystem::remove_all(profile);	// with an argument the programs are kept, to be read
	return g_fail == 0 ? 0 : 1;
}
