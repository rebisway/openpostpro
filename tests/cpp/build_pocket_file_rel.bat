@echo off
rem Release build of test_pocket_file, to time the calculation (build_pocket_file.bat is the Debug one, much slower, with the checks of the lists)
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%~dp0obj_rel" mkdir "%~dp0obj_rel"
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /MD /O2 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE /I"%SRC%\common" /I"%SRC%\cad" /I"%SRC%\cam" /I"%SRC%\postpro" /I"%SRC%\ui" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\IMGUI" /I"%SRC%\third\GLFW" /I"%SRC%\third\gl3w" /Fo"%~dp0obj_rel\\" /Fe"%~dp0test_pocket_file_rel.exe" "%~dp0test_pocket_file.cpp" "%SRC%\cam\pocket.cpp" "%SRC%\cam\toolpath.cpp" "%SRC%\cad\graphic.cpp" "%SRC%\common\curve.cpp" "%SRC%\common\geometry.cpp" "%SRC%\common\logger.cpp" "%SRC%\ui\renderer.cpp" "%SRC%\ui\camera.cpp" "%SRC%\common\lang.cpp" "%SRC%\common\strings.cpp" "%SRC%\common\environment.cpp" "%SRC%\common\history.cpp" "%SRC%\ui\config.cpp" "%SRC%\common\inifile.cpp" "%SRC%\common\file.cpp" "%SRC%\third\IMGUI\imgui.cpp" "%SRC%\third\IMGUI\imgui_draw.cpp" "%SRC%\third\IMGUI\imgui_tables.cpp" "%SRC%\third\IMGUI\imgui_widgets.cpp" 2>&1





