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
#include <filesystem>
#include <exception>

#include <imgui.h>

#include "lang.h"
#include <inifile.h>
#include <environment.h>
#include <config.h>
#include <renderer.h>
#include <logger.h>

#include "test_cad.h"
#include <../import/dxfloader.h>
#include <dialog.h>
#include <file.h>
#include <strings.h>
#include <history.h>
#include <script.h>


#include "mod_log.h"
#include "mod_output.h"
#include "mod_cad.h"
#include "mod_script.h"


Application::Application(std::string title) : Window(title)
{

}

Application::~Application()
{
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
	_modules.push_back(new ModLog(this));
	_modules.push_back(new ModScript(this));
	_current_module = _modules[0];
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
		ImGui::LoadIniSettingsFromDisk(io.IniFilename);
	}

	onSizeChanged(width(), height());

	// the first start asks for the language and the unit
	_display_first_run = config.first;
	_first_language = Lang::getName();
	_first_unit = config.default_unit;

	// Default document
	_document = new Document();
	_document->unit(config.default_unit == 1 ? Unit::Inch : Unit::Millimeter);
	if (config.open_last_file)
		load_file(config.last_file_path);
	else
		new_document();

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
			auto path = entry.path().string();
			if (entry.path().extension() == ".py" && !stringex::end_with(path, "postcore.py"))
			{
				Postpro* p = new Postpro();

				try {
					if (p->initialize(entry.path().string(), Lang::getName()))
					{
						_postpros.push_back(p);
						Logger::log("Loading post-processor " + entry.path().filename().string());
						p->finalize();
					}
					else
					{
						Logger::error("Loading post-processor " + entry.path().filename().string() + " -> " + std::string(p->error()));
						p->finalize();
						delete p;
					}
				}
				catch (const std::exception& e)
				{
					Logger::error("Loading post-processor " + entry.path().filename().string() + " -> " + std::string(e.what()));
				}
			}
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
			auto path = entry.path().string();
			if (entry.path().extension() == ".py" && !stringex::end_with(path, "cadcore.py"))
			{
				CadScript* p = new CadScript();

				try {
					if (p->initialize(entry.path().string(), Lang::getName()))
					{
						_scripts.push_back(p);
						Logger::log("Loading post-processor " + entry.path().filename().string());
						p->finalize();
					}
					else
					{
						Logger::error("Loading post-processor " + entry.path().filename().string() + " -> " + std::string(p->error()));
						p->finalize();
						delete p;
					}
				}
				catch (const std::exception& e)
				{
					Logger::error("Loading post-processor " + entry.path().filename().string() + " -> " + std::string(e.what()));
				}
			}
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
		auto height = ImGui::GetMainViewport()->Size.y - top;

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
	}

	render_menu();

	ImGui::PopFont();
}

void Application::render_menu()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 7;

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
				std::string path("");
				if (dialog::open_file_dialog("", path, "OpenPostPro", "*.opp"))
					load_file(path);
			}
			if (ImGui::MenuItem(Lang::l("FILE_SAVE"), "CTRL+S", false)) 
			{
				if (std::filesystem::exists(_document->path()))
					save_file(_document->path());
				else
				{
					std::string path = "", p;
					if (dialog::save_file_dialog("", path, "OpenPostPro", "*.opp"))
					{
						if (!stringex::end_with(path, ".opp"))
							path += ".opp";
						save_file(path);
					}
				}
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
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("FILE_EXIT"), "ALT+X")) 
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
				if (ImGui::MenuItem(Lang::l("EDIT_SELECT_REVERSE"), NULL, false))
				{
					_current_module->reverse_selection();
				}
				ImGui::EndMenu();
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
				_current_postpro->run(_document, config.output_path, std::to_string(_version));
				auto m = module("MOD_OUTPUT");
				m->show(true);
				m->selected(true);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(Lang::l("POST_CONFIG")))
			{
				_display_postpro_preferences = true;
			}
			if (ImGui::MenuItem(Lang::l("POST_EDIT")))
			{
				auto m = module("MOD_SCRIPT");
				m->load(_current_postpro->path());
				m->show(true);
				m->selected(true);
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
				for (int i = 0; i < _scripts.size(); i++)
				{
					if (ImGui::BeginMenu(_scripts[i]->category().c_str()))
					{
						if (ImGui::MenuItem(Lang::l(_scripts[i]->description())))
						{
							insert(_scripts[i]->run( ((ModCad*)module("MOD_CAD"))->mouse()));
						}
						ImGui::EndMenu();
					}
				}
				//ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu(Lang::l("TOOLS")))
		{
			if (ImGui::MenuItem(Lang::l("PREFERENCES")))
			{
				_display_preferences = true;
			}
			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu(Lang::l("DISPLAY")))
		{
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

			ImGui::Separator();

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
			ImGui::EndMenu();
		}



		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	if (_display_first_run)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin(Lang::l("WELCOME"), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("%s", Lang::l("WELCOME_MESSAGE"));
		ImGui::NewLine();

		if (ImGui::BeginCombo(Lang::l("LANGUAGE"), _first_language.c_str()))
		{
			for (const std::string& code : Lang::get_langs())
			{
				const bool is_selected = (code == _first_language);
				if (ImGui::Selectable(code.c_str(), is_selected) && code != _first_language)
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

void Application::load_file(std::string path)
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

			_mutex.unlock();
		}
		catch (const std::exception& e)
		{
			_mutex.unlock();
			Logger::log(std::string("ERROR : ") + e.what());
		}
	}
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
	for (auto l : lines)
	{
		if (l.size() > 0)
		{
			Logger::log("Insert " + l);
			cad->insert(l);
		}
	}

}

