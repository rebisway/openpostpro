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
}
#endif