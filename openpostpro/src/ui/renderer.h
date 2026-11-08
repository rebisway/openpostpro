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
#ifndef ENGINE_RENDERER_H
#define ENGINE_RENDERER_H
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "camera.h"

class Buffer;
class Texture;

class Renderer
{
private:
	float _width = 0;
	float _height = 0;
	Camera* _camera = NULL;

public:
#pragma region constructor/destructor
	Renderer();
	~Renderer();
#pragma endregion

#pragma region properties
	float width() { return _width; }
	float height() { return _height; }
	Camera* camera() { return _camera; }
	void camera(Camera* value) { _camera = value; }
	virtual bool blend() { return false; }
	virtual void blend(bool state) {}

	virtual int pr_points() { return 0; }
	virtual int pr_lines() { return 0; }
	virtual int pr_line_strip() { return 0; }
	virtual int pr_triangles() { return 1; }
	virtual int pr_triangle_strip() { return 2; }
	virtual int pr_triangle_fan() { return 3; }
#pragma endregion

	virtual void initialize_GLFW() {}
	virtual void initialize();
	virtual void finalize() {}

	virtual bool is_available() { return true; }

	virtual void set_size(float width, float height) { _width = width; _height = height; }

	// space left under the drawing area (docked panel), used as the origin of the viewport
	float _bottom = 0.0f;
	virtual void set_bottom(float value) { _bottom = value; }
	virtual void clear(float r, float g, float b, float a) {}

	void load_programs();
	virtual void load_program(std::string name, std::string directory) {}
	virtual void use_program(std::string name) {}
	virtual void render() {}

	virtual void set_uniform(std::string name, int value) {}
	virtual void set_uniform(std::string name, float value) {}
	virtual void set_uniform(std::string name, double value) {}
	virtual void set_uniform(std::string name, glm::vec2 value) {}
	virtual void set_uniform(std::string name, glm::vec3 value) {}
	virtual void set_uniform(std::string name, glm::vec4 value) {}
	virtual void set_uniform(std::string name, glm::mat4 value) {}

	virtual int get_uniform_int(std::string name) { return 0; }
	virtual float get_uniform_float(std::string name) { return 0; }
	virtual double get_uniform_double(std::string name) { return 0; }
	virtual glm::vec2 get_uniform_vec2(std::string name) { return glm::vec2(); }
	virtual glm::vec3 get_uniform_vec3(std::string name) { return glm::vec3(); }
	virtual glm::vec4 get_uniform_vec4(std::string name) { return glm::vec4(); }

	virtual Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) { return NULL;  }
	virtual void delete_buffer(Buffer* buffer) {}
	virtual Texture* load_texture(std::string path) { return NULL; }
	// An image in memory: 4 bytes (red, green, blue, alpha) for each pixel, filtered. For the images which the application draws itself
	// (the glyphs of the table of characters)
	virtual Texture* create_rgba_texture(int width, int height, const unsigned char* data) { return NULL; }
	virtual void delete_texture(Texture* texture) {}

	// A texture of one floating point number for each pixel (a map of heights, read by a shader with texelFetch). It is not filtered.
	// `data` can be NULL (the texture is then filled with zero)
	virtual Texture* create_float_texture(int width, int height, const float* data) { return NULL; }
	// Replaces a rectangle of the texture. `data` points to the first number of the rectangle, `row_length` is the number of
	// numbers between two rows of the source (the width of the whole map when the rectangle is a part of it)
	virtual void update_float_texture(Texture* texture, int x, int y, int width, int height, const float* data, int row_length) {}
	// Binds the texture to a unit, the shader reads it with a sampler2D which has this unit as its value
	virtual void bind_texture(Texture* texture, int unit) {}

	// Draws vertices which the vertex shader makes from gl_VertexID: nothing is read from a buffer
	virtual void draw_arrays(int primitive, int count) {}

	// The test of depth, for the scenes in 3D
	virtual void depth_test(bool state) {}
};

class Buffer
{
public:
	Buffer() {}
	Buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	Buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	Buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}

	virtual ~Buffer() {}

	virtual size_t size() { return 0; }

	virtual void update(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	virtual void update(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	virtual void update(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}

	virtual void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	virtual void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}
	virtual void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) {}

	virtual void draw(int primitive) {}
	virtual void draw(int primitive, int first, int count) {}
	virtual void draw(int primitive, int count, int* indice) {}
};


class Texture
{
private:
	void* _id = NULL;
	int _width = 0;
	int _height = 0;

public:
	void* id() { return _id; }
	void id(void* texture) { _id = texture; }
	int width() { return _width; }
	void width(int w) { _width = w; }
	int height() { return _height; }
	void height(int h) { _height = h; }


	Texture() {}
	virtual ~Texture() {}

	virtual void load(std::string path) {}
};
#endif // !ENGINE_RENDERER_H

