// OpenPostPro - www.openpostpro.org
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

/************************************************************************
* Static methods for common dialogs
*************************************************************************/

#pragma once
#pragma once
#ifndef _DIALOG_H
#define _DIALOG_H

#include <string>

namespace dialog
{
	bool open_file_dialog(std::string initial_path, std::string& result, std::string filter_name = "", std::string filter_pattern = "");
	bool save_file_dialog(std::string initial_path, std::string& result, std::string filter_name = "", std::string filter_pattern = "");
};
#endif