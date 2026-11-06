@echo off
rem Builds tests\cpp\test_empty_inputs.exe with MSVC (set VCVARS if Visual Studio lives elsewhere).
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul || exit /b 1
set SRC=%~dp0..\..\openpostpro\src
cl /nologo /std:c++20 /EHsc /O2 /D_USE_MATH_DEFINES /I"%SRC%\common" /I"%SRC%\third" /I"%SRC%\third\GLM" /Fo"%~dp0obj\\" /Fe"%~dp0test_empty_inputs.exe" "%~dp0test_empty_inputs.cpp" "%SRC%\common\curve.cpp" "%SRC%\common\geometry.cpp" "%SRC%\common\logger.cpp" || exit /b 1
