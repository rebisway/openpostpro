@echo off
rem Loads the fonts of the system with FreeType, without any window.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
set FTLIB=%~dp0..\..\openpostpro\lib\freetype
cl /nologo /std:c++20 /EHsc /MD /O1 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE /I"%SRC%\common" /I"%SRC%\third" /I"%SRC%\third\GLM" /I"%SRC%\third\freetype" /Fo"%~dp0obj\\" /Fe"%~dp0test_fonts.exe" "%~dp0test_fonts.cpp" "%SRC%\common\font.cpp" "%SRC%\common\environment.cpp" "%SRC%\common\geometry.cpp" "%SRC%\common\strings.cpp" /link /LIBPATH:"%FTLIB%" freetype.lib shell32.lib 2>&1
