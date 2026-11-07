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

#pragma once
#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include <GLFW/glfw3.h>
#include <functional>
#include <string>
#include <vector>
//#include "renderer.h"
//#include "orthographic_camera.h"
#include <imgui.h>
#include <map>

enum Driver : int {
	OpengGL,
	Vulkan
};

class Window
{
private:
	GLFWwindow* _window = NULL;

	float _left;
	float _top;
	float _width;
	float _height;
	int _graphic_card;
	Driver _driver;
	bool _internal_camera = false;
	ImFont* _default_font = NULL;
	std::map<std::string, ImFont*> _fonts;

	std::string _title;
	std::string _imgui_ini;

#pragma region internal callback
	static void error_callback(int error, const char* description);
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void cursor_enter_callback(GLFWwindow* window, int entered);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void character_callback(GLFWwindow* window, unsigned int codepoint);
	static void drop_callback(GLFWwindow* window, int count, const char** paths);
#pragma endregion

#pragma region callback transfert
	void delegate_size_changed(float width, float height) { onSizeChanged(width, height); }
	void delegate_cursor_position(double xpos, double ypos) { onMouseMove(xpos, ypos); }
	void delegate_left_down() { onLeftButtonDown(); }
	void delegate_left_up() { onLeftButtonUp(); }
	void delegate_right_down() { onRightButtonDown(); }
	void delegate_right_up() { onRightButtonUp(); }
	void delegate_scroll(double xoffset, double yoffset) { onScroll(xoffset, yoffset); }
	void delegate_enter() { onEnter(); }
	void delegate_leave() { onLeave(); }
	void delegate_key(int key, int scancode, int action, int mods) { onKeyPressed(key, scancode, action, mods); }
	void delegate_character(unsigned int codepoint) { onCharacterInput(codepoint); }
	void delegate_drop(std::vector<std::string> paths) { onDrop(paths); }
#pragma endregion

protected:
#pragma region virtual methods
	virtual void onSizeChanged(float width, float height) {
		_width = width;
		_height = height;

		if (SizeChanged) SizeChanged(this, width, height);
	}

	virtual void onMouseMove(double xpos, double ypos) {
		if (MouseMove) MouseMove(this, xpos, ypos);
	}

	virtual void onLeftButtonDown() {
		if (MouseLeftDown) MouseLeftDown(this);
	}

	virtual void onLeftButtonUp() {
		if (MouseLeftUp) MouseLeftUp(this);
	}

	virtual void onRightButtonDown() {
		if (MouseRightDown) MouseRightDown(this);
	}

	virtual void onRightButtonUp() {
		if (MouseRightUp) MouseRightUp(this);
	}

	virtual void onScroll(double xoffset, double yoffset) {
		if (MouseScroll) MouseScroll(this, xoffset, yoffset);
	}

	virtual void onEnter() {
		if (MouseEnter) MouseEnter(this);
	}

	virtual void onLeave() {
		if (MouseLeave) MouseLeave(this);
	}

	virtual void onKeyPressed(int key, int scancode, int action, int mods) {
		if (KeyPressed) KeyPressed(this, key, scancode, action, mods);
	}

	virtual void onCharacterInput(unsigned int codepoint) {
		if (CharacterInput) CharacterInput(this, codepoint);
	}

	virtual void onDrop(std::vector<std::string> paths) {
		if (Drop) Drop(this, paths);
	}

	virtual void onRenderFrame() {
		if (RenderFrame) RenderFrame(this);
	}

	virtual void onRenderGUI() {
		if (RenderGUI) RenderGUI(this);
	}

	virtual void onClosing() {
		if (Closing) Closing(this);
	}
#pragma endregion

public:
#pragma region constructor/destructor
	Window(std::string title = "My Window");
	~Window();
#pragma endregion

#pragma region events
	std::function<void(Window*, float, float)> SizeChanged;
	std::function<void(Window*, double, double)> MouseMove;
	std::function<void(Window*)> MouseLeftDown;
	std::function<void(Window*)> MouseLeftUp;
	std::function<void(Window*)> MouseRightDown;
	std::function<void(Window*)> MouseRightUp;
	std::function<void(Window*, double, double)> MouseScroll;
	std::function<void(Window*)> MouseEnter;
	std::function<void(Window*)> MouseLeave;
	std::function<void(Window*, int, int, int, int)> KeyPressed;
	std::function<void(Window*, unsigned int)> CharacterInput;
	std::function<void(Window*, std::vector<std::string>)> Drop;
	std::function<void(Window*)> RenderFrame;
	std::function<void(Window*)> RenderGUI;
	std::function<void(Window*)> Initialised;
	std::function<void(Window*)> Finalized;
	std::function<void(Window*)> Closing;
#pragma endregion

#pragma region loop
	virtual void initialize();
	void run();
	virtual void finalize();
	virtual void doEvents();
#pragma endregion

#pragma region window properties
	float left() { return _left; }
	float left(float l) { if (_left != l) _left = l; glfwSetWindowPos(_window, (int)_left, (int)_top); }

	float top() { return _top; }
	float top(float t) { if (_top != t) _top = t; glfwSetWindowPos(_window, (int)_left, (int)_top); }

	float width() { return _width; }
	float width(float w) { if (_width != w) _width = w; glfwSetWindowSize(_window, (int)_width, (int)_height); }

	float height() { return _height; }
	float height(float h) { if (_height != h) _height = h; glfwSetWindowSize(_window, (int)_width, (int)_height); }

	std::string title() { return _title; }
	void title(std::string t) { _title = t; glfwSetWindowTitle(_window, _title.data()); }

	bool is_visible() { return glfwGetWindowAttrib(_window, GLFW_VISIBLE); }

	GLFWwindow* window() { return _window; }

#pragma endregion

	// Events given by the program (the test bridge) as if they came from the person: they go through the same callbacks as the
	// ones of GLFW (the interface first, then the modules)
	void simulate_cursor(double x, double y);
	void simulate_button(int button, int action, int mods);
	void simulate_scroll(double x, double y);
	void simulate_key(int key, int action, int mods);
	void simulate_char(unsigned int codepoint);

	void show();
	void hide();
	void close();

	void push_default_font() { ImGui::PushFont(_default_font); }
	void push_font(std::string name) { ImGui::PushFont(_fonts[name]); }
	void pop_font() { ImGui::PopFont(); }

};

#endif // !ENGINE_WINDOW_H
