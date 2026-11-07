@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /O2 /I"%SRC%\postpro" /I"%SRC%\third" /I"%SRC%\third\GLM" /Fo"%~dp0obj\\" /Fe"%~dp0test_travel_sort.exe" "%~dp0test_travel_sort.cpp" 2>&1
