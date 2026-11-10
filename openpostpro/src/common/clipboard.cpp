// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "clipboard.h"


#if !defined(_WIN32) && !defined(linux) && !defined(__linux) && defined(__MACH__)

std::string Clipboard::text()
{
	return std::string();
}

void Clipboard::text(std::string value)
{
	_text = value;
}

#endif

#if defined(_WIN32)

#define _AMD64_

#include <windef.h>
#include <winuser.h>
#include <WinBase.h>

std::string Clipboard::text()
{
	std::string content = "";
	if (OpenClipboard(NULL))
	{
		auto handle = GetClipboardData(CF_TEXT);

		if (handle != NULL)
		{
			char* data = (char*)GlobalLock(handle);
			if (data != NULL)
			{
				content = data;
				GlobalUnlock(data);
			}
		}

		CloseClipboard();
	}

	return content;
}

void Clipboard::text(std::string value)
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();

		HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, (value.size() + 1) * sizeof(char));

		if (handle == NULL)
		{
			CloseClipboard();
			return;
		}

		char*  data = (char*)GlobalLock(handle);
		memcpy(data, value.c_str(), value.size() * sizeof(char));
		data[(value.size() + 1)] = 0;

		GlobalUnlock(data);

		SetClipboardData(CF_TEXT, handle);

		GlobalFree(handle);

		CloseClipboard();
	}
}

#endif

#if defined(__linux__)

#include <GLFW/glfw3.h>

// GLFW owns the clipboard of the window system (X11 or Wayland); the window is not needed for the text

std::string Clipboard::text()
{
	const char* content = glfwGetClipboardString(nullptr);
	return content != nullptr ? std::string(content) : std::string();
}

void Clipboard::text(std::string value)
{
	glfwSetClipboardString(nullptr, value.c_str());
}

#endif
