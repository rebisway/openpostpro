@echo off
rem The nesting engine only needs glm and parallel.h: no other source file.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /O2 /D_USE_MATH_DEFINES /I"%SRC%\cam" /I"%SRC%\common" /I"%SRC%\third" /I"%SRC%\third\GLM" /Fo"%~dp0obj\\" /Fe"%~dp0test_nesting.exe" "%~dp0test_nesting.cpp" "%SRC%\cam\nesting.cpp" 2>&1
