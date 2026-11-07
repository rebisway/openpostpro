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

#include "bridge.h"

namespace bridge
{
	std::function<bool(const std::vector<std::string>&)> application;
}

#ifndef _DEBUG

// Release build: no bridge
void bridge::log(const std::string&) {}
void bridge::poll(Window*) {}
void bridge::after_render(Window*) {}

#else

#include <GL/gl3w.h>		// before GLFW, which would define the OpenGL types by its own way
#include "window.h"
#include <GLFW/glfw3.h>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace
{
	bool g_checked = false;
	bool g_enabled = false;
	std::filesystem::path g_folder;

	std::deque<std::vector<std::string>> g_queue;
	int g_waiting = 0;
	int g_done = 0;
	std::string g_screenshot;
	std::string g_status;
	double g_x = 0, g_y = 0;		// the position of the mouse, to press where it is

	std::string now()
	{
		auto t = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count() % 100000;
		char text[32];
		snprintf(text, sizeof(text), "%05lld", (long long)ms);
		return text;
	}

	void write_status(const std::string& value)
	{
		if (value == g_status)
			return;
		g_status = value;
		std::ofstream out(g_folder / "bridge.status", std::ios::trunc);
		out << value << "\n";
	}

	// the command and its arguments; the rest of the line (type, log) keeps its spaces
	std::vector<std::string> split(const std::string& line)
	{
		std::vector<std::string> words;
		std::istringstream in(line);
		std::string w;
		while (in >> w)
			words.push_back(w);
		if (!words.empty() && (words[0] == "type" || words[0] == "log"))
		{
			std::string rest = line.substr(line.find(words[0]) + words[0].size());
			if (!rest.empty() && rest[0] == ' ')
				rest.erase(0, 1);
			words.resize(1);
			words.push_back(rest);
		}
		return words;
	}

	int key_code(const std::string& name)
	{
		if (name.size() == 1)
		{
			char c = name[0];
			if (c >= 'a' && c <= 'z')
				return GLFW_KEY_A + (c - 'a');
			if (c >= 'A' && c <= 'Z')
				return GLFW_KEY_A + (c - 'A');
			if (c >= '0' && c <= '9')
				return GLFW_KEY_0 + (c - '0');
		}
		if (name == "enter") return GLFW_KEY_ENTER;
		if (name == "escape") return GLFW_KEY_ESCAPE;
		if (name == "space") return GLFW_KEY_SPACE;
		if (name == "tab") return GLFW_KEY_TAB;
		if (name == "delete") return GLFW_KEY_DELETE;
		if (name == "backspace") return GLFW_KEY_BACKSPACE;
		if (name == "left") return GLFW_KEY_LEFT;
		if (name == "right") return GLFW_KEY_RIGHT;
		if (name == "up") return GLFW_KEY_UP;
		if (name == "down") return GLFW_KEY_DOWN;
		if (name.size() >= 2 && name[0] == 'f')
		{
			int n = std::atoi(name.c_str() + 1);
			if (n >= 1 && n <= 12)
				return GLFW_KEY_F1 + (n - 1);
		}
		return -1;
	}

	int button_code(const std::vector<std::string>& c, size_t index)
	{
		return c.size() > index && c[index] == "right" ? GLFW_MOUSE_BUTTON_RIGHT : GLFW_MOUSE_BUTTON_LEFT;
	}

	void push_front(const std::vector<std::string>& command)
	{
		g_queue.push_front(command);
	}

	// the commands which take several frames are written as a list of simple ones
	void execute(Window* w, const std::vector<std::string>& c)
	{
		const std::string& name = c[0];
		auto number = [&](size_t i) { return c.size() > i ? std::atof(c[i].c_str()) : 0.0; };

		if (name == "wait")
		{
			g_waiting = (int)number(1);
		}
		else if (name == "size")
		{
			glfwSetWindowSize(w->window(), (int)number(1), (int)number(2));
			g_waiting = 2;
		}
		else if (name == "move")
		{
			g_x = number(1);
			g_y = number(2);
			w->simulate_cursor(g_x, g_y);
		}
		else if (name == "down")
		{
			w->simulate_button(button_code(c, 1), GLFW_PRESS, 0);
			g_waiting = 1;
		}
		else if (name == "up")
		{
			w->simulate_button(button_code(c, 1), GLFW_RELEASE, 0);
			g_waiting = 1;
		}
		else if (name == "click")
		{
			std::string button = c.size() > 3 ? c[3] : "left";
			push_front({ "up", button });
			push_front({ "down", button });
			push_front({ "wait", "1" });
			push_front({ "move", c.size() > 1 ? c[1] : "0", c.size() > 2 ? c[2] : "0" });
		}
		else if (name == "drag")
		{
			// the mouse moves by steps so that the application sees a movement
			double x1 = number(1), y1 = number(2), x2 = number(3), y2 = number(4);
			std::vector<std::vector<std::string>> steps;
			std::string button = c.size() > 5 ? c[5] : "left";
			steps.push_back({ "move", c[1], c[2] });
			steps.push_back({ "wait", "1" });
			steps.push_back({ "down", button });
			for (int i = 1; i <= 8; i++)
			{
				char sx[32], sy[32];
				snprintf(sx, sizeof(sx), "%g", x1 + (x2 - x1) * i / 8);
				snprintf(sy, sizeof(sy), "%g", y1 + (y2 - y1) * i / 8);
				steps.push_back({ "move", sx, sy });
				steps.push_back({ "wait", "1" });
			}
			steps.push_back({ "up", button });
			for (auto it = steps.rbegin(); it != steps.rend(); ++it)
				push_front(*it);
		}
		else if (name == "scroll")
		{
			w->simulate_scroll(0.0, number(1));
			g_waiting = 1;
		}
		else if (name == "key")
		{
			int key = key_code(c.size() > 1 ? c[1] : "");
			int mods = 0;
			for (size_t i = 2; i < c.size(); i++)
			{
				if (c[i] == "shift") mods |= GLFW_MOD_SHIFT;
				if (c[i] == "ctrl") mods |= GLFW_MOD_CONTROL;
				if (c[i] == "alt") mods |= GLFW_MOD_ALT;
			}
			if (key < 0)
				bridge::log("unknown key: " + (c.size() > 1 ? c[1] : std::string()));
			else
			{
				w->simulate_key(key, GLFW_PRESS, mods);
				w->simulate_key(key, GLFW_RELEASE, mods);
			}
			g_waiting = 1;
		}
		else if (name == "type")
		{
			for (unsigned char ch : (c.size() > 1 ? c[1] : std::string()))
				w->simulate_char(ch);
			g_waiting = 1;
		}
		else if (name == "screenshot")
		{
			g_screenshot = c.size() > 1 ? c[1] : "screenshot.png";
			// the picture is taken at the end of this frame: the next command waits for it
			g_waiting = 1;
		}
		else if (name == "log")
		{
			bridge::log(c.size() > 1 ? c[1] : "");
		}
		else if (name == "quit")
		{
			bridge::log("quit");
			w->close();
		}
		else
		{
			bool handled = false;
			if (bridge::application)
			{
				try
				{
					handled = bridge::application(c);
				}
				catch (const std::exception& e)
				{
					bridge::log(std::string("ERROR in the command: ") + e.what());
					handled = true;
				}
			}
			if (!handled)
				bridge::log("unknown command: " + name);
		}
	}

	void read_commands()
	{
		auto file = g_folder / "commands.txt";
		if (!std::filesystem::exists(file))
			return;

		std::ifstream in(file);
		std::string line;
		int count = 0;
		while (std::getline(in, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			auto start = line.find_first_not_of(" \t");
			if (start == std::string::npos || line[start] == '#')
				continue;
			g_queue.push_back(split(line.substr(start)));
			count++;
		}
		in.close();

		auto done = g_folder / "commands.done";
		std::error_code ec;
		std::filesystem::remove(done, ec);
		std::filesystem::rename(file, done, ec);
		bridge::log("read " + std::to_string(count) + " commands");
	}
}

void bridge::log(const std::string& text)
{
	if (!g_enabled)
		return;
	std::ofstream out(g_folder / "bridge.log", std::ios::app);
	out << now() << " " << text << "\n";
}

void bridge::poll(Window* window)
{
	if (!g_checked)
	{
		g_checked = true;
		char* value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, "OPP_BRIDGE") == 0 && value != nullptr && value[0] != 0)
		{
			g_folder = value;
			std::error_code ec;
			std::filesystem::create_directories(g_folder, ec);
			g_enabled = std::filesystem::exists(g_folder);
		}
		free(value);
		if (g_enabled)
		{
			log("bridge active, folder " + g_folder.string());
			write_status("idle 0");
		}
	}
	if (!g_enabled)
		return;

	if (g_waiting > 0)
	{
		g_waiting--;
		return;
	}

	if (g_queue.empty())
		read_commands();

	while (!g_queue.empty() && g_waiting == 0)
	{
		write_status("busy");
		auto command = g_queue.front();
		g_queue.pop_front();
		execute(window, command);
		g_done++;
	}

	if (g_queue.empty() && g_waiting == 0 && g_screenshot.empty())
		write_status("idle " + std::to_string(g_done));
}

void bridge::after_render(Window* window)
{
	if (!g_enabled || g_screenshot.empty())
		return;

	int width = 0, height = 0;
	glfwGetFramebufferSize(window->window(), &width, &height);
	std::vector<unsigned char> pixels((size_t)width * height * 4);
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	// the origin of OpenGL is at the bottom
	std::vector<unsigned char> flipped(pixels.size());
	for (int y = 0; y < height; y++)
		std::copy(pixels.begin() + (size_t)(height - 1 - y) * width * 4, pixels.begin() + (size_t)(height - y) * width * 4, flipped.begin() + (size_t)y * width * 4);
	for (size_t i = 3; i < flipped.size(); i += 4)
		flipped[i] = 255;

	auto path = g_folder / g_screenshot;
	int ok = stbi_write_png(path.string().c_str(), width, height, 4, flipped.data(), width * 4);
	log("screenshot " + path.string() + " " + std::to_string(width) + "x" + std::to_string(height) + (ok ? " ok" : " FAILED"));
	g_screenshot.clear();
}

#endif
