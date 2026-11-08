@echo off
rem The models of the material are compared without any window (no OpenGL: the graphic part of the models is not used).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /MD /O2 /D_USE_MATH_DEFINES /I"%SRC%\sim" /I"%SRC%\ui" /I"%SRC%\common" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\gl3w" /Fo"%~dp0obj\\" /Fe"%~dp0test_sim_models.exe" "%~dp0test_sim_models.cpp" "%SRC%\sim\sim_material.cpp" "%SRC%\sim\sim_model.cpp" "%SRC%\sim\sim_model_grid.cpp" "%SRC%\sim\sim_model_tiles.cpp" 2>&1
