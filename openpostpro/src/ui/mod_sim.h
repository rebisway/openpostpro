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

// The simulator tab: loads a program (the output of the post-processor, or a file), has it translated into the simple commands of the
// simulator by a Python translator, and plays it on a block of material. All the work is done by the core of src/sim: this module
// shows it (the scene, the buttons, the text of the program) and passes the buttons on to it.
// The play, pause, step, back and speed buttons belong to the simulator, they do not depend on the commands of the program.

#pragma once
#ifndef _MODSIM_H
#define _MODSIM_H

#include "module.h"
#include <sim_view.h>
#include <sim_loader.h>

class ModSim : public Module
{
private:
	Renderer* _render = nullptr;
	SimView* _view = nullptr;

	SimProgram _program;
	SimPlayer _player;
	std::unique_ptr<SimModel> _model;			// the material: its method is chosen in the settings (SimModel::create)
	int _model_method = -1;						// the method of _model
	SimSettings _settings;
	SimTool _tool;
	PythonTranslator* _translator = nullptr;
	SimLoader _loader;

	std::vector<std::string> _lines;			// the text of the program
	std::vector<int> _line_level;				// for each line: -1 nothing, 0 information, 1 warning, 2 error
	std::string _name;							// the name of what is simulated
	std::string _translator_name = "gcode";		// the translator which is used
	bool _force = false;						// the translator of the list is used for every file, whatever its extension
	std::string _source_path;					// the file which is shown (its extension chooses the translator)
	std::string _chosen_translator = "gcode";		// the one which the person chose in the list: a file of a known format uses the translator of its format for once
	std::vector<std::string> _translators;		// the translators of the folder, refreshed when the list is opened
	std::string _text;							// the text which is simulated: translated again when the translator changes
	bool _text_from_document = false;
	bool _ready = false;						// the program is translated and the block is made
	bool _block_dirty = false;					// the settings changed: the block has to be made again

	// settings of the block and of the tool (edited in the panel)
	int _tool_shape = 0;						// SimToolShape
	float _tool_diameter = 6.0f;
	float _tool_corner = 1.0f;
	float _tool_angle = 60.0f;
	float _stock_top = 0.0f;
	float _stock_thickness = 0.0f;				// 0: from the depth of the program
	float _precision = 15.0f;
	double _real_time = 0;						// seconds of the computer spent to play the program (what is written in the log at its end)
	double _machined = 0;						// the length (mm) of the moves at a feed rate which were played
	double _cut_time = 0;						// ... of which in the model of the material
	bool _program_lights = false;				// the program lights the torch or the spindle (M3, torch command): a torch only cuts then
	size_t _cut_count = 0;						// the cuts made in the model
	int _pauses = 0;							// pauses of the player (button), and stops asked by the program (M0, M1), since the start
	int _program_stops = 0;
	bool _was_paused = false;
	bool _was_held = false;
	bool _end_logged = false;					// the summary of the end of the program was written
	int _method = 1;							// the method of the model of the material: the tiles (see SimModel::method_name)
	float _max_cells = 4.0f;					// the most cells of the grid of the material, in millions: a large block with a small tool is limited by it
	bool _optional_stop = true;
	bool _tool_from_group = true;				// the radius of the tool comes from the first group of the document, until it is changed

	// the buttons
	int _speed_index = 3;
	bool _full_speed = false;
	bool _show_settings = false;
	bool _show_program = true;

	// the mouse
	bool _rotating = false;
	bool _panning = false;
	double _last_time = 0;
	int _shown_line = -1;						// the line of the text which is at the player
	float _controls_height = 0;					// the height of the panel of the buttons, the scene is drawn under it
	bool _refit = false;						// the camera looks at the whole program at the next frame (the size of the scene is then known)

	void load_text(const std::string& text, const std::string& name, bool from_document);
	void load_file(const std::string& path);
	void load_output();
	void finish_loading();
	void build_block();
	void apply_tool();
	std::string translators_folder();
	void select_translator(const std::string& name);
	void use_translator(const std::string& name);
	void select_auto();
	void translator_for(const std::string& path);
	void update(double seconds);
	void sync_view();
	std::string state_text();

	void draw_controls();
	void draw_cards();
	void draw_overlays();
	void draw_program(float top, float width);
	static std::string format_time(double seconds);

public:
	ModSim(Window* window);
	~ModSim();

	void display(float left, float top, float width, float height) override;
	void render_frame() override {}
	void render_GUI() override;
	void render_menu() override;
	void scroll(double xdelta, double ydelta) override;
	void left_button_down() override;
	void right_button_down() override;
	void key_pressed(int key, int scancode, int action, int mods) override;
	void drop(std::vector<std::string> paths) override;
	void document_loaded() override;
	Renderer* render() override { return _render; }

	// loads the output of the post-processor of the document and shows the tab
	void simulate_output();

	// the commands of the test bridge: "sim load <file>", "sim output", "sim play", ... Returns false if the command is not known
	bool command(const std::vector<std::string>& words);

	// state, for the tests
	const SimPlayer& player() const { return _player; }
	const SimProgram& program() const { return _program; }
	const SimModel& model() const { return *_model; }
	bool ready() const { return _ready; }
};

#endif
