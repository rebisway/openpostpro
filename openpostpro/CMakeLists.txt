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

# the program and its resources end in bin/ (like x64/Release on Windows); the build tree stays apart
set_target_properties(openpostpro PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

foreach(module ${MODULES})
	target_include_directories(openpostpro PRIVATE ${SRC}/${module})
endforeach()

# Include order: our folders first (our dialog.h must win over /usr/include/dialog.h), but the GLFW header
# of src/third goes after the system one, which must win
target_compile_options(openpostpro PRIVATE
	"SHELL:-idirafter ${SRC}/third"
	"SHELL:-include ${SRC}/common/compat.h"
	"SHELL:-include ${SRC}/common/pch.h")
target_include_directories(openpostpro PRIVATE ${SRC}/third/stb ${Python3_INCLUDE_DIRS})

# Warnings off: the MSVC code gives thousands of them and hides the errors. Use -DOPP_WARNINGS=ON to see them.
option(OPP_WARNINGS "Show the GCC warnings" OFF)
if(NOT OPP_WARNINGS)
	target_compile_options(openpostpro PRIVATE -w)
endif()
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

# ---------------------------------------------------------------------------------------------
# Installation and Debian package: cmake --build ../build-linux && (cd ../build-linux && cpack -G DEB)
# The program finds its resources beside the executable (through /proc/self/exe, which follows
# the link of /usr/bin), so everything goes in /usr/lib/openpostpro.
# ---------------------------------------------------------------------------------------------
set(OPP_LIB lib/openpostpro)

install(TARGETS openpostpro RUNTIME DESTINATION ${OPP_LIB})
foreach(resource ${RESOURCES})
	string(REPLACE ":" ";" pair ${resource})
	list(GET pair 0 from)
	list(GET pair 1 to)
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${from}/ DESTINATION ${OPP_LIB}/${to}
		PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
endforeach()

# the example projects: the program looks for docs/examples beside itself (as in the release of Windows)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../docs/examples/ DESTINATION ${OPP_LIB}/docs/examples FILES_MATCHING PATTERN "*.opp")

install(CODE "
	file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")
	execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink ../${OPP_LIB}/openpostpro \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/openpostpro\")
")

install(FILES linux/openpostpro.desktop DESTINATION share/applications)
foreach(size 16 32 64 128 256)
	install(FILES textures/icon_${size}.png DESTINATION share/icons/hicolor/${size}x${size}/apps RENAME openpostpro.png)
endforeach()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE DESTINATION share/doc/openpostpro RENAME copyright)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../THIRD_PARTY_LICENSES.md DESTINATION share/doc/openpostpro)

# the version comes from src/common/version.h
file(STRINGS ${SRC}/common/version.h version_lines REGEX "^#define APP_VERSION_(MAJOR|MINOR|PATCH) ")
foreach(line ${version_lines})
	string(REGEX REPLACE "^#define (APP_VERSION_[A-Z]+) +([0-9]+).*" "\\1;\\2" pair "${line}")
	list(GET pair 0 key)
	list(GET pair 1 value)
	set(${key} ${value})
endforeach()

set(CPACK_PACKAGE_NAME "openpostpro")
set(CPACK_PACKAGE_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "Thomas Gourgnier <contact@rebisway.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "2D CAD/CAM with Python post-processors for CNC machines")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/rebisway/openpostpro")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Python 3.12 or later: the program uses its stable ABI and needs the standard library
# libgl1: the program loads libGL.so.1 at the start (dlopen), which the computed dependencies do not show
set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3 (>= 3.12), libgl1")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python3-tk, zenity | kdialog")
# After the package is made, a script makes it independent of the version of Python of the build system (needs patchelf):
# see linux/portable-python.sh
if(NOT CMAKE_VERSION VERSION_LESS 3.19)
	set(CPACK_OPP_LINUX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux)
	set(CPACK_POST_BUILD_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/linux/portable_python.cmake)
endif()
include(CPack)
