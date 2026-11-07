// RENDERER TEST - checks the OpenGL renderer on a real (hidden) OpenGL 3.3 core context: shaders, indexed drawing, clear, uniforms.
//
// What for:
//   The renderer is the base of everything which is drawn. Its errors do not stop the program: OpenGL reports them with an
//   error flag and draws nothing (a shader which does not compile or link, an indexed draw from a pointer which the core
//   profile forbids, a uniform of a type which needs OpenGL 4). They only show on some machines (macOS does not accept the
//   ES versions of the shaders). This test opens a hidden window with the same context as the application and checks them.
//
// What is checked:
//   - every program of the shaders folder compiles and links in GLSL 3.30 core, and has no ES version left;
//   - the clear color is applied by the first clear (read back from the framebuffer);
//   - an indexed draw (used by the outline of the anchors) gives no OpenGL error and draws (a pixel is read back);
//   - a uniform given as a double does not need OpenGL 4 (no error, the value is the one given).
// A machine without OpenGL 3.3 (a remote session without a graphic card) is reported as skipped, not as a failure.
//
// Build: tests/cpp/build_renderer_gl.bat   Run: tests/cpp/test_renderer_gl.exe   (exit code 0 = all checks pass)
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "renderer_opengl.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <set>

#ifndef SHADER_DIR
#error SHADER_DIR must be defined by the build script
#endif

static int g_fail = 0, g_run = 0;
#define CHECK(cond, msg) do { g_run++; if (!(cond)) { g_fail++; std::printf("  FAIL line %d: %s\n", __LINE__, msg); } } while (0)

static unsigned int gl_errors()
{
	unsigned int count = 0;
	while (glGetError() != GL_NO_ERROR && count < 100)
		count++;
	return count;
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);

	if (!glfwInit())
	{
		std::puts("skipped: glfwInit failed");
		return 0;
	}

	OpenGlRenderer renderer;
	renderer.initialize_GLFW();		// the hints of the application: 3.3 core, forward compatible
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(64, 64, "renderer test", nullptr, nullptr);
	if (window == nullptr)
	{
		std::puts("skipped: no OpenGL 3.3 core context on this machine");
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	if (gl3wInit() != 0)
	{
		std::puts("skipped: gl3w could not load OpenGL");
		glfwTerminate();
		return 0;
	}
	std::printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	glViewport(0, 0, 64, 64);

	std::puts("the shaders");
	std::set<std::string> names;
	for (const auto& entry : std::filesystem::directory_iterator(SHADER_DIR))
	{
		std::string ext = entry.path().extension().string();
		if (ext == ".vert" || ext == ".frag")
		{
			names.insert(entry.path().stem().string());

			std::ifstream in(entry.path());
			std::stringstream text;
			text << in.rdbuf();
			CHECK(text.str().find("#version 330 core") != std::string::npos, "the shader is in GLSL 3.30 core");
			CHECK(text.str().find("300 es") == std::string::npos, "no ES version");
		}
	}
	CHECK(names.size() >= 7, "the shaders of the application are there");
	for (const std::string& name : names)
	{
		renderer.load_program(name, SHADER_DIR);
		renderer.use_program(name);
		GLint program = 0, linked = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked)
			std::printf("  program %s did not link\n", name.c_str());
		CHECK(program != 0 && linked == GL_TRUE, "the program compiles and links");
	}
	CHECK(gl_errors() == 0, "no OpenGL error after the programs");

	std::puts("clear");
	renderer.clear(0.2f, 0.4f, 0.6f, 1.0f);
	unsigned char pixel[4] = { 0, 0, 0, 0 };
	glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	CHECK(std::abs((int)pixel[0] - 51) <= 1 && std::abs((int)pixel[1] - 102) <= 1 && std::abs((int)pixel[2] - 153) <= 1, "the first clear uses its color");

	std::puts("indexed draw");
	renderer.use_program("solid");
	renderer.set_uniform("projection", glm::mat4(1.0f));
	renderer.set_uniform("modelview", glm::mat4(1.0f));
	renderer.set_uniform("inner_color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	renderer.set_uniform("use_aa", 0);
	renderer.set_uniform("use_clip", 0);
	gl_errors();

	// a square in the middle of the window: 4 corners, 6 indices for the two triangles
	std::vector<glm::vec3> square = {
		glm::vec3(-0.5f, -0.5f, 0), glm::vec3(0.5f, -0.5f, 0), glm::vec3(-0.5f, 0.5f, 0), glm::vec3(0.5f, 0.5f, 0) };
	Buffer* buffer = renderer.create_buffer(square);
	int indices[6] = { 0, 1, 2, 2, 1, 3 };
	buffer->draw(renderer.pr_triangles(), 6, indices);
	CHECK(gl_errors() == 0, "an indexed draw gives no OpenGL error");
	glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	CHECK(pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50, "the indexed draw draws (the center is red)");
	glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	CHECK(!(pixel[0] > 200 && pixel[1] < 50), "and not outside of the square");
	renderer.delete_buffer(buffer);

	std::puts("uniforms");
	renderer.set_uniform("aa_factor", 0.5);		// a double
	CHECK(gl_errors() == 0, "a double uniform gives no OpenGL error");
	CHECK(std::abs(renderer.get_uniform_float("aa_factor") - 0.5f) < 0.0001f, "the value is the one given");
	CHECK(std::abs(renderer.get_uniform_double("aa_factor") - 0.5) < 0.0001, "and is read back as a double");

	glfwDestroyWindow(window);
	glfwTerminate();

	std::printf("%d checks, %d failed\n", g_run, g_fail);
	return g_fail == 0 ? 0 : 1;
}
