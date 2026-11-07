@echo off
rem The simulator core with the python interpreter of the application (the stable ABI: python3.lib, and python3.dll found by the PATH).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
set PYLIB=%~dp0..\..\openpostpro\lib\python
set SIMDIR=%~dp0..\..\openpostpro\simulator
cl /nologo /std:c++20 /EHsc /MD /O1 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE /DSIMULATOR_DIR=R\"(%SIMDIR%)\" /I"%SRC%\sim" /I"%SRC%\common" /I"%SRC%\cad" /I"%SRC%\cam" /I"%SRC%\ui" /I"%SRC%\python" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\IMGUI" /I"%SRC%\third\GLFW" /I"%SRC%\third\gl3w" /I"%SRC%\third\python\313" /Fo"%~dp0obj\\" /Fe"%~dp0test_sim_core.exe" "%~dp0test_sim_core.cpp" "%SRC%\sim\sim_geometry.cpp" "%SRC%\sim\sim_program.cpp" "%SRC%\sim\sim_player.cpp" "%SRC%\sim\sim_material.cpp" "%SRC%\sim\sim_translator.cpp" "%SRC%\sim\sim_loader.cpp" "%SRC%\python\script.cpp" "%SRC%\common\geometry.cpp" "%SRC%\common\logger.cpp" "%SRC%\common\lang.cpp" "%SRC%\common\strings.cpp" "%SRC%\common\environment.cpp" "%SRC%\common\history.cpp" "%SRC%\ui\config.cpp" "%SRC%\common\inifile.cpp" "%SRC%\common\file.cpp" "%SRC%\third\IMGUI\imgui.cpp" "%SRC%\third\IMGUI\imgui_draw.cpp" "%SRC%\third\IMGUI\imgui_tables.cpp" "%SRC%\third\IMGUI\imgui_widgets.cpp" /link /LIBPATH:"%PYLIB%" python3.lib 2>&1
