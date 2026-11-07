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
#ifndef ENGINE_RENDERER_OPENGL
#define ENGINE_RENDERER_OPENGL

#include "renderer.h"
#include <map>

class OpenGlRenderer : public Renderer
{
private:
	struct Program {
		int id = -1;
		std::map<std::string, int> uniforms;

		Program() {}
		Program(int id) { this->id = id; }
	};

	Program _current_program;

	std::map<std::string, Program> _programs;

	int get_uniform_location(std::string name);
public:
#pragma region properties
	bool blend() override;
	void blend(bool state) override;
#pragma endregion

	int pr_points() override;
	int pr_lines() override;
	int pr_line_strip() override;
	int pr_triangles() override;
	int pr_triangle_strip() override;
	int pr_triangle_fan() override;


	void initialize_GLFW() override;
	void initialize() override;
	void finalize() override;
	
	void set_size(float width, float height) override;
	void clear(float r, float g, float b, float a) override;

	void load_program(std::string name, std::string directory) override;
	void use_program(std::string name) override;
	void render() override;

	void set_uniform(std::string name, int value) override;
	void set_uniform(std::string name, float value) override;
	void set_uniform(std::string name, double value) override;
	void set_uniform(std::string name, glm::vec2 value) override;
	void set_uniform(std::string name, glm::vec3 value) override;
	void set_uniform(std::string name, glm::vec4 value) override;
	void set_uniform(std::string name, glm::mat4 value) override;

	int get_uniform_int(std::string name) override;
	float get_uniform_float(std::string name) override;
	double get_uniform_double(std::string name) override;
	glm::vec2 get_uniform_vec2(std::string name) override;
	glm::vec3 get_uniform_vec3(std::string name) override;
	glm::vec4 get_uniform_vec4(std::string name) override;

	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override;
	void delete_buffer(Buffer* buffer) override;
	Texture* load_texture(std::string path) override;
	void delete_texture(Texture* texture) override;

	Texture* create_float_texture(int width, int height, const float* data) override;
	void update_float_texture(Texture* texture, int x, int y, int width, int height, const float* data, int row_length) override;
	void bind_texture(Texture* texture, int unit) override;
	void draw_arrays(int primitive, int count) override;
	void depth_test(bool state) override;

private:
	unsigned int _empty_vao = 0;		// the vertex array of the draws without buffer
};


class GlBuffer : public Buffer
{
private:
	unsigned int _vbo_id = 0;
	unsigned int _vao_id = 0;
	size_t	     _count = 0;
	size_t		 _size = 0;

public:
	GlBuffer() {}

	GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);
	GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);
	GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);

	~GlBuffer() override;

	size_t size() override { return _count; }

	void create(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);
	void create(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);
	void create(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL);

	void release();

	/// <summary>
	/// Update vertices in the buffer. Vertices arrays size must be the same as buffer size 
	/// </summary>
	/// <param name="vertices"></param>
	/// <param name="normales"></param>
	/// <param name="usage"></param>
	void update(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;
	void update(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;
	void update(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;

	/// <summary>
	/// Update if arrays have the same size or inferior, otherwize, recreate
	/// </summary>
	/// <param name="vertices"></param>
	/// <param name="normales"></param>
	/// <param name="usage"></param>
	void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;
	void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;
	void flush(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = NULL) override;

	void draw(int primitive) override;
	void draw(int primitive, int first, int count) override;
	void draw(int primitive, int count, int* indice) override;
};

class GlTexture : public Texture
{
public:
	GlTexture() {}
	~GlTexture() override;

	void load(std::string path) override;
};

#endif // !ENGINE_RENDERER_OPENGL
