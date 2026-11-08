// DXF IMPORT TESTS - checks the blocks, the splines and the texts read from a DXF file (import/dxf.cpp, import/dxfloader.cpp).
//
// What for:
//   The loader used to ignore the blocks (INSERT), the splines and the texts. A drawing made of blocks (a part placed
//   several times) came out empty, and a spline disappeared.
//
// What is checked: the blocks are read with their base point; an INSERT places the shapes of its block (position,
//   scale, rotation, layer 0 of the block takes the layer of the INSERT); a block inside a block; a missing block;
//   the B-spline (degree 1, a clamped cubic through its first and last control points, a rational one, fit points only);
//   the codes of the texts (%%d, %%c) and of the MTEXT (fonts, line breaks).
//
// Build: tests/cpp/build_dxf_import.bat   Run: tests/cpp/test_dxf_import.exe   (exit code 0 = all checks pass)
#include <dxf.h>
#include <dxfloader.h>
#include <line.h>
#include <circle.h>
#include <polyline.h>
#include <layer.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)
#define CHECK_NEAR(a, b, tol, msg) do { g_run++; double _a = (a), _b = (b); if (std::fabs(_a - _b) > (tol)) { g_fail++; std::printf("  FAIL line %d: %s (got %g, expected %g)\n", __LINE__, msg, _a, _b); } } while (0)

// the loader reads a Document: only the methods used by the functions tested here are needed, they are not called
void Document::clear() {}
Layer* Document::layer(std::string name) { return nullptr; }

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

static void code(std::ofstream& out, int c, const std::string& value) { out << c << "\n" << value << "\n"; }
static void code(std::ofstream& out, int c, double value) { out << c << "\n" << value << "\n"; }

static std::string write_file()
{
	std::string path = (std::filesystem::temp_directory_path() / "opp_test_import.dxf").string();
	std::ofstream out(path);
	code(out, 0, "SECTION"); code(out, 2, "HEADER"); code(out, 0, "ENDSEC");

	code(out, 0, "SECTION"); code(out, 2, "BLOCKS");
	// PLATE: a square of 10 with a hole, base point (5, 5), the entities are on the layer 0
	code(out, 0, "BLOCK"); code(out, 8, "0"); code(out, 2, "PLATE"); code(out, 70, 0); code(out, 10, 5.0); code(out, 20, 5.0); code(out, 30, 0.0);
	code(out, 0, "LINE"); code(out, 8, "0"); code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0); code(out, 11, 10.0); code(out, 21, 0.0); code(out, 31, 0.0);
	code(out, 0, "CIRCLE"); code(out, 8, "HOLES"); code(out, 10, 5.0); code(out, 20, 5.0); code(out, 30, 0.0); code(out, 40, 2.0);
	code(out, 0, "ENDBLK"); code(out, 8, "0");
	// PAIR: two PLATE, one of them moved by 100
	code(out, 0, "BLOCK"); code(out, 8, "0"); code(out, 2, "PAIR"); code(out, 70, 0); code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0);
	code(out, 0, "INSERT"); code(out, 8, "0"); code(out, 2, "PLATE"); code(out, 10, 5.0); code(out, 20, 5.0); code(out, 30, 0.0);
	code(out, 0, "INSERT"); code(out, 8, "0"); code(out, 2, "PLATE"); code(out, 10, 105.0); code(out, 20, 5.0); code(out, 30, 0.0);
	code(out, 0, "ENDBLK"); code(out, 8, "0");
	code(out, 0, "ENDSEC");

	code(out, 0, "SECTION"); code(out, 2, "ENTITIES");
	// a plate placed at (50, 60), scaled by 2 and turned by 90 degrees, on the layer PARTS
	code(out, 0, "INSERT"); code(out, 8, "PARTS"); code(out, 2, "PLATE"); code(out, 10, 50.0); code(out, 20, 60.0); code(out, 30, 0.0);
	code(out, 41, 2.0); code(out, 42, 2.0); code(out, 43, 1.0); code(out, 50, 90.0);
	code(out, 0, "INSERT"); code(out, 8, "PARTS"); code(out, 2, "PAIR"); code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0);
	code(out, 0, "INSERT"); code(out, 8, "PARTS"); code(out, 2, "NOPE"); code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0);
	// a degree 1 spline: its control points
	code(out, 0, "SPLINE"); code(out, 8, "S"); code(out, 70, 8); code(out, 71, 1); code(out, 72, 4); code(out, 73, 3); code(out, 74, 0);
	code(out, 40, 0.0); code(out, 40, 0.0); code(out, 40, 1.0); code(out, 40, 2.0);
	code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0); code(out, 10, 10.0); code(out, 20, 0.0); code(out, 30, 0.0); code(out, 10, 10.0); code(out, 20, 10.0); code(out, 30, 0.0);
	// a cubic with 4 control points, clamped knots
	code(out, 0, "SPLINE"); code(out, 8, "S"); code(out, 70, 8); code(out, 71, 3); code(out, 72, 8); code(out, 73, 4); code(out, 74, 0);
	for (double k : { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 }) code(out, 40, k);
	code(out, 10, 0.0); code(out, 20, 0.0); code(out, 30, 0.0); code(out, 10, 10.0); code(out, 20, 20.0); code(out, 30, 0.0);
	code(out, 10, 20.0); code(out, 20, 20.0); code(out, 30, 0.0); code(out, 10, 30.0); code(out, 20, 0.0); code(out, 30, 0.0);
	// a quarter of a circle: a rational quadratic with weights 1, sqrt(2)/2, 1
	code(out, 0, "SPLINE"); code(out, 8, "S"); code(out, 70, 8); code(out, 71, 2); code(out, 72, 6); code(out, 73, 3); code(out, 74, 0);
	for (double k : { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 }) code(out, 40, k);
	code(out, 10, 10.0); code(out, 20, 0.0); code(out, 30, 0.0); code(out, 10, 10.0); code(out, 20, 10.0); code(out, 30, 0.0); code(out, 10, 0.0); code(out, 20, 10.0); code(out, 30, 0.0);
	for (double w : { 1.0, 0.70710678, 1.0 }) code(out, 41, w);
	// fit points only
	code(out, 0, "SPLINE"); code(out, 8, "S"); code(out, 70, 0); code(out, 71, 3); code(out, 72, 0); code(out, 73, 0); code(out, 74, 3);
	code(out, 11, 0.0); code(out, 21, 0.0); code(out, 31, 0.0); code(out, 11, 5.0); code(out, 21, 5.0); code(out, 31, 0.0); code(out, 11, 10.0); code(out, 21, 0.0); code(out, 31, 0.0);
	code(out, 0, "ENDSEC");
	code(out, 0, "EOF");
	return path;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	TestRenderer renderer;
	Camera camera;
	renderer.camera(&camera);

	auto path = write_file();
	Dxf dxf;
	dxf.read(path);

	std::puts("blocks");
	{
		CHECK(dxf.blocks() != nullptr, "the BLOCKS section is read");
		auto plate = dxf.blocks() ? dxf.blocks()->block("PLATE") : nullptr;
		CHECK(plate != nullptr, "block PLATE");
		if (plate)
		{
			CHECK(plate->items().size() == 2, "two entities in PLATE");
			CHECK_NEAR(plate->x(), 5, 1e-6, "base point x");
			CHECK_NEAR(plate->y(), 5, 1e-6, "base point y");
		}
		CHECK(dxf.blocks() && dxf.blocks()->block("PAIR") != nullptr, "block PAIR");
		CHECK(dxf.blocks() && dxf.blocks()->block("NOPE") == nullptr, "no block NOPE");
		CHECK(dxf.entities() && dxf.entities()->items().size() == 7, "7 entities in the drawing");
	}

	std::puts("insert");
	{
		std::vector<DxfLoader::Placed> placed;
		auto items = dxf.entities()->items();
		// scaled by 2, turned by 90 degrees, base point (5, 5) to (50, 60): the point (0, 0) of the block goes to (50 + 10, 60 - 10)
		DxfLoader::add_insert(dxf, items[0], &renderer, glm::mat4(1.0f), "PARTS", 0, placed);
		CHECK(placed.size() == 2, "line and circle");
		if (placed.size() == 2)
		{
			CHECK(placed[0].layer == "PARTS", "the layer 0 of the block takes the layer of the insert");
			CHECK(placed[1].layer == "HOLES", "the other layers are kept");
			auto line = (Line*)placed[0].shape;
			// (0,0) -> (-5,-5) * 2 = (-10,-10) -> rotated by 90 degrees (10,-10) -> + (50, 60) = (60, 50)
			CHECK_NEAR(line->p1().x, 60, 1e-3, "x1");
			CHECK_NEAR(line->p1().y, 50, 1e-3, "y1");
			// (10,0) -> (5,-5) * 2 = (10,-10) -> rotated (10, 10) -> (60, 70)
			CHECK_NEAR(line->p2().x, 60, 1e-3, "x2");
			CHECK_NEAR(line->p2().y, 70, 1e-3, "y2");
			auto circle = (Circle*)placed[1].shape;
			CHECK_NEAR(circle->center().x, 50, 1e-3, "the center of the hole is the insertion point");
			CHECK_NEAR(circle->center().y, 60, 1e-3, "the center of the hole is the insertion point");
		}

		// a block in a block: two plates, the second one 100 further
		placed.clear();
		DxfLoader::add_insert(dxf, items[1], &renderer, glm::mat4(1.0f), "PARTS", 0, placed);
		CHECK(placed.size() == 4, "two plates of two shapes");
		if (placed.size() == 4)
		{
			CHECK_NEAR(((Line*)placed[0].shape)->p1().x, 0, 1e-3, "first plate at the origin");
			CHECK_NEAR(((Line*)placed[2].shape)->p1().x, 100, 1e-3, "second plate 100 further");
		}

		placed.clear();
		DxfLoader::add_insert(dxf, items[2], &renderer, glm::mat4(1.0f), "PARTS", 0, placed);
		CHECK(placed.empty(), "a block which does not exist gives nothing");
	}

	std::puts("splines");
	{
		auto items = dxf.entities()->items();
		auto degree1 = DxfLoader::add_spline(items[3], &renderer);
		CHECK(degree1.size() == 1, "a polyline for the degree 1 spline");
		if (degree1.size() == 1)
		{
			auto points = ((Polyline*)degree1[0])->points();
			CHECK(points.size() > 3, "points");
			CHECK_NEAR(glm::distance(points.front(), glm::vec2(0, 0)), 0, 1e-3, "starts on the first control point");
			CHECK_NEAR(glm::distance(points.back(), glm::vec2(10, 10)), 0, 1e-3, "ends on the last control point");
		}

		auto cubic = DxfLoader::add_spline(items[4], &renderer);
		CHECK(cubic.size() == 1, "a polyline for the cubic");
		if (cubic.size() == 1)
		{
			auto points = ((Polyline*)cubic[0])->points();
			CHECK_NEAR(glm::distance(points.front(), glm::vec2(0, 0)), 0, 1e-3, "starts on the first control point");
			CHECK_NEAR(glm::distance(points.back(), glm::vec2(30, 0)), 0, 1e-3, "ends on the last control point");
			// the middle of a Bezier cubic: (P0 + 3 P1 + 3 P2 + P3) / 8 = (7.5 * ... ) = (0 + 30 + 60 + 30) / 8 = 15 in x, (0 + 60 + 60 + 0) / 8 = 15 in y
			float best = 1e9f;
			for (auto& p : points)
				best = std::min(best, glm::distance(p, glm::vec2(15, 15)));
			CHECK(best < 0.3f, "the curve goes through the middle point of the Bezier");
		}

		auto circle = DxfLoader::add_spline(items[5], &renderer);
		CHECK(circle.size() == 1, "a polyline for the rational spline");
		if (circle.size() == 1)
		{
			auto points = ((Polyline*)circle[0])->points();
			bool on_circle = true;
			for (auto& p : points)
				on_circle = on_circle && std::fabs(glm::length(p) - 10.0f) < 0.01f;
			// a quarter of a circle of radius 10 around the origin, from (10, 0) to (0, 10)
			CHECK(on_circle, "the points of the rational spline are on the circle");
			CHECK_NEAR(glm::distance(points.back(), glm::vec2(0, 10)), 0, 1e-3, "ends at (0, 10)");
		}

		auto fit = DxfLoader::add_spline(items[6], &renderer);
		CHECK(fit.size() == 1 && ((Polyline*)fit[0])->mode() == PolylineMode::Catmull, "fit points only: a Catmull polyline");
	}

	std::puts("texts");
	{
		CHECK(DxfLoader::plain_text("45%%d") == "45\xC2\xB0", "degree");
		CHECK(DxfLoader::plain_text("%%c12 %%p0.1") == "\xC3\x98" "12 \xC2\xB1" "0.1", "diameter and plus or minus");
		CHECK(DxfLoader::plain_text("%%uunder%%u") == "under", "underline dropped");
		CHECK(DxfLoader::mtext_content("{\\fArial|b0|i0|c0|p34;Hello}\\Pworld") == "Hello world", "font and line break");
		CHECK(DxfLoader::mtext_content("\\H2.5x;big \\C1;red") == "big red", "height and color");
		CHECK(DxfLoader::mtext_content("a\\Sx^y;b") == "ax/yb", "stacking");
	}

	// the real files of the author, if they are there: nothing may throw, and the entities give shapes (the texts are left out: the fonts are not loaded here)
	std::puts("real files");
	{
		std::filesystem::path folder = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path() / "dxf";
		if (std::filesystem::exists(folder))
		{
			for (auto& entry : std::filesystem::directory_iterator(folder))
			{
				if (entry.path().extension() != ".dxf")
					continue;
				try
				{
					Dxf file;
					file.read(entry.path().string());
					int shapes = 0, inserts = 0, entities = 0;
					if (file.entities())
					{
						for (DxfEntity* e : file.entities()->items())
						{
							entities++;
							if (e->type() == "TEXT" || e->type() == "MTEXT" || e->type() == "ATTRIB")
								continue;
							if (e->type() == "INSERT")
							{
								std::vector<DxfLoader::Placed> placed;
								DxfLoader::add_insert(file, e, &renderer, glm::mat4(1.0f), "0", 0, placed);
								inserts++;
								shapes += (int)placed.size();
							}
							else
								shapes += (int)DxfLoader::add_entity(e, &renderer).size();
						}
					}
					std::printf("  %s: %d entities, %d inserts, %d shapes\n", entry.path().filename().string().c_str(), entities, inserts, shapes);
				}
				catch (const std::exception& ex)
				{
					g_run++;
					g_fail++;
					std::printf("  FAIL %s: %s\n", entry.path().filename().string().c_str(), ex.what());
				}
			}
		}
	}

	std::filesystem::remove(path);
	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
