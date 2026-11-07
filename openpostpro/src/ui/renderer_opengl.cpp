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

#include "renderer_opengl.h"
#include <GL/gl3w.h>
#include "GLFW/glfw3.h"
#include <filesystem>
#include <environment.h>
#include <file.h>
#include <logger.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <strings.h>

int OpenGlRenderer::pr_points()
{
	return GL_POINTS;
}

int OpenGlRenderer::pr_lines()
{
	return GL_LINES;
}

int OpenGlRenderer::pr_line_strip()
{
	return GL_LINE_STRIP;
}

int OpenGlRenderer::pr_triangles()
{
	return GL_TRIANGLES;
}

int OpenGlRenderer::pr_triangle_strip()
{
	return GL_TRIANGLE_STRIP;
}

int OpenGlRenderer::pr_triangle_fan()
{
	return GL_TRIANGLE_FAN;
}

void OpenGlRenderer::initialize_GLFW()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // On veut OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To keep macOS happy; should not be necessary
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We do not want the legacy OpenGL
}

void OpenGlRenderer::initialize()
{
	bool err = gl3wInit() != 0;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	Renderer::initialize();
}

void OpenGlRenderer::finalize()
{
	for (auto program : _programs)
		glDeleteProgram(program.second.id);
	_programs.clear();

	Renderer::finalize();
}

void OpenGlRenderer::set_size(float width, float height)
{
	Renderer::set_size(width, height);

	glViewport(0, (int)_bottom, (int)width, (int)height);
}

void OpenGlRenderer::clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool OpenGlRenderer::blend() 
{
	GLboolean result = 0;
	glGetBooleanv(GL_BLEND, &result);
	return result != 0;
}

void OpenGlRenderer::blend(bool state)
{
	if (state)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
}

void OpenGlRenderer::load_program(std::string name, std::string directory)
{
	// load shaders from local shaders folder
	// vertex shader has '.vert' extension
	// fragment shadeer has '.frag' extension
	std::filesystem::path path(environment::application_path());
	path /= "shaders";
	// the folder given by the caller (the default one is the shaders folder next to the executable)
	if (!directory.empty())
		path = directory;

	if (std::filesystem::exists(path))
	{
		std::vector<int> shaders;
		int program = -1;
		if (_programs.count(name) == 0)
			_programs[name] = Program(glCreateProgram());
		program = _programs[name].id;

		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_regular_file()) {
				int type = -1;
				auto extension = entry.path().extension().string();
				auto file_name = entry.path().filename().string();
				file_name = file_name.substr(0, file_name.size() - extension.size());

				if (name == file_name)
				{
					if (extension == ".vert" || extension == ".vs")
					{
						type = GL_VERTEX_SHADER;
					}
					else if (extension == ".frag" || extension == ".fs")
					{
						type = GL_FRAGMENT_SHADER;
					}

					if (type != -1)
					{
						Logger::log(std::string("Compiling shader ") + entry.path().string());

						int shader_id = glCreateShader(type);

						std::string code = file::read_all_text(entry.path().string());
						const char* c_str = code.c_str();
						glShaderSource(shader_id, 1, &c_str, NULL);
						glCompileShader(shader_id);

						// check for python_error
						int result = 1;
						glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
						if (result == GL_FALSE)
						{
							GLint maxLength = GL_INFO_LOG_LENGTH;
							std::vector<GLchar> errorLog(GL_INFO_LOG_LENGTH);
							glGetShaderInfoLog(shader_id, GL_INFO_LOG_LENGTH, &maxLength, &errorLog[0]);

							Logger::error(errorLog.data());
						}
						else
						{
							glAttachShader(program, shader_id);
							shaders.push_back(shader_id);
						}
					}
				}
			}
		}

		glLinkProgram(program);

		// check for errors
		int result = 1;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		if (result == GL_FALSE)
		{
			GLint maxLength = GL_INFO_LOG_LENGTH;
			std::vector<GLchar> errorLog(GL_INFO_LOG_LENGTH);
			glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);

			Logger::log(std::string("ERROR(Shader compiling): ") + errorLog.data());
		}

		// free memory
		for (auto shader : shaders)
		{
			glDetachShader(program, shader);
			glDeleteShader(shader);
		}
	}
}

void OpenGlRenderer::use_program(std::string name)
{
	_current_program = _programs[name];
	glUseProgram(_current_program.id);
}

int OpenGlRenderer::get_uniform_location(std::string name)
{	
	int location = -1;
	if (_current_program.uniforms.find(name) == _current_program.uniforms.end())
	{
		_current_program.uniforms[name] = location = glGetUniformLocation(_current_program.id, name.data());
	}
	else
	{
		location = _current_program.uniforms[name];
	}
	return location;
}

void OpenGlRenderer::set_uniform(std::string name, int value)
{
	glUniform1i(get_uniform_location(name), value);
}

void OpenGlRenderer::set_uniform(std::string name, float value)
{
	glUniform1f(get_uniform_location(name), value);
}

void OpenGlRenderer::set_uniform(std::string name, double value)
{
	glUniform1f(get_uniform_location(name), (float)value);
}

void OpenGlRenderer::set_uniform(std::string name, glm::vec2 value)
{
	glUniform2f(get_uniform_location(name), value.x, value.y);
}

void OpenGlRenderer::set_uniform(std::string name, glm::vec3 value)
{
	glUniform3f(get_uniform_location(name), value.x, value.y, value.z);
}

void OpenGlRenderer::set_uniform(std::string name, glm::vec4 value)
{
	glUniform4f(get_uniform_location(name), value.x, value.y, value.z, value.a);
}

void OpenGlRenderer::set_uniform(std::string name, glm::mat4 value)
{
	glUniformMatrix4fv(get_uniform_location(name), 1, 0, (float*)& value);
}

int OpenGlRenderer::get_uniform_int(std::string name)
{
	int i = 0;
	glGetUniformiv(_current_program.id, get_uniform_location(name), &i);
	return i;
}

float OpenGlRenderer::get_uniform_float(std::string name)
{
	float f = 0;
	glGetUniformfv(_current_program.id, get_uniform_location(name), &f);
	return f;
}

double OpenGlRenderer::get_uniform_double(std::string name)
{
	float f = 0;
	glGetUniformfv(_current_program.id, get_uniform_location(name), &f);
	return f;
}

glm::vec2 OpenGlRenderer::get_uniform_vec2(std::string name)
{
	float f[2];
	glm::vec2 v = glm::vec2();
	glGetUniformfv(_current_program.id, get_uniform_location(name), f);
	return glm::vec2(f[0], f[1]);
}

glm::vec3 OpenGlRenderer::get_uniform_vec3(std::string name)
{
	float f[3];
	glm::vec3 v = glm::vec3();
	glGetUniformfv(_current_program.id, get_uniform_location(name), f);
	return glm::vec3(f[0], f[1], f[2]);
}

glm::vec4 OpenGlRenderer::get_uniform_vec4(std::string name)
{
	float f[4];
	glm::vec4 v = glm::vec4();
	glGetUniformfv(_current_program.id, get_uniform_location(name), f);
	return glm::vec4(f[0], f[1], f[2], f[3]);
}

Buffer* OpenGlRenderer::create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales, int usage)
{
	return new GlBuffer(vertices, normales, usage);
}

void OpenGlRenderer::delete_buffer(Buffer* buffer)
{
	delete (GlBuffer*)buffer;
}

void OpenGlRenderer::render()
{

}

Texture* OpenGlRenderer::load_texture(std::string path)
{
	GlTexture* texture = new GlTexture();
	texture->load(path);
	return texture;
}

void OpenGlRenderer::delete_texture(Texture* texture)
{
	delete (GlTexture*)texture;
}


GlBuffer::GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales, int usage) 
{
	create(vertices, normales, usage);
}


GlBuffer::GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales, int usage) 
{
	create(vertices, colors, normales, usage);
}

GlBuffer::GlBuffer(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales, int usage) 
{
	create(vertices, textures, normales, usage);
}

GlBuffer::~GlBuffer() 
{
	release();
}

void GlBuffer::create(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales, int usage)
{
	if (usage == NULL)
		usage = GL_STATIC_DRAW;

	_count = vertices.size();

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// get next buffer id (VBO)
	glGenBuffers(1, &_vbo_id);

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// Allocate video memory
	glBufferData(GL_ARRAY_BUFFER, vertices_len + normales_len, NULL, usage);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, NULL, vertices_len, vertices.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len, normales_len, normales.data());


	///////// setting the memory structure information for OpenGL ////////////////////

	// get next vertex array id (VAO)
	glGenVertexArrays(1, &_vao_id);

	// lock VAO
	glBindVertexArray(_vao_id);

	// memory structure
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	if (normales.size() > 0)
	{
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)vertices_len);
		glEnableVertexAttribArray(1);
	}

	// unlock VAO
	glBindVertexArray(0);

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::create(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales, int usage)
{
	if (usage == NULL)
		usage = GL_STATIC_DRAW;

	_count = vertices.size();

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t colors_len = colors.size() * glm::vec4::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// get next buffer id (VBO)
	glGenBuffers(1, &_vbo_id);

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// Allocate video memory : size of both arrays
	glBufferData(GL_ARRAY_BUFFER, vertices_len + colors_len + normales_len, 0, usage);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices_len, colors_len, colors.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, (vertices_len + colors_len), normales_len, normales.data());

	///////// setting the memory structure information for OpenGL ////////////////////

	// get next vertex array id (VAO)
	glGenVertexArrays(1, &_vao_id);

	// lock VAO
	glBindVertexArray(_vao_id);

	// memory structure
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, (void*)vertices_len);
	glEnableVertexAttribArray(1);

	if (normales.size() > 0)
	{
		glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, (void*)(vertices_len + colors_len));
		glEnableVertexAttribArray(2);
	}

	// unlock VAO
	glBindVertexArray(0);

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::create(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales, int usage)
{
	if (usage == NULL)
		usage = GL_STATIC_DRAW;

	_count = vertices.size();

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t textures_len = textures.size() * glm::vec2::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// get next buffer id (VBO)
	glGenBuffers(1, &_vbo_id);

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// Allocate video memory : size of both arrays
	glBufferData(GL_ARRAY_BUFFER, vertices_len + textures_len + normales_len, (void*)0, usage);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices_len, textures_len, textures.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len + textures_len, normales_len, normales.data());

	///////// setting the memory structure information for OpenGL ////////////////////

	// get next vertex array id (VAO)
	glGenVertexArrays(1, &_vao_id);

	// lock VAO
	glBindVertexArray(_vao_id);

	// memory structure
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, (void*)vertices_len);
	glEnableVertexAttribArray(1);

	if (normales.size() > 0)
	{
		glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, (void*)(vertices_len + textures_len));
		glEnableVertexAttribArray(2);
	}

	// unlock VAO
	glBindVertexArray(0);

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::release()
{
	glDeleteVertexArrays(1, &_vao_id);
	glDeleteBuffers(1, &_vbo_id);
}


void GlBuffer::update(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales, int usage) 
{
	if (vertices.size() > _count)
		throw std::runtime_error("ERROR - Buffer is too small for vertices");

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len, normales_len, normales.data());

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::update(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales, int usage) 
{
	if (vertices.size() > _count)
		throw std::runtime_error("ERROR - Buffer is too small for vertices");

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t colors_len = colors.size() * glm::vec4::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices_len, colors_len, colors.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len + colors_len, normales_len, normales.data());

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::update(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales, int usage) 
{
	if (vertices.size() > _count)
		throw std::runtime_error("ERROR - Buffer is too small for vertices");

	size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
	size_t textures_len = textures.size() * glm::vec2::length() * sizeof(float); // taille de vertices en octets
	size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

	/////// sending data to memory  ////////////////

	// lock buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

	// data transfert to video memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices_len, textures_len, textures.data());
	if (normales.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len + textures_len, normales_len, normales.data());

	// unlock VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlBuffer::flush(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales, int usage)
{
	if (vertices.size() <= _size)
	{
		_count = vertices.size();

		size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
		size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

		/////// sending data to memory  ////////////////

		// lock buffer
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

		// data transfert to video memory
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
		if (normales.size() > 0)
			glBufferSubData(GL_ARRAY_BUFFER, vertices_len, normales_len, normales.data());

		// unlock VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		release();
		create(vertices, normales, usage);
		_size = _count;
	}
}

void GlBuffer::flush(std::vector<glm::vec3> vertices, std::vector<glm::vec4> colors, std::vector<glm::vec3> normales, int usage)
{
	if (vertices.size() == _count)
	{
		_count = vertices.size();

		size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
		size_t colors_len = colors.size() * glm::vec4::length() * sizeof(float); // taille de vertices en octets
		size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

		/////// sending data to memory  ////////////////

		// lock buffer
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

		// data transfert to video memory
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len, colors_len, colors.data());
		if (normales.size() > 0)
			glBufferSubData(GL_ARRAY_BUFFER, vertices_len + colors_len, normales_len, normales.data());

		// unlock VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		release();
		create(vertices, colors, normales, usage);
		_size = _count;
	}
}

void GlBuffer::flush(std::vector<glm::vec3> vertices, std::vector<glm::vec2> textures, std::vector<glm::vec3> normales, int usage)
{
	if (vertices.size() == _count)
	{
		_count = vertices.size();

		size_t vertices_len = _count * glm::vec3::length() * sizeof(float); // taille de vertices en octets
		size_t textures_len = textures.size() * glm::vec2::length() * sizeof(float); // taille de vertices en octets
		size_t normales_len = normales.size() * glm::vec3::length() * sizeof(float); // taille de vertices en octets

		/////// sending data to memory  ////////////////

		// lock buffer
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);

		// data transfert to video memory
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len, vertices.data());
		glBufferSubData(GL_ARRAY_BUFFER, vertices_len, textures_len, textures.data());
		if (normales.size() > 0)
			glBufferSubData(GL_ARRAY_BUFFER, vertices_len + textures_len, normales_len, normales.data());

		// unlock VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		release();
		create(vertices, textures, normales, usage);
		_size = _count;
	}
}

void GlBuffer::draw(int primitive)
{
	glBindVertexArray(_vao_id);
	glDrawArrays(primitive, 0, (GLsizei)_count);
	glBindVertexArray(0);
}

void GlBuffer::draw(int primitive, int first, int count)
{
	glBindVertexArray(_vao_id);
	glDrawArrays(primitive, first, count);
	glBindVertexArray(0);
}


void GlBuffer::draw(int primitive, int count, int* indice)
{
	// the indices are few: a temporary element buffer
	GLuint ebo = 0;
	glGenBuffers(1, &ebo);

	glBindVertexArray(_vao_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)count * sizeof(unsigned int), indice, GL_STREAM_DRAW);
	glDrawElements(primitive, count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &ebo);
}


GlTexture::~GlTexture()
{
	glDeleteTextures(1, (GLuint*)id());
}

void GlTexture::load(std::string path)
{
	auto filename = path;
	
	if (!std::filesystem::exists(filename))
	{
		// get texture from textures folder
		filename = environment::combine_path(environment::combine_path(environment::application_path(), "textures"), path);
	}

	if (std::filesystem::exists(filename))
	{
		int width, height, n;
		GLuint texture = 0;
		bool alpha = stringex::end_with(filename, ".png");

		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &n, alpha ? STBI_rgb_alpha : 0);
		this->width(width);
		this->height(height);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? GL_RGBA : GL_RGB, width, height, 0, alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
			
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		
		glHint(GL_MIPMAP, GL_NICEST);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);

		id((void*)texture);
	}
}
