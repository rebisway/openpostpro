// OpenPostPro - www.openpostpro.org
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

#include "window.h"
#include "logger.h"
#include "inifile.h"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "environment.h"
#include <filesystem>

#pragma region initialisation/finalization
Window::Window(std::string title)
{
	_title = title;
	
	IniFile ini;

	_left = ini.get_float(_title, "Left", 20);
	_top = ini.get_float(_title, "Top", 30);
	_width = ini.get_float(_title, "Width", 1024);
	_height = ini.get_float(_title, "Height", 768);
	_graphic_card = ini.get_int(_title, "Graphic", 0);
	_driver = (Driver)ini.get_int(_title, "Driver", Driver::OpengGL);

	Logger::log("Load initialization file");

	// initialize_python GLFW
	if (!glfwInit())
	{
		Logger::error("GLFW Initialization failed");
		exit(EXIT_FAILURE);
	}

	Logger::log("GLFW initialized");

	// set python_error callback which is global
	glfwSetErrorCallback(error_callback);

	// define the window as rezisable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	//_renderer->initialize_GLFW();

	// creating GLFW window
	// change width and height to force resize callback
	_window = glfwCreateWindow((int)(_width + 1), (int)(_height + 1), _title.data(), NULL, NULL);
	if (!_window)
	{
		Logger::error("GLFW Window or OpenGL context creation failed");
		exit(EXIT_FAILURE);
	}
	else
		Logger::log("GLFW Window created");

	// associate this object with the GLFW window
	glfwSetWindowUserPointer(_window, this);

	// register callbacks
	glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
	glfwSetKeyCallback(_window, key_callback);
	glfwSetCharCallback(_window, character_callback);
	glfwSetCursorPosCallback(_window, cursor_position_callback);
	glfwSetCursorEnterCallback(_window, cursor_enter_callback);
	glfwSetMouseButtonCallback(_window, mouse_button_callback);
	glfwSetScrollCallback(_window, scroll_callback);
	glfwSetDropCallback(_window, drop_callback);

	// make this window as the current context
	glfwMakeContextCurrent(_window);

	// set by default V-Sync to minimize processor load
	glfwSwapInterval(1);

	//_camera = NULL;

	Logger::log("GLFW parameters applied");
}

Window::~Window()
{
	IniFile ini;

	int w, h, l, t;

	glfwGetWindowSize(_window, &w, & h);
	glfwGetWindowPos(_window, &l, &t);

	_left = (float)l;
	_top = (float)t;
	_width = (float)w;
	_height = (float)h;

	ini.set(_title, "Left", _left);
	ini.set(_title, "Top", _top);
	ini.set(_title, "Width", _width);
	ini.set(_title, "Height", _height);
	ini.set(_title, "Graphic", _graphic_card);
	ini.set(_title, "Driver", _driver);

	ini.write();

	//delete _renderer;

	// close window and release context
	glfwDestroyWindow(_window);

	// release GLFW library
	glfwTerminate();

}

void Window::initialize()
{
	//_renderer->initialize_python();

	glfwSetWindowPos(_window, (int)_left, (int)_top);
	glfwSetWindowSize(_window, (int)_width, (int)_height);
	glfwShowWindow(_window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	IniFile ini;

	//_imgui_ini = Environment::application_data_path() + "imgui.ini";

	/*if (ini.get(title(), "First", "True") == "True")
	{
		ini.set(title(), "First", "False");
		IniFile im(_imgui_ini);
		im.set("Window][###PROJECT", "Pos", "11,62");
		im.set("Window][###PROJECT", "Size", "184,402");
		im.set("Window][###OBJECT", "Pos", "11,472");
		im.set("Window][###OBJECT", "Size", "186,93");
	}*/

	//io.IniFilename = _imgui_ini.c_str();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();

	_default_font = io.Fonts->AddFontDefault();

	auto path = environment::application_path() + "\\fonts\\" + "ProggyTiny.ttf";
	if (std::filesystem::exists(path))
		_fonts["properties"] = io.Fonts->AddFontFromFileTTF(path.c_str(), 10);
	else
		_fonts["properties"] = _default_font;

	if (Initialised) Initialised(this);
}

void Window::finalize()
{
	if (Finalized) Finalized(this);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Window::show() 
{ 
	glfwShowWindow(_window); 
}

void Window::hide() 
{ 
	glfwHideWindow(_window); 
}

void Window::close() 
{
	glfwSetWindowShouldClose(_window, GLFW_TRUE);
}


void Window::doEvents()
{
	glfwPollEvents();
}
#pragma endregion

#pragma region rendering loop
void Window::run()
{
	while (!glfwWindowShouldClose(_window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);

		onRenderFrame();


		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		onRenderGUI();

		// Rendering
		ImGui::Render();


		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(_window);
		glfwPollEvents();
	}

	onClosing();
}
#pragma endregion

#pragma region camera
//Camera* Window::camera()
//{
//	return _camera;
//}
//
//void Window::camera(Camera* c)
//{
//	if (_camera && _internal_camera)
//	{
//		delete _camera;
//		_internal_camera = false;
//	}
//	_camera = c;
//}
//
//void Window::orthographic()
//{
//	if (_camera && _internal_camera)
//	{
//		delete _camera;
//	}
//	_internal_camera = true;
//	_camera = new OrthographicCamera();
//}
//Texture* Window::load_texture(std::string path)
//{
//	return _renderer->load_texture(path);
//}
#pragma endregion

#pragma region internal callbacks
void Window::error_callback(int error, const char* description)
{
	Logger::log(std::string("ERROR: ") + description);
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	//if (w->_camera)
	//	w->_camera->set_size((float)width, (float)height);

	w->delegate_size_changed((float)width, (float)height);
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	//if (w->_camera)
	//	w->_camera->mouseMove((float)xpos, (float)ypos);

	w->delegate_cursor_position(xpos, ypos);
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.WantCaptureMouse)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			//if (w->_camera)
			//	w->_camera->mouseLeftDown();
			w->delegate_left_down();
		}
		else if (action == GLFW_RELEASE)
		{
			//if (w->_camera)
			//	w->_camera->mouseLeftUp();
			w->delegate_left_up();
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			//if (w->_camera)
			//	w->_camera->mouseRightDown();
			w->delegate_right_down();
		}
		else if (action == GLFW_RELEASE)
		{
			//if (w->_camera)
			//	w->_camera->mouseRightUp();
			w->delegate_right_up();
		}
	}
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	//if (w->_camera)
	//	w->_camera->mouseWheel((float)xoffset, (float)yoffset);

	w->delegate_scroll(xoffset, yoffset);
}

void Window::cursor_enter_callback(GLFWwindow* window, int entered)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	if (entered)
		w->delegate_enter();
	else
		w->delegate_leave();
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	w->delegate_key(key, scancode, action, mods);
}

void Window::character_callback(GLFWwindow* window, unsigned int codepoint)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	w->delegate_character(codepoint);
}

void Window::drop_callback(GLFWwindow* window, int count, const char** paths)
{
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	std::vector<std::string> data;

	for (int i = 0; i < count; i++)
		data.push_back(paths[i]);

	w->delegate_drop(data);
}
#pragma endregion
