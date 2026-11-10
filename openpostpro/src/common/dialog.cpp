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

#include "dialog.h"
#include "strings.h"

#ifdef _WIN32
#include <Windows.h>
#include <shobjidl.h>
#include <shellapi.h>

namespace dialog
{
    bool open_file_dialog(std::string initial_path, std::string& result, std::string filter_name, std::string filter_pattern)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
            COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                auto fn = stringex::to_wide_char(filter_name);
                auto fp = stringex::to_wide_char(filter_pattern);
                COMDLG_FILTERSPEC rgSpec[] = { {fn.c_str(), fp.c_str()} };
                pFileOpen->SetFileTypes(1, rgSpec);
                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);


                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            result = stringex::to_string(pszFilePath);
                            //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }
        return SUCCEEDED(hr);
    }

    bool save_file_dialog(std::string initial_path, std::string& result, std::string filter_name, std::string filter_pattern)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
            COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileSaveDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                auto fn = stringex::to_wide_char(filter_name);
                auto fp = stringex::to_wide_char(filter_pattern);
                COMDLG_FILTERSPEC rgSpec[] = { {fn.c_str(), fp.c_str()} };
                pFileOpen->SetFileTypes(1, rgSpec);
                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);


                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            result = stringex::to_string(pszFilePath);
                            //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return SUCCEEDED(hr);
    }

    void open_folder(std::string path)
    {
        auto wide = stringex::to_wide_char(path);
        ShellExecuteW(NULL, L"open", wide.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}
#else
#include <cstdlib>
#include <cstdio>

namespace dialog
{
    namespace
    {
        // quote a text for a POSIX shell
        std::string shell_quote(const std::string& text)
        {
            std::string result = "'";
            for (char c : text)
            {
                if (c == '\'')
                    result += "'\\''";
                else
                    result += c;
            }
            return result + "'";
        }

        // run a command and keep its first output line; true if the command succeeded and printed something
        bool run(const std::string& command, std::string& result)
        {
            FILE* pipe = popen(command.c_str(), "r");
            if (pipe == nullptr)
                return false;

            std::string output;
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                output += buffer;

            int status = pclose(pipe);

            while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
                output.pop_back();

            if (status != 0 || output.empty())
                return false;

            result = output;
            return true;
        }

        // "*.nc;*.gcode" -> "*.nc *.gcode"
        std::string patterns(std::string pattern, char separator)
        {
            for (char& c : pattern)
                if (c == ';')
                    c = separator;
            return pattern;
        }

        bool has_command(const char* name)
        {
            std::string command = std::string("command -v ") + name + " >/dev/null 2>&1";
            return std::system(command.c_str()) == 0;
        }

        // zenity (GTK) first, then kdialog (KDE); false if no tool exists or the person cancelled
        bool file_dialog(bool save, const std::string& initial_path, std::string& result, const std::string& filter_name, const std::string& filter_pattern)
        {
            std::string command;

            if (has_command("zenity"))
            {
                command = "zenity --file-selection";
                if (save)
                    command += " --save --confirm-overwrite";
                if (!initial_path.empty())
                    command += " --filename=" + shell_quote(initial_path);
                if (!filter_pattern.empty())
                    command += " --file-filter=" + shell_quote(filter_name + " | " + patterns(filter_pattern, ' '));
                command += " 2>/dev/null";
            }
            else if (has_command("kdialog"))
            {
                command = std::string("kdialog ") + (save ? "--getsavefilename " : "--getopenfilename ");
                command += shell_quote(initial_path.empty() ? "." : initial_path);
                if (!filter_pattern.empty())
                    command += " " + shell_quote(patterns(filter_pattern, ' ') + " | " + filter_name);
                command += " 2>/dev/null";
            }
            else
                return false;

            return run(command, result);
        }
    }

    bool open_file_dialog(std::string initial_path, std::string& result, std::string filter_name, std::string filter_pattern)
    {
        return file_dialog(false, initial_path, result, filter_name, filter_pattern);
    }

    bool save_file_dialog(std::string initial_path, std::string& result, std::string filter_name, std::string filter_pattern)
    {
        return file_dialog(true, initial_path, result, filter_name, filter_pattern);
    }

    void open_folder(std::string path)
    {
#ifdef __APPLE__
        std::string command = "open \"" + path + "\" &";
#else
        std::string command = "xdg-open \"" + path + "\" &";
#endif
        std::system(command.c_str());
    }
}
#endif