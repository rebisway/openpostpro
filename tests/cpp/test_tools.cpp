// TOOLS TEST - the table of tools, the tool of a group, and the oblique plunge of the post-processor.
//
// What for:
//   A tool gives its values to a group (diameter, feeds, pass, spindle, cooling) and tells if it is a torch. The table is a text file of
//   the user; the group is saved in the document, and the documents which were saved before these fields exist must still open. The
//   oblique plunge replaces the vertical plunge at the offset start of a toolpath: a mistake here puts the tool into the material
//   at the wrong place, or makes it plunge vertically when it should not.
//
// What is checked: the table (add, number already used, renumber, remove, text and file round trip, a damaged line), the values taken by a
//   group from a tool and the torch, the group saved and read again, an old group (without the new fields), the moves of the plunge
//   (vertical, oblique on a line entry and on an arc entry, the next passes, no room, no segment) through the same code as the
//   post-processor (cut_path.h), with a recording object in place of the Python post-processor.
//
//   The table of the feeds (feeds.h): speed from the cutting speed, feed from the teeth, the limit of the spindle, the interpolation of the
//   feed per tooth, the values given to a tool, and the number of teeth saved in the table of the tools (an old line has two).
//
// Build: tests/cpp/build_tools.bat   Run: tests/cpp/test_tools.exe   (exit code 0 = all checks pass)
// The data folder is a temporary one (OPP_PROFILE): the tools of the user are not touched.
#include <follow.h>
#include <group.h>
#include <tool.h>
#include <feeds.h>
#include <cut_path.h>
#include <strings.h>
#include <environment.h>
#include <filesystem>
#include <cstdlib>
#include <cmath>
#include <cstdio>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

// Records the calls of the plunge and of the path, in place of the Python post-processor
struct Call
{
	char kind;			// R rapid, L linear, C circular, F feed, S start of the path
	glm::vec3 to;
	glm::vec2 center;
	bool cw;
	float feed;
};
struct Recorder
{
	std::vector<Call> calls;
	std::string rapid(glm::vec3 v) { calls.push_back({ 'R', v, glm::vec2(), false, 0 }); return "R"; }
	std::string linear(glm::vec3 v, bool = false) { calls.push_back({ 'L', v, glm::vec2(), false, 0 }); return "L"; }
	std::string circular(glm::vec3 v, glm::vec2 c, bool cw) { calls.push_back({ 'C', v, c, cw, 0 }); return "C"; }
	std::string feed(float f) { calls.push_back({ 'F', glm::vec3(), glm::vec2(), false, f }); return "F"; }
	std::string start_single_path() { calls.push_back({ 'S', glm::vec3(), glm::vec2(), false, 0 }); return "S"; }
	int count(char kind) { int n = 0; for (auto& c : calls) n += c.kind == kind; return n; }
};

static Curve square()
{
	Curve c;
	c.add(glm::vec2(0, 0)); c.add(glm::vec2(20, 0)); c.add(glm::vec2(20, 20)); c.add(glm::vec2(0, 20)); c.add(glm::vec2(0, 0));
	return c;
}

static CutPathParams params(float z, float z_top, bool oblique)
{
	CutPathParams p;
	p.z = z; p.z_top = z_top; p.safe = 5; p.plunge_feed = 300; p.feed = 900; p.tab_z = 100; p.oblique = oblique;
	return p;
}

static bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }
static bool at(glm::vec3 p, glm::vec2 xy) { return glm::distance(glm::vec2(p), xy) < 0.001f; }

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	// the data folder of the test (before any use of environment::)
	std::filesystem::path profile = std::filesystem::temp_directory_path() / "opp_test_tools_profile";
	std::filesystem::remove_all(profile);
	std::filesystem::create_directories(profile);
	_putenv_s("OPP_PROFILE", profile.string().c_str());

	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	std::puts("table: add, numbers, remove");
	{
		ToolTable t;
		Tool a; a.number = 3; a.name = "Mill 6"; a.diameter = 6;
		Tool b; b.number = 3; b.name = "duplicate";
		t.add(a);
		int added = t.add(b).number;
		CHECK(t.tools().size() == 2, "two tools");
		CHECK(added != 3 && added >= 1, "a number which is used gives the next free one");
		CHECK(t.find(3) != nullptr && t.find(3)->name == "Mill 6", "the first tool keeps its number");
		CHECK(t.find(99) == nullptr, "unknown number");
		CHECK(!t.renumber(3, added), "renumber to a used number is refused");
		CHECK(t.renumber(3, 10) && t.find(10) != nullptr && t.find(3) == nullptr, "renumber");
		CHECK(t.tools().front().number < t.tools().back().number, "sorted by number");
		CHECK(t.remove(10) && t.tools().size() == 1, "remove");
		CHECK(!t.remove(10), "remove an unknown tool");
	}

	std::puts("table: text and file");
	{
		ToolTable t;
		Tool a; a.number = 1; a.name = "End mill; 6 mm"; a.diameter = 6; a.feed = 1200; a.plunge_feed = 400; a.pass = 1.5f; a.spindle_speed = 12000;
		a.spindle_cw = false; a.cooling = CoolingType::Mist;
		Tool p; p.number = 2; p.name = "Plasma 45A"; p.type = ToolType::Plasma; p.diameter = 1.5f; p.feed = 2500;
		t.add(a); t.add(p);

		ToolTable r;
		r.read(t.write());
		CHECK(r.tools().size() == 2, "two tools read");
		Tool* ra = r.find(1);
		CHECK(ra && ra->name == "End mill; 6 mm", "name with a separator");
		CHECK(ra && near(ra->diameter, 6) && near(ra->feed, 1200) && near(ra->plunge_feed, 400) && near(ra->pass, 1.5f), "values");
		CHECK(ra && near(ra->spindle_speed, 12000) && !ra->spindle_cw && ra->cooling == CoolingType::Mist, "spindle and cooling");
		Tool* rp = r.find(2);
		CHECK(rp && rp->type == ToolType::Plasma && rp->torch(), "the torch is a torch");
		CHECK(ra && !ra->torch(), "the mill is not");

		ToolTable d;
		d.read("5;short line\n\nnot a number;x\n7;ok;1;2;3\n5;duplicate\n");
		CHECK(d.tools().size() == 2, "a damaged line and a duplicate are ignored");
		CHECK(d.find(7) && d.find(7)->type == ToolType::Plasma && near(d.find(7)->diameter, 2), "missing fields keep their default");

		std::string path = (profile / "data" / "tools.txt").string();
		CHECK(t.save(path), "save");
		ToolTable f;
		CHECK(f.load(path) && f.tools().size() == 2, "load");
		CHECK(!f.load((profile / "none.txt").string()), "load of a missing file");
		CHECK(ToolTable::default_path().find("opp_test_tools_profile") != std::string::npos, "the default path is in the profile of the test");
	}

	std::puts("group: values of a tool");
	{
		Group g(&renderer);
		Tool mill; mill.number = 4; mill.diameter = 8; mill.feed = 1500; mill.plunge_feed = 350; mill.pass = 2; mill.spindle_speed = 9000;
		mill.spindle_cw = false; mill.cooling = CoolingType::Flood;
		g.apply_tool(mill);
		CHECK(g.tool_number() == 4 && near(g.tool_radius(), 4) && near(g.feed(), 1500) && near(g.plung_feed(), 350), "number, radius, feeds");
		CHECK(near(g.pass(), 2) && near(g.spindle_speed(), 9000) && !g.spindle_cw() && g.cooling() == CoolingType::Flood, "pass, spindle, cooling");
		CHECK(!g.is_torch(), "a mill");
		Tool torch; torch.number = 9; torch.type = ToolType::WaterJet; torch.diameter = 1;
		g.apply_tool(torch);
		CHECK(g.is_torch() && g.tool_type() == ToolType::WaterJet && near(g.tool_radius(), 0.5f), "a water jet is a torch");
	}

	std::puts("group: saved and read again, old document");
	{
		Group g(&renderer);
		Tool plasma; plasma.number = 2; plasma.type = ToolType::Plasma; plasma.diameter = 1.5f; plasma.feed = 2500;
		g.apply_tool(plasma);
		g.oblique(true);
		g.pass(3);
		g.spindle_speed(5000);
		g.cooling(CoolingType::Mist);
		std::string text = g.write();

		Group r(&renderer);
		r.read(text);
		CHECK(r.oblique() && r.is_torch() && r.tool_number() == 2, "oblique, torch and number");
		CHECK(near(r.pass(), 3) && near(r.spindle_speed(), 5000) && r.cooling() == CoolingType::Mist, "pass, spindle, cooling");
		CHECK(near(r.tool_radius(), 0.75f) && near(r.feed(), 2500), "radius and feed");

		// a document written before the new fields: the first 19 fields only
		auto fields = stringex::split(text, ';');
		std::string old;
		for (int i = 0; i < 19; i++)
			old += fields[i] + ";";
		Group o(&renderer);
		o.read(old);
		CHECK(!o.oblique() && !o.is_torch() && o.tool_type() == ToolType::Mill, "old group: no oblique plunge, a mill");
		CHECK(near(o.pass(), 1) && near(o.spindle_speed(), 0) && o.spindle_cw() && o.cooling() == CoolingType::None, "old group: default pass, spindle, cooling");
		CHECK(o.tool_number() == 2 && near(o.tool_radius(), 0.75f), "old group: the old fields are read");
	}

	std::puts("feeds: the table");
	{
		const FeedMaterial* aluminium = nullptr;
		for (const FeedMaterial& m : feeds::materials())
			if (std::string(m.key) == "ALUMINIUM")
				aluminium = &m;
		CHECK(aluminium != nullptr, "aluminium is in the table");
		if (aluminium)
		{
			// 6 mm, 2 teeth: n = 250 * 1000 / (pi * 6) = 13263 rpm, fz = 0.05, F = 1326 mm/min
			feeds::Result r = feeds::compute(*aluminium, 6, 2);
			CHECK(near(r.spindle_speed, 13263) && near(r.feed, 1326) && !r.limited, "aluminium 6 mm, 2 teeth");
			CHECK(near(r.pass, 3.0f) && near(r.plunge_feed, std::round(1326 * 0.4f)) && r.cooling == CoolingType::Mist, "pass, plunge and cooling");
			// the spindle of the machine is slower: the feed follows the speed
			feeds::Result slow = feeds::compute(*aluminium, 6, 2, 10000);
			CHECK(slow.limited && near(slow.spindle_speed, 10000) && near(slow.feed, 1000), "limited by the maximum speed");
			// between two columns of the table: 9 mm is in the middle of 6 and 12
			CHECK(near(feeds::feed_per_tooth(*aluminium, 9), 0.065f), "fz is interpolated");
			CHECK(near(feeds::feed_per_tooth(*aluminium, 1), 0.02f) && near(feeds::feed_per_tooth(*aluminium, 50), 0.10f), "fz outside of the table");
			feeds::Result four = feeds::compute(*aluminium, 6, 4);
			CHECK(near(four.feed, std::round(13263 * 4 * 0.05f)), "the feed follows the number of teeth");
			CHECK(feeds::compute(*aluminium, 0, 2).feed == 0 && feeds::compute(*aluminium, 6, 0).feed == 0, "no diameter or no tooth: nothing");

			Tool t;
			feeds::apply(r, t);
			CHECK(near(t.feed, 1326) && near(t.spindle_speed, 13263) && near(t.pass, 3.0f), "applied to a tool");
		}
		// a harder material is slower than a softer one at the same diameter
		float previous = 1e9f;
		for (const char* key : { "ALUMINIUM", "STEEL", "STAINLESS" })
			for (const FeedMaterial& m : feeds::materials())
				if (std::string(m.key) == key)
				{
					float f = feeds::compute(m, 6, 2).feed;
					CHECK(f < previous, "the harder the material, the slower the feed");
					previous = f;
				}

		ToolTable table;
		Tool a; a.number = 1; a.flutes = 3;
		table.add(a);
		ToolTable back;
		back.read(table.write());
		CHECK(back.find(1) && back.find(1)->flutes == 3, "the number of teeth is saved");
		back.read("5;old tool;0;6;1000;500;1;0;1;0\n");
		CHECK(back.find(5) && back.find(5)->flutes == 2, "an old line has the default number of teeth");
	}

	std::puts("plunge: vertical");
	{
		Recorder post;
		Curve c = square();
		cut_path(post, c, params(-1, 0, false));
		CHECK(post.calls.size() > 4, "calls");
		CHECK(post.calls[0].kind == 'F' && near(post.calls[0].feed, 300), "plunge feed first");
		CHECK(post.calls[1].kind == 'L' && at(post.calls[1].to, glm::vec2(0, 0)) && near(post.calls[1].to.z, -1), "straight down at the first point");
		CHECK(post.calls[2].kind == 'F' && near(post.calls[2].feed, 900), "cutting feed after the plunge");
		CHECK(post.calls[3].kind == 'S', "start of the path");
		CHECK(post.count('L') == 5, "plunge + 4 sides");
	}

	std::puts("plunge: oblique along a line entry");
	{
		Follow f(&renderer);
		f.add(square());
		f.start_point_type(StartPointType::line);
		f.start_point_offset(5);
		f.update();
		auto curves = f.coordinates();
		CHECK(curves.size() == 1, "one curve");
		Curve c = curves[0];
		glm::vec2 s0 = c[0].point, s1 = c[1].point;
		CHECK(near(glm::distance(s0, s1), 5), "the first segment is the entry (5 long)");

		Recorder post;
		cut_path(post, c, params(-1, 0, true));
		// feed, rapid to the surface above the entry, entry going down, feed, start, then the contour
		CHECK(post.calls[0].kind == 'F' && near(post.calls[0].feed, 300), "plunge feed");
		CHECK(post.calls[1].kind == 'R' && near(post.calls[1].to.z, 0) && at(post.calls[1].to, s0), "down to the surface above the entry start");
		CHECK(post.calls[2].kind == 'L' && at(post.calls[2].to, s1) && near(post.calls[2].to.z, -1), "the entry goes down to the depth");
		CHECK(post.calls[3].kind == 'F' && near(post.calls[3].feed, 900), "cutting feed");
		CHECK(post.calls[4].kind == 'S', "start of the path");
		bool vertical = false;
		for (auto& m : post.calls)
			if (m.kind == 'L' && at(m.to, s0) && near(m.to.z, -1))
				vertical = true;
		CHECK(!vertical, "no vertical plunge at the start of the entry");
		// the entry is not done twice: one move for it, the others are the contour
		CHECK(post.count('L') == (int)c.size() - 1, "one move per segment, the entry included once");
		CHECK(near(post.calls.back().to.z, -1), "the contour is at the depth");

		// the same curve, vertical: the plunge is at the start of the entry
		Recorder vert;
		cut_path(vert, c, params(-1, 0, false));
		CHECK(vert.calls[1].kind == 'L' && near(vert.calls[1].to.z, -1) && at(vert.calls[1].to, s0), "not oblique: vertical at the start");
		CHECK(vert.count('R') == 0, "not oblique: no rapid move");
	}

	std::puts("plunge: oblique along an arc entry");
	{
		Follow f(&renderer);
		f.add(square());
		f.start_point_type(StartPointType::arc);
		f.start_point_offset(4);
		f.update();
		Curve c = f.coordinates()[0];
		CHECK(c[0].type == SegmentType::Arc, "the entry is an arc");
		Recorder post;
		cut_path(post, c, params(-1, 0, true));
		CHECK(post.calls[2].kind == 'C' && near(post.calls[2].to.z, -1) && glm::distance(post.calls[2].center, c[0].center) < 0.001f && post.calls[2].cw == c[0].cw, "helical move on the arc of the entry");
		CHECK(at(post.calls[2].to, c[1].point), "to the end of the arc");
	}

	std::puts("plunge: next pass, high safe, no room, no segment");
	{
		Follow f(&renderer);
		f.add(square());
		f.start_point_type(StartPointType::line);
		f.start_point_offset(5);
		f.update();
		Curve c = f.coordinates()[0];
		Recorder second;
		cut_path(second, c, params(-2, -1, true));
		CHECK(second.calls[1].kind == 'R' && near(second.calls[1].to.z, -1), "second pass: the entry starts at the depth of the first pass");
		CHECK(second.calls[2].kind == 'L' && near(second.calls[2].to.z, -2), "second pass: down to the new depth");

		// the surface is above the safe height: the tool starts from the safe height, no rapid move
		Recorder high;
		cut_path(high, c, params(-1, 20, true));
		CHECK(high.calls[1].kind == 'L' && near(high.calls[1].to.z, -1) && high.count('R') == 0, "no rapid above the safe height");

		// already at the depth: nothing to go down, a vertical plunge
		Recorder flat;
		cut_path(flat, c, params(-1, -1, true));
		CHECK(flat.count('R') == 0 && flat.calls[1].kind == 'L' && at(flat.calls[1].to, c[0].point), "no room to go down: vertical");

		// a curve of one point
		Curve one; one.add(glm::vec2(1, 1));
		Recorder tiny;
		cut_path(tiny, one, params(-1, 0, true));
		CHECK(tiny.calls[1].kind == 'L', "a curve without segment: vertical");
	}

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	std::filesystem::remove_all(profile);
	return g_fail == 0 ? 0 : 1;
}
