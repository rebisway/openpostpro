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

#include "application.h"
#include <tool.h>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <exception>

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "lang.h"
#include <inifile.h>
#include <environment.h>
#include <config.h>
#include <renderer.h>
#include <logger.h>

#include "test_cad.h"
#include <../import/dxfloader.h>
#include <../export/dxfexport.h>
#include <dialog.h>
#include <file.h>
#include "stringex.h"
#include <version.h>
#include <history.h>
#include <script.h>


#include "mod_log.h"
#include "mod_output.h"
#include "mod_sim.h"
#include "bridge.h"
#include "mod_cad.h"
#include "mod_script.h"


Application::Application(std::string title) : Window(title)
{

}

Application::~Application()
{
}

void Application::splash(const std::string& message)
{
	if (!_splash_on)
		return;
	if (_splash == nullptr)
	{
		// the functions of OpenGL are loaded here when the renderers of the modules do not exist yet (loading twice is harmless)
		if (!OpenGlRenderer::load_functions())
		{
			_splash_on = false;
			return;
		}
		GlTexture* texture = new GlTexture();
		texture->load("splash.png");
		_splash = texture;
	}
	int fw = 0, fh = 0;
	glfwGetFramebufferSize(glfw_window(), &fw, &fh);
	glViewport(0, 0, fw, fh);
	glClearColor(20.0f / 255.0f, 25.0f / 255.0f, 35.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* list = ImGui::GetForegroundDrawList();
		ImVec2 size((float)_splash->width(), (float)_splash->height());
		ImVec2 pos(std::floor((io.DisplaySize.x - size.x) / 2), std::floor((io.DisplaySize.y - size.y) / 2));
		if (_splash->id() != nullptr)
			list->AddImage((ImTextureID)_splash->id(), pos, ImVec2(pos.x + size.x, pos.y + size.y));

		// the name and the version under the logo, the step of the loading above the line
		ImFont* font = ImGui::GetFont();
		auto centered = [&](float size_px, float y, ImU32 color, const std::string& text)
		{
			ImVec2 extent = font->CalcTextSizeA(size_px, FLT_MAX, 0.0f, text.c_str());
			list->AddText(font, size_px, ImVec2(pos.x + std::floor((size.x - extent.x) / 2), pos.y + y), color, text.c_str());
		};
		centered(34.0f, 196.0f, IM_COL32(240, 240, 240, 255), "OpenPostPro");
		centered(15.0f, 232.0f, IM_COL32(255, 154, 31, 255), "v" + appinfo::version());
		centered(14.0f, size.y - 52.0f, IM_COL32(170, 175, 190, 255), message);
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(glfw_window());
}

void Application::splash_end()
{
	_splash_on = false;
	if (_splash != nullptr)
	{
		delete (GlTexture*)_splash;
		_splash = nullptr;
	}
}

void Application::initialize()
{
	try
	{
		Window::initialize();
	}
	catch (const std::exception& e)
	{
		Logger::error("Initializing window [" + std::string(e.what()) + "]");
	}

	//auto ll = file::read_all_lines("c:\\dev\\colors.txt");
	//std::vector<std::string> res;
	//for (auto s : ll)
	//{
	//	auto i = s.find("vec4(");
	//	auto v = s.substr(i + 5, 8);
	//	unsigned int ai, ri, gi, bi;
	//	float a, r, g, b;

	//	unsigned int x;
	//	std::stringstream sa, sr, sg, sb;
	//	sa << std::hex << v.substr(0, 2);
	//	sa >> ai;
	//	a = ai / 255.0f;
	//	sr << std::hex << v.substr(2, 2);
	//	sr >> ri;
	//	r = ri / 255.0f;
	//	sg << std::hex << v.substr(4, 2);
	//	sg >> gi;
	//	g = gi / 255.0f;
	//	sb << std::hex << v.substr(6, 2);
	//	sb >> bi;
	//	b = bi / 255.0f;

	//	res.push_back(s.substr(0, i + 5) + std::to_string(r) + "f, " + std::to_string(g) + "f, " + std::to_string(b) + "f, " + std::to_string(a) + "f);");
	//}
	//file::write_all_lines("c:\\dev\\colors2.txt", res);

	_application_title = title();

	try
	{
		config.read();
		tool_table().load();
		machine_table().load();
	}
	catch (const std::exception& e)
	{
		Logger::error("Reading configuration [" + std::string(e.what()) + "]");
	}

	// apply the saved language before the modules, post-processors and scripts are loaded
	if (!config.language.empty() && config.language != Lang::getName())
	{
		auto languages = Lang::get_langs();
		if (std::find(languages.begin(), languages.end(), config.language) != languages.end())
			Lang::load(config.language);
	}

	// the start screen, as soon as the language is known (the fonts of the application are added below)
	// the start screen draws a frame before the settings of the windows are read: no ini file yet, or the settings would be
	// considered as read (and saved over) without their file
	ImGui::GetIO().IniFilename = nullptr;
	splash(Lang::t("SPLASH_START"));

	/*_textures.push_back(load_texture("image2.jpg"));
	_textures.push_back(load_texture("box-4x.png"));
	_textures.push_back(load_texture("bug-4x.png"));
	_textures.push_back(load_texture("calculator-4x.png"));*/

	float round = 0.0f;
	ImGui::GetStyle().WindowRounding = round;// <- Set this on init or use ImGui::PushStyleVar()
	ImGui::GetStyle().ChildRounding = round;
	ImGui::GetStyle().FrameRounding = round;
	ImGui::GetStyle().GrabRounding = round;
	ImGui::GetStyle().PopupRounding = round;
	ImGui::GetStyle().ScrollbarRounding = round;

	// Initialize modules - see config.cpp
	// Class to instanciate
	_modules.push_back(new ModCad(this));
	_modules.push_back(new ModOutput(this));
	_modules.push_back(new ModSim(this));
	_modules.push_back(new ModLog(this));
	auto script_module = new ModScript(this);
	script_module->on_compiled = [this](const std::string& path) { reload_script(path); };
	_modules.push_back(script_module);
	_current_module = _modules[0];

#ifdef _DEBUG
	// the commands of the test bridge which belong to the application (see bridge.h)
	bridge::application = [this](const std::vector<std::string>& c) -> bool
	{
		if (c[0] == "module" && c.size() > 1)
		{
			// "module sim", "module cad": the tab of the module is shown
			for (Module* m : _modules)
			{
				std::string code = m->code();
				std::transform(code.begin(), code.end(), code.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
				if (code == "mod_" + c[1] || code == c[1])
				{
					m->show(true);
					m->selected(true);
					_current_module = m;
					bridge::log("module " + m->code());
					return true;
				}
			}
			return false;
		}
		if (c[0] == "open" && c.size() > 1)
		{
			load_file(c[1]);
			bridge::log("document " + c[1]);
			return true;
		}
		if (c[0] == "post")
		{
			// the post-processor of the menu, then the output is shown
			if (_current_postpro == nullptr)
				return false;
			_current_postpro->run(_document, config.output_path, appinfo::version());
			bridge::log("post-processor run, output " + _document->output());
			return true;
		}
		if (c[0] == "pocket")
		{
			// "pocket 0" (offset), "pocket 1" ...: a pocket of all the shapes of the document, in the current group
			ModCad* cad = (ModCad*)module("MOD_CAD");
			if (cad == nullptr)
				return false;
			_document->select_all();
			if (c.size() > 2)
			{
				// "pocket 1 text" or "pocket 1 other": only the texts, or only the other shapes
				std::vector<Graphic*> kept;
				for (Graphic* g : _document->selected())
					if ((g->type() == GraphicType::Text) == (c[2] == "text"))
						kept.push_back(g);
				_document->unselect_all();
				_document->select(kept);
			}
			cad->cam_pocket((PocketMode)(c.size() > 1 ? std::stoi(c[1]) : 0));
			bridge::log("pocket done");
			return true;
		}
		if (c[0] == "nestwindow")
		{
			// every shape is selected and the window of the nesting is shown
			ModCad* cad = (ModCad*)module("MOD_CAD");
			if (cad == nullptr)
				return false;
			cad->cad_nesting_window();
			bridge::log("nesting window");
			return true;
		}
		if (c[0] == "nest")
		{
			// "nest <width> <height> <spacing> [quantity [seconds]]": every shape of the document is nested, for some seconds if they are given with the algorithm 1 (search, the default) or 2 (overlap)
			ModCad* cad = (ModCad*)module("MOD_CAD");
			if (cad == nullptr || c.size() < 4)
				return false;
			cad->cad_nesting_bridge(std::stof(c[1]), std::stof(c[2]), std::stof(c[3]), c.size() > 4 ? std::stoi(c[4]) : 1, c.size() > 5 ? std::stof(c[5]) : 0, c.size() > 6 ? std::stoi(c[6]) : 1);
			bridge::log("nesting done");
			return true;
		}
		if (c[0] == "save" && c.size() > 1)
		{
			save_file(c[1]);
			bridge::log("saved " + c[1]);
			return true;
		}
		if (c[0] == "sim")
		{
			ModSim* sim = (ModSim*)module("MOD_SIM");
			return sim != nullptr && sim->command(c);
		}
		return false;
	};
#endif
	// parameters (simple and effective...)

	switch (config.display_style)
	{
	case 0: ImGui::StyleColorsClassic(); break;
	case 1: ImGui::StyleColorsDark(); break;
	case 2: ImGui::StyleColorsLight(); break;
	}

	// fonts
	ImGuiIO& io = ImGui::GetIO();
	auto font_path = environment::combine_path(environment::combine_path(environment::application_path(), "fonts"), "Roboto-Medium.ttf");
	if (std::filesystem::exists(font_path))
	{
		_font = io.Fonts->AddFontFromFileTTF(environment::combine_path(environment::combine_path(environment::application_path(), "fonts"), "Roboto-Medium.ttf").c_str(), 16);
		// the start screen has already built the atlas of the fonts: it is built again with the new font
		io.Fonts->ClearTexData();
		ImGui_ImplOpenGL3_DestroyFontsTexture();
		ImGui_ImplOpenGL3_CreateFontsTexture();
	}
	else
	{
		Logger::error("Missing Font : " + font_path);
	}

	// set the imgui ini file in same application data path
	_imgui_ini = environment::application_data_path() + "imgui.ini";
	io.IniFilename = _imgui_ini.c_str();
	if (config.first)
	{
		// if first run_python, create a default imgui ini file and force loading
		IniFile im(_imgui_ini);
		im.set("Window][###PROJECT", "Pos", "11,62");
		im.set("Window][###PROJECT", "Size", "215,238");
		im.set("Window][###OBJECT", "Pos", "11,305");
		im.set("Window][###OBJECT", "Size", "215,400");
		im.write();
	}
	// the positions and the sizes of the windows of the last session
	ImGui::LoadIniSettingsFromDisk(io.IniFilename);

	onSizeChanged(width(), height());

	// the first start asks for the language and the unit
	_display_first_run = config.first;
	_first_language = Lang::getName();
	_first_unit = config.default_unit;

	splash(Lang::t("SPLASH_DOCUMENT"));

	// Default document
	_document = new Document();
	_document->unit(config.default_unit == 1 ? Unit::Inch : Unit::Millimeter);
	if (config.open_last_file)
		load_file(config.last_file_path);
	else
		new_document();

	splash_end();

	run_test(_document);

	/*IPython test;

	int args[] = { 5, 6 };
	long result = (long)test.run_python(
		"multiply",
		"multiply",
		&args
		,
		2
	);*/

	///////////////////////////////////
	// Initialize the python library //
	///////////////////////////////////

	auto appl = environment::combine_path(environment::application_path(), "postpro");
	auto path = environment::combine_path(environment::application_data_path(), "postpro");
	splash(Lang::t("SPLASH_SCRIPTS"));
	Script::initialize_python();
	Script::add_module_path(environment::combine_path(environment::application_data_path(), "postpro"));
	Script::add_module_path(environment::combine_path(environment::application_data_path(), "script"));

	auto pp = Script::sys_path();

	////////////////////////////////////
	// Initialize the postpro scripts //
	////////////////////////////////////
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	if (std::filesystem::exists(appl))
	{
		for (const auto& entry : std::filesystem::directory_iterator(appl))
		{
			auto dst = environment::combine_path(path, entry.path().filename().string());
			file::write_all_text(dst, file::read_all_text(entry.path().string()));
		}
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			auto file = entry.path().string();
			if (entry.path().extension() == ".py" && !stringex::end_with(file, "postcore.py"))
				load_postpro(file);
		}
		if (_postpros.size() > 0)
		{
			if (config.postpro == "")
				config.postpro = _postpros[0]->module();

			for (auto p : _postpros)
			{
				if (config.postpro == p->module())
					_current_postpro = p;
			}
		}
	}

	/////////////////////////////////////////
	// Initialize the translators of the simulator //
	/////////////////////////////////////////
	appl = environment::combine_path(environment::application_path(), "simulator");
	path = environment::combine_path(environment::application_data_path(), "simulator");
	if (!std::filesystem::exists(path))
		std::filesystem::create_directory(path);
	if (std::filesystem::exists(appl))
	{
		for (const auto& entry : std::filesystem::directory_iterator(appl))
		{
			auto dst = environment::combine_path(path, entry.path().filename().string());
			file::write_all_text(dst, file::read_all_text(entry.path().string()));
		}
	}
	Script::add_module_path(path);

	////////////////////////////////
	// Initialize the cad scripts //
	////////////////////////////////
	appl = environment::combine_path(environment::application_path(), "script");
	path = environment::combine_path(environment::application_data_path(), "script");

	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	if (std::filesystem::exists(appl))
	{
		for (const auto& entry : std::filesystem::directory_iterator(appl))
		{
			auto dst = environment::combine_path(path, entry.path().filename().string());
			file::write_all_text(dst, file::read_all_text(entry.path().string()));
		}
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			auto file = entry.path().string();
			if (entry.path().extension() == ".py" && !stringex::end_with(file, "cadcore.py"))
				load_cad_script(file);
		}
	}
}


// true if the source defines the class "name:" or "name(" (a file without it is a library module)
static bool defines_class(const std::string& source, const std::string& name)
{
	return source.find("class " + name + ":") != std::string::npos || source.find("class " + name + "(") != std::string::npos;
}

Postpro* Application::load_postpro(const std::string& file)
{
	auto name = std::filesystem::path(file).filename().string();

	// a file without the class is a module used by other post-processors
	if (!defines_class(file::read_all_text(file), "Postpro"))
	{
		Logger::log("Module " + name + " is not a post-processor, skipped");
		return nullptr;
	}

	Postpro* p = new Postpro();

	try
	{
		if (p->initialize(file, Lang::getName()))
		{
			_postpros.push_back(p);
			Logger::log("Loading post-processor " + name);
			p->finalize();
			return p;
		}
		Logger::error("Loading post-processor " + name + " -> " + std::string(p->error()));
		p->finalize();
	}
	catch (const std::exception& e)
	{
		Logger::error("Loading post-processor " + name + " -> " + std::string(e.what()));
	}

	delete p;
	return nullptr;
}

CadScript* Application::load_cad_script(const std::string& file)
{
	auto name = std::filesystem::path(file).filename().string();

	// a file without the class is a module used by other scripts
	if (!defines_class(file::read_all_text(file), "Cadscript"))
	{
		Logger::log("Module " + name + " is not a script, skipped");
		return nullptr;
	}

	CadScript* p = new CadScript();

	try
	{
		if (p->initialize(file, Lang::getName()))
		{
			_scripts.push_back(p);
			Logger::log("Loading script " + name);
			p->finalize();
			return p;
		}
		Logger::error("Loading script " + name + " -> " + std::string(p->error()));
		p->finalize();
	}
	catch (const std::exception& e)
	{
		Logger::error("Loading script " + name + " -> " + std::string(e.what()));
	}

	delete p;
	return nullptr;
}

std::string Application::clone()
{
	// the name is a python module name
	static const std::regex valid_name("^[A-Za-z_][A-Za-z0-9_]*$");
	if (_clone_source.empty() || !std::regex_match(_clone_name, valid_name))
		return "CLONE_ERR_NAME";

	// the module must not hide another one, including the python libraries
	Script::run_python("import importlib\nimportlib.invalidate_caches()");
	if (Script::module_exists(_clone_name))
		return "CLONE_ERR_EXISTS";

	// the description of each language of the copy is replaced
	auto description = stringex::replace(_clone_description, "\\", "\\\\");
	description = stringex::replace(description, "'", "\\'");
	description = stringex::replace(description, "$", "$$");
	static const std::regex description_key("('description'\\s*:\\s*)'[^']*'");
	auto text = std::regex_replace(file::read_all_text(_clone_source), description_key, "$1'" + description + "'");

	auto path = environment::combine_path(environment::combine_path(environment::application_data_path(), _clone_script ? "script" : "postpro"), _clone_name + ".py");
	file::write_all_text(path, text);

	Script::run_python("import importlib\nimportlib.invalidate_caches()");

	std::string loaded;
	if (_clone_script)
	{
		auto p = load_cad_script(path);
		if (p == nullptr)
			return "CLONE_ERR_LOAD";
		loaded = p->path();
	}
	else
	{
		auto p = load_postpro(path);
		if (p == nullptr)
			return "CLONE_ERR_LOAD";

		// the clone is selected
		_current_postpro = p;
		config.postpro = p->module();
		loaded = p->path();
	}

	// the clone is opened in the editor
	auto m = module("MOD_SCRIPT");
	m->load(loaded);
	m->show(true);
	m->selected(true);

	return "";
}

void Application::save_current()
{
	// the module edits its own file (script, output), otherwise the document is saved
	if (_current_module->save())
		return;

	if (std::filesystem::exists(_document->path()))
		save_file(_document->path());
	else
	{
		std::string path = "";
		if (dialog::save_file_dialog("", path, "OpenPostPro", "*.opp"))
		{
			if (!stringex::end_with(path, ".opp"))
				path += ".opp";
			save_file(path);
		}
	}
}

void Application::reload_script(const std::string& path)
{
	auto name = std::filesystem::path(path).stem().string();

	auto error = Script::reload_module(name);
	if (!error.empty())
	{
		Logger::error("Reloading " + name + " -> " + error);
		return;
	}

	// the description, the category and the properties are read again
	for (auto p : _postpros)
	{
		if (p->path() == path)
		{
			if (p->initialize(path, Lang::getName()))
				Logger::log("Post-processor " + name + " reloaded");
			else
				Logger::error("Reloading " + name + " -> " + std::string(p->error()));
			p->finalize();
		}
	}
	for (auto s : _scripts)
	{
		if (s->path() == path)
		{
			if (s->initialize(path, Lang::getName()))
				Logger::log("Script " + name + " reloaded");
			else
				Logger::error("Reloading " + name + " -> " + std::string(s->error()));
			s->finalize();
		}
	}
}

void Application::finalize()
{
	title(_application_title);

	config.last_file_path = _document->path();
	config.write();

	for (int i = 0; i < _modules.size(); i++)
		delete _modules[i];
	_modules.clear();

	delete _document;

	for (auto p : _postpros)
		delete p;
	_postpros.clear();

	Script::finalize_python();
	Window::finalize();
}

void Application::onSizeChanged(float width, float height)
{
	Window::onSizeChanged(width, height);

	for (auto module : _modules)
	{
		module->display(_nav_width, (float)_menu_height, width - _nav_width, height);
	}

}

void Application::onMouseMove(double xpos, double ypos)
{
	Window::onMouseMove(xpos, ypos);

	_xpos = (int)xpos;
	_ypos = (int)ypos;

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
	{
		if (_move_mutex.try_lock())
		{
			_current_module->mouseMouve(xpos, ypos);
			_move_mutex.unlock();
		}
	}
}

void Application::onLeftButtonDown()
{
	Window::onLeftButtonDown();

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		_current_module->left_button_down();
}

void Application::onLeftButtonUp()
{
	Window::onLeftButtonUp();

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		_current_module->left_button_up();
}

void Application::onRightButtonDown()
{
	Window::onRightButtonDown();

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		_current_module->right_button_down();
}

void Application::onRightButtonUp()
{
	Window::onRightButtonUp();

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		_current_module->right_button_up();
}

void Application::onScroll(double xoffset, double yoffset)
{
	Window::onScroll(xoffset, yoffset);

	if ( !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) )
		_current_module->scroll(xoffset, yoffset);
}

void Application::onEnter()
{
	Window::onEnter();

	_current_module->enter();
}

void Application::onLeave()
{
	Window::onLeave();

	_current_module->leave();
}

void Application::onKeyPressed(int key, int scancode, int action, int mods)
{
	Window::onKeyPressed(key, scancode, action, mods);

	// CTRL+S saves the file edited by the current module, or the document
	if (action == GLFW_PRESS && key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL))
	{
		_save_requested = true;
		return;
	}
	// the keys are compared by the letter they type, not by their position: on an AZERTY keyboard the key of the A is the Q of the codes
	if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL))
	{
		const char* name = glfwGetKeyName(key, scancode);
		char letter = name != nullptr ? (char)std::tolower((unsigned char)name[0]) : 0;
		if (letter == 'n')
		{
			_new_requested = true;
			return;
		}
		if (letter == 'o')
		{
			_open_requested = true;
			return;
		}
		if (letter == 'q')
		{
			close();
			return;
		}
	}

	bool throw_module = !ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow);

	if (key == GLFW_KEY_F2 || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_A)
		throw_module = true;

	if (throw_module)
		_current_module->key_pressed(key, scancode, action, mods);
}

void Application::onCharacterInput(unsigned int codepoint)
{
	Window::onCharacterInput(codepoint);

	if (!ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow))
		_current_module->character_input(codepoint);
}

void Application::onDrop(std::vector<std::string> paths)
{
	Window::onDrop(paths);

	_current_module->drop(paths);
}

void Application::onRenderFrame()
{
	Window::onRenderFrame();

	if (_mutex.try_lock())
	{
		try 
		{
			_current_module->render_frame();
			_mutex.unlock();
		}
		catch (const std::exception& e)
		{
			_mutex.unlock();
			Logger::log(std::string("ERROR : ") + e.what());
		}
	}
}

void Application::onRenderGUI()
{
	ImGui::PushFont(_font);


	Window::onRenderGUI();

	if (!_initialized)
	{
		IniFile ini;

		for (Module* m : _modules)
		{
			m->initialize_GUI(ini);
		}

		_initialized = true;
	}

	// navigation

	// Navbar style
	//if (_nav_style == 1) {
	//	ImGui::SetNextWindowSize(ImVec2(_nav_width, (float)(height() - _menu_height + 1)));
	//	ImGui::SetNextWindowPos(ImVec2(0.0f, (float)(_menu_height - 1)));

	//	ImGui::Begin("navigation_bar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

	//	for (int i = 0; i < _modules.size(); i++)
	//	{
	//		auto module = _modules[i];
	//		/*auto texture = _textures[module->textureIndex()];
	//		if (ImGui::ImageButton(texture->id(), ImVec2((float)texture->width(), (float)texture->height())))
	//		{
	//			_current_module = module;
	//		}*/
	//	}

	//	ImGui::End();
	//	_current_module->render_GUI();
	//}

	// Tab style
	if (_nav_style == 0) {
		ImGui::SetNextWindowSize(ImVec2((float)width(), (float)0));
		ImGui::SetNextWindowPos(ImVec2(0.0f, (float)(_menu_height - 1)));
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::Begin("TabsWindow", NULL, flags);
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		ImGui::BeginTabBar("navigation_tabs", tab_bar_flags);

		auto last_pos = ImGui::GetWindowPos();
		auto last_size = ImGui::GetWindowSize();
		auto top = last_pos.y + last_size.y;

		// the log can be a panel at the bottom of the window: the modules keep the space above it
		auto log = (ModLog*)module("MOD_LOG");
		float dock_height = (log->docked() && log->show()) ? (float)config.log_height : 0.0f;
		auto height = ImGui::GetMainViewport()->Size.y - top - dock_height;

		if (_top_left.y != top - 1 || _bottom_right.x != width() || _bottom_right.y != (height + 1))
		{
			_top_left.y = top - 1;
			_bottom_right.x = width();
			_bottom_right.y = height + 1;

			for (Module* m : _modules)
				m->display(_top_left.x, _top_left.y, _bottom_right.x, _bottom_right.y);
		}

		for (int i = 0; i < _modules.size(); i++)
		{
			auto module = _modules[i];
			if (module->docked())
				continue;
			bool* open = NULL;
			if (module->closable())
			{
				open = (bool*)malloc(sizeof(bool));
				*open = module->show();
			}
			ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
			if (module->selected() || module == _current_module)
			{
				flags = ImGuiTabItemFlags_SetSelected;
				module->selected(false);
			}

			if (ImGui::BeginTabItem(module->title().c_str(), open, flags))
			{
				if (_mutex.try_lock())
				{
					try
					{
						if (open != NULL)
							module->show(*open);

						if (ImGui::IsItemFocused())
						{
							_current_module = module;
							module->selected(true);
						}				

						module->render_frame();
						module->render_GUI();
						_mutex.unlock();
					}
					catch (const std::exception& e)
					{
						_mutex.unlock();
						Logger::log(std::string("ERROR : ") + e.what());
					}
				}

				ImGui::EndTabItem();
			}
			if (open != NULL)
				free(open);
		}

		ImGui::EndTabBar();
		ImGui::End();

		if (dock_height > 0.0f)
			log->render_dock(ImGui::GetMainViewport()->Size.y - dock_height, (float)width(), dock_height);
	}

	render_menu();

	ImGui::PopFont();
}

// the most recent document is the first one of the list, which keeps ten of them
void Application::add_recent(const std::string& path)
{
	auto& history = config.file_history;
	history.erase(std::remove(history.begin(), history.end(), path), history.end());
	history.insert(history.begin(), path);
	if (history.size() > 10)
		history.resize(10);
}

void Application::open_dialog()
{
	std::string path("");
	if (dialog::open_file_dialog("", path, "OpenPostPro", "*.opp"))
		load_file(path);
}

// the folder of the example projects: next to the program, or in the docs folder of the sources
static std::string examples_folder()
{
	namespace fs = std::filesystem;
	std::error_code error;
	fs::path folder = fs::path(environment::application_path());
	for (int i = 0; i < 6 && !folder.empty(); i++)
	{
		for (const char* name : { "examples", "docs/examples" })
		{
			fs::path candidate = folder / name;
			if (fs::is_directory(candidate, error))
				return candidate.string();
		}
		folder = folder.parent_path();
	}
	return "";
}

// the examples in the language of the interface (French, otherwise English): the name shown and the path
static std::vector<std::pair<std::string, std::string>> example_files(const std::string& folder)
{
	namespace fs = std::filesystem;
	std::string wanted = Lang::getName().substr(0, 2) == "fr" ? "fr" : "en";
	std::vector<std::pair<std::string, std::string>> result;
	std::error_code error;
	for (const auto& entry : fs::directory_iterator(folder, error))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".opp")
			continue;
		std::string name = entry.path().stem().string();
		size_t dot = name.rfind('.');
		if (dot != std::string::npos)
		{
			// "plaque.fr": the tag of the language is not shown, the examples of the other languages are not listed
			if (name.substr(dot + 1) != wanted)
				continue;
			name = name.substr(0, dot);
		}
		result.push_back({ name, entry.path().string() });
	}
	std::sort(result.begin(), result.end());
	return result;
}

void Application::render_menu()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 7;

	// the button of the log window switches it between the full screen (a tab) and the panel at the bottom of the window
	Module* log = module("MOD_LOG");
	if (((ModLog*)log)->take_dock_toggle())
	{
		config.log_docked = !config.log_docked;
		log->show(true);
		if (config.log_docked)
		{
			// the log leaves the tabs: the drawing becomes the current module again
			if (_current_module == log)
			{
				_current_module = _modules[0];
				_modules[0]->selected(true);
			}
		}
		else
		{
			// the log comes back as a tab and stays the current one (the tab which was current must not ask to be selected again)
			_current_module->selected(false);
			_current_module = log;
			log->selected(true);
		}
	}

	if (ImGui::BeginMainMenuBar())
	{
		_menu_height = (int)ImGui::GetWindowHeight();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 2.0f);
		if (ImGui::BeginMenu(Lang::l("FILE")))
		{
			if (ImGui::MenuItem(Lang::l("FILE_NEW"), "CTRL+N", false)) 
			{
				new_document();
			}
			if (ImGui::MenuItem(Lang::l("FILE_OPEN"), "CTRL+O", false))
			{
				open_dialog();
			}
			// the recent documents (the ones which do not exist any more are dropped)
			auto& history = config.file_history;
			history.erase(std::remove_if(history.begin(), history.end(), [](const std::string& p) { return !std::filesystem::exists(p); }), history.end());
			if (ImGui::BeginMenu(Lang::l("FILE_RECENT"), !history.empty()))
			{
				std::string chosen;
				for (size_t i = 0; i < history.size(); i++)
				{
					std::string name = std::filesystem::path(history[i]).filename().string();
					if (ImGui::MenuItem((name + "###RECENT_" + std::to_string(i)).c_str()))
						chosen = history[i];
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", history[i].c_str());
				}
				ImGui::Separator();
				if (ImGui::MenuItem(Lang::l("FILE_RECENT_CLEAR")))
					history.clear();
				ImGui::EndMenu();
				if (!chosen.empty())
					load_file(chosen);
			}
			// the examples are copies: the document has no file, so a save asks for a name
			std::string examples = examples_folder();
			if (ImGui::BeginMenu(Lang::l("FILE_EXAMPLES"), !examples.empty()))
			{
				std::string chosen;
				for (const auto& example : example_files(examples))
					if (ImGui::MenuItem(example.first.c_str()))
						chosen = example.second;
				ImGui::EndMenu();
				if (!chosen.empty())
				{
					load_file(chosen, false);
					_document->path("");
					title(_application_title);
				}
			}
			if (ImGui::MenuItem(Lang::l("FILE_SAVE"), "CTRL+S", false)) 
			{
				save_current();
			}
			if (ImGui::MenuItem(Lang::l("FILE_SAVE_AS"), NULL, false)) 
			{
				std::string path = "", p;
				if (dialog::save_file_dialog("", path, "OpenPostPro", "*.opp"))
				{
					if (!stringex::end_with(path, ".opp"))
						path += ".opp";
					save_file(path);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("FILE_IMPORT"), NULL, false)) {
				std::string path("");
				if ( dialog::open_file_dialog("", path, "DXF", "*.dxf") )
					load_dxf(path);
			}
			if (ImGui::MenuItem(Lang::l("FILE_EXPORT_DXF"), NULL, false)) {
				std::string path("");
				if (dialog::save_file_dialog("", path, "DXF", "*.dxf"))
				{
					if (!stringex::end_with(path, ".dxf"))
						path += ".dxf";
					export_dxf(path);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("FILE_EXIT"), "CTRL+Q")) 
			{ 
				close(); 
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(Lang::l("EDIT")))
		{
			if (ImGui::MenuItem(Lang::l("EDIT_UNDO"), "CTRL+Z", false, _current_module->can_undo()))
			{
				_current_module->undo();
			}
			if (ImGui::MenuItem(Lang::l("EDIT_REDO"), "CTRL+Y", false, _current_module->can_redo()))
			{
				_current_module->redo();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("EDIT_CUT"), "CTRL+X", false))
			{
				_current_module->cut();
			}
			if (ImGui::MenuItem(Lang::l("EDIT_COPY"), "CTRL+C", false))
			{
				_current_module->copy();
			}
			if (ImGui::MenuItem(Lang::l("EDIT_PASTE"), "CTRL+V", false))
			{
				_current_module->paste();
			}
			if (ImGui::MenuItem(Lang::l("EDIT_REMOVE"), "DEL", false))
			{
				_current_module->remove();
			}
			if (_current_module->code() == "MOD_CAD")
			{
				bool any = _document->selected().size() > 0;
				if (ImGui::MenuItem(Lang::l("EDIT_DUPLICATE"), NULL, false, any))
					((ModCad*)_current_module)->duplicate();
				if (ImGui::MenuItem(Lang::l("EDIT_ARRAY"), NULL, false, any))
					((ModCad*)_current_module)->array_window();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu(Lang::l("EDIT_SELECT")))
			{
				if (ImGui::MenuItem(Lang::l("EDIT_SELECT_ALL"), "CTRL+A", false))
				{
					_current_module->select_all();
				}
				if (_current_module->code() == "MOD_CAD")
				{
					if (ImGui::MenuItem(Lang::l("EDIT_SELECT_SELECTED_LAYER"), NULL, false))
					{
						if (_document->selected().size() == 1 && _document->selected()[0]->type() == GraphicType::Layer)
						{
							Layer* layer = (Layer*)_document->selected()[0];
							for (Graphic* g : _document->selected())
								g->selected(false);
							_document->selected().clear();
							for (Shape* s : layer->shapes())
							{
								_document->selected().push_back(s);
								s->selected(true);
							}
						}
					}
					if (ImGui::BeginMenu(Lang::l("EDIT_SELECT_LEVEL")))
					{
						for (Layer* l : _document->layers())
						{
							if (ImGui::MenuItem(l->name().c_str(), NULL, false))
							{
								for (Graphic* g : _document->selected())
									g->selected(false);
								_document->selected().clear();
								for (Shape* s : l->shapes())
								{
									_document->selected().push_back(s);
									s->selected(true);
								}
							}
						}
						ImGui::EndMenu();
					}
				}
				if (_current_module->code() == "MOD_CAD" && _document->selected().size() > 0 && ImGui::MenuItem(Lang::l("EDIT_SELECT_CONTOUR"), NULL, false))
				{
					((ModCad*)_current_module)->select_contour();
				}
				if (ImGui::MenuItem(Lang::l("EDIT_SELECT_REVERSE"), NULL, false))
				{
					_current_module->reverse_selection();
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("PREFERENCES")))
			{
				_display_preferences = true;
			}
			ImGui::EndMenu();
		}

		for (Module* m : _modules)
		{
			if (m == _current_module)
				m->render_menu();
		}
		

		if (ImGui::BeginMenu(Lang::l("POST")))
		{
			if (_current_postpro != nullptr && ImGui::MenuItem((Lang::t("POST_RUN") + " (" + _current_postpro->description() + ")").c_str()))
			{
				if (_document->path().empty() || !std::filesystem::exists(_document->path()))
				{
					std::string path = "", p;
					if (dialog::save_file_dialog("", path, "OpenPostPro", "*.opp"))
					{
						if (!stringex::end_with(path, ".opp"))
							path += ".opp";
						save_file(path);
					}
				}
				_current_postpro->run(_document, config.output_path, appinfo::version());
				auto m = module("MOD_OUTPUT");
				m->show(true);
				m->selected(true);
			}
			if (_current_postpro != nullptr && ImGui::MenuItem(Lang::l("POST_SIMULATE")))
			{
				// the same as the run of the post-processor, then the output is simulated
				_current_postpro->run(_document, config.output_path, appinfo::version());
				auto sim = (ModSim*)module("MOD_SIM");
				sim->simulate_output();
				sim->show(true);
				sim->selected(true);
				_current_module = sim;
			}
			ImGui::Separator();
			// the configuration needs properties, the delivered post-processors cannot be edited (they are copied again at each start)
			bool configure = _current_postpro != nullptr && _current_postpro->has_properties_dialog();
			bool edit = _current_postpro != nullptr && !_current_postpro->read_only();
			if (configure && ImGui::MenuItem(Lang::l("POST_CONFIG")))
			{
				_display_postpro_preferences = true;
			}
			if (ImGui::BeginMenu(Lang::l("POST_MANAGE")))
			{
				if (edit && ImGui::MenuItem(Lang::l("POST_EDIT")))
				{
					auto m = module("MOD_SCRIPT");
					m->load(_current_postpro->path());
					m->show(true);
					m->selected(true);
				}
				if (_current_postpro != nullptr && ImGui::MenuItem(Lang::l("POST_CLONE")))
				{
					_clone_source = _current_postpro->path();
					_clone_script = false;
					_clone_name = _current_postpro->module() + "_custom";
					_clone_description = _current_postpro->description();
					_clone_error = "";
					_display_clone = true;
				}
				if (ImGui::MenuItem(Lang::l("POST_OPEN_FOLDER")))
				{
					dialog::open_folder(environment::combine_path(environment::application_data_path(), "postpro"));
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu(Lang::l("POST_SELECT")))
			{
				for (int i = 0; i<_postpros.size(); i++)
				{
					if (ImGui::BeginMenu(_postpros[i]->category().c_str()))
					{
						if (ImGui::MenuItem(Lang::l(_postpros[i]->description()), NULL, _postpros[i] == _current_postpro))
						{
							_current_postpro = _postpros[i];
							config.postpro = _current_postpro->module();
						}
						ImGui::EndMenu();
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(Lang::l("SCRIPTS")))
		{
			//if (ImGui::BeginMenu(Lang::l("POST_SELECT")))
			{
				// one menu per category, in the order of the first script found
				std::vector<std::string> categories;
				for (auto script : _scripts)
					if (std::find(categories.begin(), categories.end(), script->category()) == categories.end())
						categories.push_back(script->category());
				for (auto& category : categories)
				{
					if (!ImGui::BeginMenu(category.c_str()))
						continue;
					for (int i = 0; i < _scripts.size(); i++)
					{
						if (_scripts[i]->category() != category)
							continue;
						ImGui::PushID(i);
						if (ImGui::MenuItem(Lang::l(_scripts[i]->description())))
						{
							auto mouse = ((ModCad*)module("MOD_CAD"))->mouse();
							if (_scripts[i]->has_properties_dialog())
							{
								// the script runs when the properties dialog box is validated
								if (_scripts[i]->begin_dialog(mouse, _document))
									_current_script = _scripts[i];
							}
							else
								insert(_scripts[i]->run(mouse, _document));
						}
						ImGui::PopID();
					}
					ImGui::EndMenu();
				}
				// the delivered scripts are copied again at each start: only the files of the user can be edited
				bool editable = false;
				for (auto script : _scripts)
					editable = editable || !script->read_only();

				ImGui::Separator();
				if (editable && ImGui::BeginMenu(Lang::l("SCRIPT_EDIT")))
				{
					for (auto script : _scripts)
					{
						ImGui::PushID(script);
						if (!script->read_only() && ImGui::MenuItem(Lang::l(script->description())))
						{
							auto m = module("MOD_SCRIPT");
							m->load(script->path());
							m->show(true);
							m->selected(true);
						}
						ImGui::PopID();
					}
					ImGui::EndMenu();
				}
				if (!_scripts.empty() && ImGui::BeginMenu(Lang::l("SCRIPT_CLONE")))
				{
					for (auto script : _scripts)
					{
						ImGui::PushID(script);
						if (ImGui::MenuItem(Lang::l(script->description())))
						{
							_clone_source = script->path();
							_clone_script = true;
							_clone_name = script->module() + "_custom";
							_clone_description = script->description();
							_clone_error = "";
							_display_clone = true;
						}
						ImGui::PopID();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem(Lang::l("SCRIPT_OPEN_FOLDER")))
					dialog::open_folder(environment::combine_path(environment::application_data_path(), "script"));
				//ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu(Lang::l("DISPLAY")))
		{
			if (_current_module->code() == "MOD_CAD")
			{
				((ModCad*)_current_module)->render_display_menu();
				ImGui::Separator();
			}
			if (ImGui::BeginMenu(Lang::l("DISPLAY_STYLE")))
			{

				if (ImGui::MenuItem(Lang::l("DISPLAY_CLASSIC"), NULL, config.display_style == 0))
				{
					ImGui::StyleColorsClassic();
					config.display_style = 0;
				}
				if (ImGui::MenuItem(Lang::l("DISPLAY_DARK"), NULL, config.display_style == 1))
				{
					ImGui::StyleColorsDark();
					config.display_style = 1;
				}
				if (ImGui::MenuItem(Lang::l("DISPLAY_LIGHT"), NULL, config.display_style == 2))
				{
					ImGui::StyleColorsLight();
					config.display_style = 2;
				}
				ImGui::EndMenu();

			}
			
			ImGui::Separator();

			Module* m = module("MOD_OUTPUT");
			if (ImGui::MenuItem(Lang::l("DISPLAY_OUTPUT"), NULL, m->show()))
			{
				m->show(!m->show());
				m->selected(m->show());
			}

			m = module("MOD_LOG");
			if (ImGui::MenuItem(Lang::l("DISPLAY_LOG"), NULL, m->show()))
			{
				m->show(!m->show());
				m->selected(m->show());
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(Lang::l("HELP")))
		{
			// the manual and the sources are on the web site of the project
			std::string manual = Lang::getName().substr(0, 2) == "fr" ? "fr" : "en";
			if (ImGui::MenuItem(Lang::l("HELP_MANUAL")))
				dialog::open_folder("https://github.com/rebisway/openpostpro/blob/main/docs/MANUAL." + manual + ".md");
			if (ImGui::MenuItem(Lang::l("HELP_SHORTCUTS")))
				_show_shortcuts = true;
			if (ImGui::MenuItem(Lang::l("HELP_WEBSITE")))
				dialog::open_folder("https://openpostpro.rebisway.com");
			if (ImGui::MenuItem(Lang::l("HELP_GITHUB")))
				dialog::open_folder("https://github.com/rebisway/openpostpro");
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("ABOUT")))
				_show_about = true;
			ImGui::EndMenu();
		}

#ifdef _DEBUG
		// development functions, not compiled in release builds
		if (ImGui::BeginMenu(Lang::l("DEBUG")))
		{
			if (ImGui::MenuItem(Lang::l("TEST")))
			{
				//Script::add_module_path(environment::application_path());
				//int value = get_value();
				//Logger::log("Value read " + std::to_string(value));
				//set_value(10);
				//value = get_value();
				//Logger::log("Value read " + std::to_string(value));
	
				//auto path = stringex::replace(environment::application_path() + "postcallback.dll", "\\", "/");
	
				//auto s = std::string("import sys\n") +
				//	std::string("import ctypes\n\n") +
				//	std::string("post = ctypes.CDLL(\"" + path + "\")\n") +
				//	std::string("post.set_value(30)");
	
				//PyObject* py_module = PyImport_ImportModule("ctypes");
				//PyObject* py_post = PyObject_CallMethod(py_module, "CDLL", "s", path.c_str());
				//PyObject* py_val = PyObject_CallMethod(py_post, "set_value", "i", 35);
	
				////Script::run_python(s);
				//s = Script::python_error();
	
				//value = get_value();
				//Logger::log("Value read " + std::to_string(value));
			}

			ImGui::Separator();

			auto cad = (ModCad*)module("MOD_CAD");
			if (ImGui::MenuItem(Lang::l("DRAWING_DEBUG_OFFSET")))
				cad->debug_offset();
			if (ImGui::MenuItem(Lang::l("DRAWING_DEBUG_CONVERT_TOOLPATH_TO_SPLINE")))
				cad->debug_convert_toolpath_to_spline();

			ImGui::EndMenu();
		}
#endif



		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	if (_show_shortcuts)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::Begin((Lang::t("HELP_SHORTCUTS") + "###SHORTCUTS_WINDOW").c_str(), &_show_shortcuts, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
		{
			static const std::pair<const char*, const char*> shortcuts[] = {
				{ "Ctrl+N", "FILE_NEW" }, { "Ctrl+O", "FILE_OPEN" }, { "Ctrl+S", "FILE_SAVE" }, { "Ctrl+Q", "FILE_EXIT" },
				{ "Ctrl+Z", "EDIT_UNDO" }, { "Ctrl+Y", "EDIT_REDO" }, { "Ctrl+X", "EDIT_CUT" }, { "Ctrl+C", "EDIT_COPY" },
				{ "Ctrl+V", "EDIT_PASTE" }, { "KEY_DEL", "EDIT_REMOVE" }, { "Ctrl+A", "EDIT_SELECT_ALL" },
				{ "KEY_HOME", "DISPLAY_FIT" }, { "F2", "SHORTCUT_RENAME" }, { "KEY_SPACE", "SHORTCUT_END_POLYLINE" }, { "KEY_ESC", "SHORTCUT_CANCEL" },
				{ "KEY_CTRL_CLICK", "SHORTCUT_CTRL_CLICK" }, { "KEY_SHIFT_CLICK", "SHORTCUT_SHIFT_CLICK" },
				{ "KEY_WHEEL", "SHORTCUT_ZOOM" }, { "KEY_RIGHT_BUTTON", "SHORTCUT_PAN" } };
			if (ImGui::BeginTable("##SHORTCUTS_TABLE", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
			{
				for (const auto& shortcut : shortcuts)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					// the names of the keys and of the mouse actions are translated, the combinations with Ctrl are not
					ImGui::TextUnformatted(strncmp(shortcut.first, "KEY_", 4) == 0 ? Lang::l(shortcut.first) : shortcut.first);
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(Lang::l(shortcut.second));
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	if (_show_about)
	{
		// the version is the one of the application, not the one of the document file format
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::Begin(Lang::l("ABOUT"), &_show_about, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
		{
			// each line is centered in the window
			auto centered = [](const std::string& text)
			{
				float width = ImGui::GetWindowSize().x;
				ImGui::SetCursorPosX((width - ImGui::CalcTextSize(text.c_str()).x) * 0.5f);
				ImGui::TextUnformatted(text.c_str());
			};

			if (_about_icon == nullptr)
			{
				GlTexture* texture = new GlTexture();
				texture->load("icon_128.png");
				_about_icon = texture;
			}
			if (_about_icon->id() != nullptr)
			{
				ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 96.0f) * 0.5f);
				ImGui::Image((ImTextureID)_about_icon->id(), ImVec2(96.0f, 96.0f));
			}

			centered("OpenPostPro");
			ImGui::NewLine();
			centered(Lang::t("VERSION") + " " + appinfo::version() + " - " + Lang::t("BUILD") + " " + std::to_string(appinfo::build()));
			centered(Lang::t("BUILD_DATE") + " " + appinfo::build_date() + " " + appinfo::build_time());
			ImGui::NewLine();
			centered("Copyright (C) 2024-2026 Thomas Gourgnier");
			centered("GNU Affero General Public License v3 or later");
			centered("https://openpostpro.rebisway.com");
			centered("https://github.com/rebisway/openpostpro");
			ImGui::NewLine();

			float button = ImGui::CalcTextSize(Lang::l("BTN_OK")).x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button) * 0.5f);
			if (ImGui::Button(Lang::l("BTN_OK")))
				_show_about = false;
		}
		ImGui::End();
	}
	else if (_about_icon != nullptr)
	{
		delete (GlTexture*)_about_icon;
		_about_icon = nullptr;
	}

	if (_display_first_run)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin(Lang::l("WELCOME"), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("%s", Lang::l("WELCOME_MESSAGE"));
		ImGui::NewLine();

		// the name of the language ("@lang" key of its file) then its code, as in the preferences: "Français (fr-fr)"
		auto language_label = [](const std::string& code)
		{
			std::string name = Lang::language_name(code);
			return name.empty() ? code : name + " (" + code + ")";
		};
		if (ImGui::BeginCombo(Lang::l("LANGUAGE"), language_label(_first_language).c_str()))
		{
			for (const std::string& code : Lang::get_langs())
			{
				const bool is_selected = (code == _first_language);
				if (ImGui::Selectable(language_label(code).c_str(), is_selected) && code != _first_language)
				{
					// the interface changes at once
					_first_language = code;
					Lang::load(code);
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		const char* units[] = { Lang::l("UNIT_MM"), Lang::l("UNIT_INCH") };
		ImGui::Combo(Lang::l("DEFAULT_UNIT"), &_first_unit, units, IM_ARRAYSIZE(units));

		ImGui::NewLine();
		if (ImGui::Button(Lang::l("OK")))
		{
			config.language = _first_language;
			config.default_unit = _first_unit;
			config.write();
			_document->unit(_first_unit == 1 ? Unit::Inch : Unit::Millimeter);
			_display_first_run = false;
		}
		ImGui::End();
	}

	if (_display_preferences)
	{
		auto result = config.drawUI();

		if (result == MessageBoxResult::Ok) // we apply preferences
		{
			for (Module* m : _modules)
				m->update();
		}

		_display_preferences = result == MessageBoxResult::None;
	}

	if (_display_postpro_preferences)
	{
		if (_current_postpro != nullptr)
		{
			if (_current_postpro->render_GUI())
			{
				_display_postpro_preferences = false;
			}
		}
	}

	if (_save_requested)
	{
		_save_requested = false;
		save_current();
	}
	if (_new_requested)
	{
		_new_requested = false;
		new_document();
	}
	if (_open_requested)
	{
		_open_requested = false;
		open_dialog();
	}

	if (_display_clone)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin((Lang::t(_clone_script ? "SCRIPT_CLONE" : "POST_CLONE") + "###CLONE").c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

		char name[64], description[128];
		snprintf(name, sizeof(name), "%s", _clone_name.c_str());
		snprintf(description, sizeof(description), "%s", _clone_description.c_str());

		ImGui::SetNextItemWidth(300.0f);
		if (ImGui::InputText(Lang::l("CLONE_NAME"), name, sizeof(name)))
			_clone_name = name;
		ImGui::SetNextItemWidth(300.0f);
		if (ImGui::InputText(Lang::l("CLONE_DESCRIPTION"), description, sizeof(description)))
			_clone_description = description;

		if (!_clone_error.empty())
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", Lang::l(_clone_error));

		ImGui::NewLine();
		if (ImGui::Button(Lang::l("OK")))
		{
			_clone_error = clone();
			if (_clone_error.empty())
				_display_clone = false;
		}
		ImGui::SameLine();
		if (ImGui::Button(Lang::l("CANCEL")))
			_display_clone = false;
		ImGui::End();
	}

	if (_current_script != nullptr)
	{
		std::string output;
		auto result = _current_script->render_GUI(output);
		if (result != MessageBoxResult::None)
		{
			_current_script = nullptr;
			if (result == MessageBoxResult::Ok)
				insert(output);
		}
	}
}

void Application::new_document()
{
	if (_mutex.try_lock())
	{
		try
		{
			_document->clear();
			_document->init();
			_document->path("");
			_document->unit(config.default_unit == 1 ? Unit::Inch : Unit::Millimeter);
			

			title(_application_title);

			for (int i = 0; i < _modules.size(); i++)
			{
				auto module = _modules[i];
				module->document(_document);
				module->clear_history();
			}
			_mutex.unlock();
		}
		catch (const std::exception& e)
		{
			_mutex.unlock();
			Logger::log(std::string("ERROR : ") + e.what());
		}
	}
}

void Application::load_file(std::string path, bool remember)
{
	if (std::filesystem::exists(path))
	{
		if (_mutex.try_lock())
		{
			try
			{
				auto t = std::chrono::high_resolution_clock::now();
				Logger::log("Reading file " + path);

				_document->path(path);

				// document affectation
				for (Module* m : _modules)
					m->document(_document);

				_document->read(file::read_all_text(path));

				// clearing history
				for (Module* m : _modules)
				{
					m->clear_history();
					m->document_loaded();
				}


				title(_application_title + " - " + _document->path());
				if (remember)
					add_recent(path);

				//onSizeChanged(width(), height());

				Logger::log("Read time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
				_mutex.unlock();
			}
			catch (const std::exception& e)
			{
				_mutex.unlock();
				Logger::log(std::string("ERROR : ") + e.what());
			}
		}
	}
	else
		new_document();
}

void Application::save_file(std::string path)
{
	if (_mutex.try_lock())
	{
		try
		{
			_document->path(path);
			auto data = _document->write(_version);
			file::write_all_text(path, data);

			title(_application_title + " - " + _document->path());
			add_recent(path);

			_mutex.unlock();
		}
		catch (const std::exception& e)
		{
			_mutex.unlock();
			Logger::log(std::string("ERROR : ") + e.what());
		}
	}
}

void Application::export_dxf(std::string path)
{
	// only the selected shapes if there are some, else the whole drawing
	bool selection = false;
	for (Graphic* g : _document->selected())
		selection = selection || dynamic_cast<Shape*>(g) != nullptr;
	if (DxfExport::write(_document, path, selection))
		Logger::log("DXF exported : " + path + (selection ? " (selection)" : ""));
	else
		Logger::log("ERROR : the DXF file can not be written : " + path);
}

void Application::load_dxf(std::string path)
{
	if (_mutex.try_lock())
	{
		try
		{
			auto t = std::chrono::high_resolution_clock::now();
			Logger::log("Reading file " + path);
			DxfLoader::read(path, _document);
			Logger::log("Read time (ms): " + std::to_string(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - t).count()));
			_document->check();

			// history is cleared
			for (Module* m : _modules)
				m->clear_history();

			title(_application_title + " - " + _document->path());

			_mutex.unlock();
		}
		catch (const std::exception& e)
		{
			_mutex.unlock();
			Logger::log(std::string("ERROR : ") + e.what());
		}
	}
}

Module* Application::module(std::string code)
{
	for (auto m : _modules)
		if (m->code() == code)
			return m;
	return nullptr;
}

void Application::insert(std::string content)
{
	auto cad = (ModCad * )module("MOD_CAD");

	auto lines = stringex::split(content, '\n');

	Logger::log("Mouse (" + std::to_string(cad->mouse().x) + ";" + std::to_string(cad->mouse().y) + ")");

	// the whole output of a script is undone in one step
	// log and layer commands do not change the drawing
	bool record = std::any_of(lines.begin(), lines.end(), [](const std::string& l) { return l.size() > 0 && l[0] != 'G' && l[0] != 'Y'; });
	if (record)
		History::begin_undo_record();
	cad->insert("Y");
	for (auto l : lines)
	{
		if (l.size() > 0)
		{
			Logger::log("Insert " + l);
			cad->insert(l);
		}
	}
	cad->insert("Y");
	if (record)
		History::end_undo_record();

}

