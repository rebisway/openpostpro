@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /O1 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE /I"%SRC%\common" /I"%SRC%\cad" /I"%SRC%\cam" /I"%SRC%\ui" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\IMGUI" /I"%SRC%\third\GLFW" /I"%SRC%\third\gl3w" /Fo"%~dp0obj\\" /Fe"%~dp0test_cut_shapes.exe" "%~dp0test_cut_shapes.cpp" "%SRC%\cad\extend.cpp" "%SRC%\cad\extend_shape.cpp" "%SRC%\cad\cut_shape.cpp" "%SRC%\cad\line.cpp" "%SRC%\cad\arc.cpp" "%SRC%\cad\circle.cpp" "%SRC%\cad\ellipse.cpp" "%SRC%\cad\polyline.cpp" "%SRC%\cad\spline.cpp" "%SRC%\cad\shape.cpp" "%SRC%\cad\layer.cpp" "%SRC%\cad\anchor.cpp" "%SRC%\cad\graphic.cpp" "%SRC%\common\curve.cpp" "%SRC%\common\geometry.cpp" "%SRC%\common\logger.cpp" "%SRC%\ui\renderer.cpp" "%SRC%\ui\camera.cpp" "%SRC%\common\lang.cpp" "%SRC%\common\strings.cpp" "%SRC%\common\environment.cpp" "%SRC%\common\history.cpp" "%SRC%\ui\config.cpp" "%SRC%\common\inifile.cpp" "%SRC%\common\file.cpp" "%SRC%\third\IMGUI\imgui.cpp" "%SRC%\third\IMGUI\imgui_draw.cpp" "%SRC%\third\IMGUI\imgui_tables.cpp" "%SRC%\third\IMGUI\imgui_widgets.cpp" 2>&1







