// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// FONT TEST - loads the fonts of the system as the Text entity does, without any window: for each family of the list, the regular, bold,
// italic and bold italic files are opened by FreeType and the glyph of "A" and of "g" is extracted. It prints the families which fail
// and why, and the files of the font folder which no family uses.
//
// Run: test_fonts.exe [folder of fonts]   (exit code 0 = every family of the list can draw text)
#include <environment.h>
#include <font.h>
#include <strings.h>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

static bool loads(const std::string& family, const std::string& path, bool bold, bool italic, std::string& reason)
{
	try
	{
		Font font(family, path, bold, italic);
		std::vector<int> indices;
		auto contours = font.get_text_coordinates("Ag", indices, 10.0f, 0.0f);
		size_t points = 0;
		for (auto& c : contours)
			points += c.size();
		if (contours.empty() || points < 8)
		{
			reason = "no outline for \"Ag\" (" + std::to_string(contours.size()) + " contours)";
			return false;
		}
		return true;
	}
	catch (const char* text)
	{
		reason = std::string("FreeType: ") + text;
		return false;
	}
	catch (...)
	{
		reason = "exception";
		return false;
	}
}

int main(int argc, char** argv)
{
	auto families = environment::fonts();
	std::printf("%zu families\n", families.size());

	std::set<std::string> used;
	int failed = 0, files = 0;
	for (auto& entry : families)
	{
		const environment::FontFamily& f = entry.second;
		struct { const char* kind; const std::string* path; bool bold, italic; } styles[] = {
			{ "regular", &f.regular_path, false, false }, { "italic", &f.italic_path, false, true },
			{ "bold", &f.bold_path, true, false }, { "bold italic", &f.bold_italic_path, true, true } };
		if (f.regular_path.empty())
			std::printf("  (%s has no regular file: the text uses another style of it)\n", entry.first.c_str());
		for (auto& s : styles)
		{
			if (s.path->empty())
				continue;
			used.insert(stringex::to_lower(*s.path));
			files++;
			std::string reason;
			if (!loads(entry.first, *s.path, s.bold, s.italic, reason))
			{
				std::printf("  %-28s %-11s %s: %s\n", entry.first.c_str(), s.kind, std::filesystem::path(*s.path).filename().string().c_str(), reason.c_str());
				failed++;
			}
		}
	}

	auto check = [&](bool condition, const char* text) { if (!condition) { std::printf("  FAILED: %s\n", text); failed++; } };
	check(families.size() > 100, "a long list of families");
	check(!environment::font_family("Arial").regular_path.empty(), "Arial is found by its name");
	check(!environment::font_family("arial").regular_path.empty(), "arial (the name of the first versions) is still found");
	check(!environment::font_family("ARIAL").regular_path.empty(), "the case does not matter");
	check(environment::font_family("arial").name == "Arial", "the family has the name of the font file");
	check(!environment::font_family("Segoe UI").regular_path.empty() && !environment::font_family("Segoe UI").bold_path.empty(), "Segoe UI has its regular and its bold file (it was cut in 'segoeu' italic)");
	check(!environment::font_family("segoeui").regular_path.empty(), "the old name of Segoe UI finds it");
	check(!environment::font_family("Cambria").regular_path.empty(), "Cambria (a .ttc file) is in the list");
	check(environment::font_family("a font which does not exist").regular_path.empty(), "an unknown family is empty");
	for (auto& entry : families)
		check(entry.first == entry.second.name && !entry.first.empty(), "the key of the list is the name of the family");

	// the decoding of UTF-8: code points, the same on every platform
	{
		auto codes = stringex::to_utf32("a\xC3\xA9\xE4\xB8\xAD\xF0\xA0\x80\x80");
		check(codes.size() == 4 && codes[0] == 'a' && codes[1] == 0xE9 && codes[2] == 0x4E2D && codes[3] == 0x20000, "UTF-8 is decoded in code points (an astral character is one)");
		check(stringex::to_utf32("a\xFF" "b").size() == 3 && stringex::to_utf32("a\xFF" "b")[1] == 0xFFFD, "a byte which is not UTF-8 is U+FFFD");
		check(stringex::to_utf32("\xE4\xB8").size() == 2, "a character cut at the end gives replacement characters, no overrun");
		check(stringex::to_utf32("\xC0\x80")[0] == 0xFFFD, "a long form of a short code is refused");
		check(stringex::to_utf32("").empty(), "empty text");
		check(stringex::from_utf32(stringex::to_utf32("a\xC3\xA9\xE4\xB8\xAD\xF0\xA0\x80\x80")) == "a\xC3\xA9\xE4\xB8\xAD\xF0\xA0\x80\x80", "encoding gives back the same bytes");
		check(stringex::reverse_utf8("a\xC3\xA9\xE4\xB8\xAD") == "\xE4\xB8\xAD\xC3\xA9" "a", "reversing keeps the characters whole");

		// the bounds and the indices have one entry for each character, with accents and Chinese
		environment::FontFamily arial = environment::font_family("Arial");
		if (!arial.regular_path.empty())
		{
			Font font("Arial", arial.regular_path, false, false, arial.regular_index);
			std::vector<int> indices;
			auto contours = font.get_text_coordinates("\xC3\xA9t\xC3\xA9", indices, 10.0f, 0.0f);
			auto bounds = font.get_text_bounds("\xC3\xA9t\xC3\xA9", 10.0f, 0.0f);
			check(bounds.size() == 3 && indices.size() == 3, "three characters, three bounds and three indices (not one for each byte)");
			check(contours.size() >= 3, "accented letters have outlines");
		}
	}

	// the table of characters: the codes of a font and the glyphs of a page, for the symbol fonts and for Chinese
	{
		struct Case { const char* family; unsigned int code; const char* what; };
		for (const Case& c : { Case{ "Arial", 'A', "a Latin font has its letters" }, Case{ "Symbol", 0xF061, "Symbol has its alpha in the private zone U+F0xx" },
			Case{ "Webdings", 0xF021, "Webdings has its signs in the private zone U+F0xx" }, Case{ "Wingdings", 0xF021, "Wingdings has its signs in the private zone U+F0xx" },
			Case{ "Microsoft YaHei", 0x4E2D, "YaHei has the Chinese character U+4E2D" } })
		{
			environment::FontFamily f = environment::font_family(c.family);
			if (f.regular_path.empty())
			{
				std::printf("  %s: not on this system\n", c.family);
				continue;
			}
			std::vector<unsigned int> codes = Font::list_codes(f.regular_path, f.regular_index);
			check(std::find(codes.begin(), codes.end(), c.code) != codes.end(), c.what);
			check(std::is_sorted(codes.begin(), codes.end()), "the codes are in ascending order");

			std::vector<unsigned int> cells(256, 0xFFFFFFFFu);
			int on_page = 0;
			for (unsigned int code : codes)
				if ((code >> 8) == (c.code >> 8))
				{
					cells[code & 0xFF] = code;
					on_page++;
				}
			std::vector<unsigned char> rgba;
			int width = 0, height = 0;
			check(Font::render_cells(f.regular_path, f.regular_index, cells, 48, rgba, width, height) && width == 768 && height == 768, "the page is rendered");
			size_t inked = 0;
			for (size_t i = 3; i < rgba.size(); i += 4)
				if (rgba[i] > 128)
					inked++;
			std::printf("  %-16s %6zu codes, %3d on the page U+%04X, %zu inked pixels\n", c.family, codes.size(), on_page, c.code & 0xFF00, inked);
			check(on_page > 0 && inked > 2000, "the glyphs of the page have ink");
		}
	}

	// the text itself with a symbol font: the code given by the table (U+F061) and the letter typed (a) give the glyph, not the missing one
	{
		environment::FontFamily symbol = environment::font_family("Symbol");
		if (!symbol.regular_path.empty())
		{
			Font font("Symbol", symbol.regular_path, false, false, symbol.regular_index);
			font.get_glyph_coordinates(0xF061, 10.0f);
			font.get_glyph_coordinates('a', 10.0f);
			check(font.get_glyph(0xF061)->index() != 0, "Symbol: U+F061 is a glyph");
			check(font.get_glyph('a')->index() == font.get_glyph(0xF061)->index(), "Symbol: the letter a typed gives the glyph of U+F061");
			std::printf("  Symbol: glyph of U+F061 = %d, glyph of the letter a = %d\n", font.get_glyph(0xF061)->index(), font.get_glyph('a')->index());
		}
	}

	// Chinese, Japanese and an astral character (U+20000), with the CJK fonts of the system
	{
		const char* texts[] = { "\xE4\xB8\xAD\xE6\x96\x87", "\xE6\x97\xA5\xE6\x9C\xAC", "\xF0\xA0\x80\x80" };
		const char* names[] = { "Chinese (U+4E2D U+6587)", "Japanese (U+65E5 U+672C)", "astral (U+20000)" };
		for (const char* family : { "Microsoft YaHei", "SimSun", "MS Gothic", "Yu Gothic" })
		{
			environment::FontFamily f = environment::font_family(family);
			if (f.regular_path.empty())
			{
				std::printf("  %s: not on this system\n", family);
				continue;
			}
			for (int t = 0; t < 3; t++)
			{
				try
				{
					Font font(family, f.regular_path, false, false, f.regular_index);
					std::vector<int> indices;
					auto contours = font.get_text_coordinates(texts[t], indices, 10.0f, 0.0f);
					std::printf("  %-16s %-26s %zu contours\n", family, names[t], contours.size());
				}
				catch (const char* text)
				{
					std::printf("  %-16s %-26s FreeType error: %s\n", family, names[t], text);
				}
			}
		}
	}

	// the files of the folder which no family uses
#ifdef _WIN32
	std::string folder = argc > 1 ? argv[1] : "c:\\windows\\fonts";
#elif defined(__APPLE__)
	std::string folder = argc > 1 ? argv[1] : "/Library/Fonts";
#else
	std::string folder = argc > 1 ? argv[1] : "/usr/share/fonts";
#endif
	std::set<std::string> ignored_extensions;
	int unused = 0;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied))
	{
		std::string extension = stringex::to_lower(entry.path().extension().string());
		if (extension != ".ttf" && extension != ".otf" && extension != ".ttc")
			continue;
		if (used.count(stringex::to_lower(entry.path().string())) == 0)
		{
			unused++;
			if (unused <= 40)
				std::printf("  not in the list: %s\n", entry.path().filename().string().c_str());
		}
	}
	std::printf("%d files opened, %d failures, %d font files of the folder are in no family\n", files, failed, unused);
	return failed == 0 ? 0 : 1;
}
