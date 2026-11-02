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

// openpostpro.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <Logger.h>
#include <lang.h>
#include <environment.h>
#include "src/ui/application.h"
#include <strings.h>

int main()
{
	// needs to be initialized first
	environment::init();
	auto path = environment::application_data_path();

	//File::LogErrors = true;

	// needs to be opened before access
	Logger::start(path + environment::application_name_without_extension() + ".log");

	Logger::log("Exe path [" + environment::application_path() + "]");
	Logger::log("Data path [" + environment::application_data_path() + "]");
	Logger::log("User path [" + environment::user_path() + "]");
	Logger::log("Documents path [" + environment::user_documents() + "]");

	Lang::load("fr-fr");

	auto langs = Lang::get_langs();

	Logger::log("languages available : " + stringex::concat_strings(langs, " "));
	Logger::log("language selected : " + Lang::getName());

	Application application("OpenPostpro");

	application.initialize();
	application.run();
	application.finalize();

	Logger::stop();

	return EXIT_SUCCESS;
}
