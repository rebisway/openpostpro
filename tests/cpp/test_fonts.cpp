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
