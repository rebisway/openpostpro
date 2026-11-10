cmake_minimum_required(VERSION 3.16)
project(openpostpro LANGUAGES C CXX)

# Linux build (the Windows build uses openpostpro.sln). See docs/BUILD-LINUX.md.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Debug adds the Debug menu (the sources test _DEBUG), as in the Visual Studio project
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

option(OPP_PERMISSIVE "Downgrade some GCC errors to warnings (code written with MSVC)" ON)

find_package(Threads REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(OpenGL)
# libpython is linked, the code uses the stable ABI (Py_LIMITED_API 3.12), so any Python >= 3.12 works
find_package(Python3 3.12 REQUIRED COMPONENTS Development.Embed)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------------------------
# Third party code: no forced include, no warnings
# ---------------------------------------------------------------------------------------------
add_library(opp_third STATIC
	${SRC}/third/gl3w/GL/gl3w.c
	${SRC}/third/IMGUI/imgui.cpp
	${SRC}/third/IMGUI/imgui_draw.cpp
	${SRC}/third/IMGUI/imgui_tables.cpp
	${SRC}/third/IMGUI/imgui_widgets.cpp
	${SRC}/third/IMGUI/backends/imgui_impl_glfw.cpp
	${SRC}/third/IMGUI/backends/imgui_impl_opengl3.cpp
	${SRC}/third/IMGUI_third/ImGuiColorTextEdit/TextEditor.cpp)

target_include_directories(opp_third PUBLIC
	${SRC}/third/IMGUI
	${SRC}/third/IMGUI/backends
	${SRC}/third/IMGUI_third/ImGuiColorTextEdit
	${SRC}/third/gl3w
	${SRC}/third/GLM)
target_link_libraries(opp_third PUBLIC glfw ${CMAKE_DL_LIBS} Threads::Threads)
target_compile_options(opp_third PRIVATE -w)

# ---------------------------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------------------------
set(MODULES cad cam common export import postpro python script sim test ui)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/openpostpro.cpp)
foreach(module ${MODULES})
	file(GLOB module_sources CONFIGURE_DEPENDS ${SRC}/${module}/*.cpp)
	list(APPEND SOURCES ${module_sources})
endforeach()
list(REMOVE_ITEM SOURCES ${SRC}/common/pch.cpp)		# only creates the precompiled header of MSVC

add_executable(openpostpro ${SOURCES})

foreach(module ${MODULES})
	if(NOT module STREQUAL "common")		# common goes after the system folders, see below
		target_include_directories(openpostpro PRIVATE ${SRC}/${module})
	endif()
endforeach()

# Two traps with the include order:
# - src/common/strings.h hides <strings.h> of the C library (used by <cstring>): the folder goes after the system ones
# - src/third holds a GLFW header: the one of the system must win, so it also goes after
target_compile_options(openpostpro PRIVATE
	"SHELL:-idirafter ${SRC}/common"
	"SHELL:-idirafter ${SRC}/third"
	"SHELL:-include ${SRC}/common/compat.h"
	"SHELL:-include ${SRC}/common/pch.h")
target_include_directories(openpostpro PRIVATE ${Python3_INCLUDE_DIRS})

target_compile_options(openpostpro PRIVATE -Wall -Wno-unknown-pragmas -Wno-sign-compare -Wno-unused-variable -Wno-reorder)
if(OPP_PERMISSIVE)
	target_compile_options(openpostpro PRIVATE -fpermissive)
endif()

target_link_libraries(openpostpro PRIVATE opp_third glfw Freetype::Freetype Python3::Python Threads::Threads ${CMAKE_DL_LIBS})
if(TARGET OpenGL::GL)
	target_link_libraries(openpostpro PRIVATE OpenGL::GL)
endif()

# ---------------------------------------------------------------------------------------------
# Runtime resources, beside the executable (what copy_libs_output.bat does on Windows)
# ---------------------------------------------------------------------------------------------
set(RESOURCES
	lang:lang
	fonts:fonts
	src/shaders:shaders
	textures:textures
	postpro:postpro
	script:script
	simulator:simulator)

foreach(resource ${RESOURCES})
	string(REPLACE ":" ";" pair ${resource})
	list(GET pair 0 from)
	list(GET pair 1 to)
	add_custom_command(TARGET openpostpro POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_CURRENT_SOURCE_DIR}/${from} $<TARGET_FILE_DIR:openpostpro>/${to}
		VERBATIM)
endforeach()
