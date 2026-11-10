// Compatibility layer for the non-Windows toolchains: force included with -include by CMakeLists.txt,
// the same way pch.h is injected with /FI by the Visual Studio project, so the sources build unchanged.

#pragma once

#ifndef _WIN32

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>

// GCC does not include these transitively as much as MSVC does

// Windows "secure" sprintf on a char array
template <std::size_t N, typename... Args>
inline int sprintf_s(char (&buffer)[N], const char* format, Args... args)
{
	return std::snprintf(buffer, N, format, args...);
}

#endif
