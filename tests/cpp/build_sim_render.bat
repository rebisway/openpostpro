@echo off
rem Opens a hidden OpenGL window: the GLFW library of the application is used.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
set GLFWLIB=%~dp0..\..\openpostpro\lib\glfw
cl /nologo /std:c++20 /EHsc /MD /O1 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE /DSHADER_DIR=R\"(%SRC%\shaders)\" /I"%SRC%\sim" /I"%SRC%\common" /I"%SRC%\ui" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\gl3w" /I"%SRC%\third\GLFW" /I"%SRC%\third\stb" /Fo"%~dp0obj\\" /Fe"%~dp0test_sim_render.exe" "%~dp0test_sim_render.cpp" "%SRC%\sim\sim_geometry.cpp" "%SRC%\sim\sim_program.cpp" "%SRC%\sim\sim_player.cpp" "%SRC%\sim\sim_material.cpp" "%SRC%\sim\sim_model.cpp" "%SRC%\sim\sim_model_grid.cpp" "%SRC%\sim\sim_view.cpp" "%SRC%\ui\renderer_opengl.cpp" "%SRC%\ui\renderer.cpp" "%SRC%\ui\camera.cpp" "%SRC%\ui\orbit_cam.cpp" "%SRC%\third\gl3w\GL\gl3w.c" "%SRC%\common\logger.cpp" "%SRC%\common\file.cpp" "%SRC%\common\environment.cpp" "%SRC%\common\strings.cpp" /link /LIBPATH:"%GLFWLIB%" opengl32.lib glfw3.lib gdi32.lib user32.lib shell32.lib 2>&1
