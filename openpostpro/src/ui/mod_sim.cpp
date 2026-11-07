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

#include "mod_sim.h"
#include "renderer_opengl.h"
#include "group.h"
#include "bridge.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <lang.h>
#include <logger.h>
#include <file.h>
#include <dialog.h>
#include <strings.h>
#include <config.h>
#include <environment.h>
#include <filesystem>
#include <algorithm>
#include <cstdio>

static const double SPEEDS[] = { 1, 2, 5, 10, 20, 50, 100, 500 };
static const int SPEED_COUNT = (int)(sizeof(SPEEDS) / sizeof(SPEEDS[0]));

ModSim::ModSim(Window* window) : Module(window)
{
	_index = 3;
	_title = Lang::l("SIM_TITLE");
	_code = "MOD_SIM";
	_show = true;

	// its own renderer, as the drawing module has
	_render = new OpenGlRenderer();
	_render->initialize();
	_render->initialize_GLFW();
	_view = new SimView(_render);

	// the translator which was chosen the last time
	_translator_name = config.simulation_translator.empty() ? "gcode" : config.simulation_translator;
	_translator = new PythonTranslator(_translator_name);
	_player.on_move = [this](const SimMove& m)
	{
		// a torch only cuts while it is lit (M3 or the torch command); a cutter takes the material away whatever the spindle does
		if (_tool.shape != SimToolShape::Plasma || m.spindle != 0 || m.torch)
			_material.cut(m.from, m.to, _tool);
		_view->add_move(m);
	};
	_player.on_reset = [this]()
	{
		_material.reset();
		_view->clear_paths();
	};
	apply_tool();
}

ModSim::~ModSim()
{
	delete _view;
	delete _translator;
	delete _render;
}

std::string ModSim::translators_folder()
{
	return environment::combine_path(environment::application_data_path(), "simulator");
}

void ModSim::select_translator(const std::string& name)
{
	if (name == _translator_name)
		return;
	_loader.cancel();
	delete _translator;
	_translator_name = name;
	_translator = new PythonTranslator(_translator_name);

	// saved with the settings of the application when it is closed
	config.simulation_translator = _translator_name;

	// the program which is shown is translated again
	if (!_text.empty())
		load_text(_text, _name, _text_from_document);
}

void ModSim::apply_tool()
{
	_tool.shape = (SimToolShape)_tool_shape;
	_tool.radius = std::max(0.05, (double)_tool_diameter / 2.0);
	_tool.small_radius = _tool_shape == (int)SimToolShape::Torus ? _tool_corner : (_tool_shape == (int)SimToolShape::Cone ? 0.0 : 0.0);
	_tool.angle = _tool_angle;
	_tool.length = std::max(15.0, _tool.radius * 6.0);
}

void ModSim::display(float left, float top, float width, float height)
{
	Module::display(left, top, width, height);
	_render->set_bottom((std::max)(0.0f, _window->height() - top - height));
	_render->set_size(width, height);
}

void ModSim::document_loaded()
{
	// a new document: the radius of the tool is the one of its first group again
	_tool_from_group = true;
}

// ---------------------------------------------------------------------------------------------- loading
void ModSim::load_file(const std::string& path)
{
	if (!std::filesystem::exists(path))
	{
		Logger::error("Simulation: file [" + path + "] not found");
		return;
	}
	double start = glfwGetTime();
	std::string text = file::read_all_text(path);
	Logger::log("Simulation: file read in " + std::to_string((int)((glfwGetTime() - start) * 1000)) + " ms");
	load_text(text, std::filesystem::path(path).filename().string(), false);
}

void ModSim::simulate_output()
{
	load_output();
}

void ModSim::load_output()
{
	if (_document == nullptr || _document->output().empty() || !std::filesystem::exists(_document->output()))
	{
		Logger::error(Lang::t("SIM_ERR_NO_OUTPUT"));
		return;
	}
	load_text(file::read_all_text(_document->output()), std::filesystem::path(_document->output()).filename().string(), true);
}

void ModSim::load_text(const std::string& text, const std::string& name, bool from_document)
{
	_loader.cancel();
	_ready = false;
	_name = name;
	_text = text;
	_text_from_document = from_document;
	double start = glfwGetTime();
	_lines = SimLoader::split_lines(text);
	Logger::log("Simulation: " + std::to_string(_lines.size()) + " lines split in " + std::to_string((int)((glfwGetTime() - start) * 1000)) + " ms");
	_line_level.assign(_lines.size(), -1);
	_shown_line = -1;
	_player.program(nullptr);
	_program.clear();

	// the program of the document: the radius of the tool is the one of its first group, until the person sets it
	if (from_document && _tool_from_group && _document != nullptr && !_document->groups().empty() && _document->groups()[0]->tool_radius() > 0)
	{
		_tool_diameter = (float)(_document->groups()[0]->tool_radius() * 2.0);
		apply_tool();
	}

	std::map<std::string, double> options;
	start = glfwGetTime();
	if (!_loader.begin(_translator, &_program, text, options))
	{
		Logger::error("Simulation: " + _loader.error());
		return;
	}
	Logger::log("Simulation: translator started in " + std::to_string((int)((glfwGetTime() - start) * 1000)) + " ms");
	Logger::log("Simulation: translating " + name + " (" + std::to_string(_lines.size()) + " lines) with the translator " + _translator_name);
}

void ModSim::finish_loading()
{
	_settings.optional_stop = _optional_stop;
	_program.analyze(_settings);

	// the messages of the translator, with their line
	int errors = 0, warnings = 0;
	for (const SimCommand& c : _program.commands())
	{
		if (c.type != SimCommandType::Message)
			continue;
		int level = (int)c.level;
		if (c.line >= 1 && c.line <= (int)_line_level.size())
			_line_level[c.line - 1] = std::max(_line_level[c.line - 1], level);
		std::string where = c.line > 0 ? " (line " + std::to_string(c.line) + ")" : "";
		if (c.level == SimLevel::Error) { errors++; Logger::error("Simulation: " + c.text + where); }
		else if (c.level == SimLevel::Warning) { warnings++; Logger::log("Simulation, warning: " + c.text + where); }
		else Logger::log("Simulation: " + c.text + where);
	}
	if (_program.stats().unknown > 0)
		Logger::log("Simulation: " + std::to_string(_program.stats().unknown) + " command(s) of the translator are not known by the simulator and were ignored");

	Logger::log("Simulation: " + std::to_string(_program.size()) + " commands, " + format_time(_program.stats().total_time) +
		", " + std::to_string(errors) + " error(s), " + std::to_string(warnings) + " warning(s)");

	build_block();
}

// The block: around the moves which cut, as deep as the program goes
void ModSim::build_block()
{
	apply_tool();
	const SimStats& s = _program.stats();

	glm::dvec3 low = s.has_cut ? s.cut_low : s.low;
	glm::dvec3 high = s.has_cut ? s.cut_high : s.high;
	double margin = _tool.radius * 2.0 + 2.0;
	glm::dvec2 corner_low(low.x - margin, low.y - margin), corner_high(high.x + margin, high.y + margin);

	double top = _stock_top;
	double deepest = std::max(0.0, top - low.z);
	double thickness = _stock_thickness > 0.0f ? _stock_thickness : std::max(3.0, deepest * 1.25 + 1.0);
	double bottom = top - thickness;

	double step = SimMaterial::choose_step(corner_high.x - corner_low.x, corner_high.y - corner_low.y, _tool.radius, _precision, (int)(_max_cells * 1e6f));
	_material.setup(corner_low, corner_high, top, bottom, step);
	_view->set_material(_material, _tool.shape == SimToolShape::Plasma);
	_view->clear_paths();
	// the view looks at the whole program, not only at the block, when the size of the scene is known
	_refit = true;

	_player.program(&_program);
	_ready = _program.size() > 0;
	_block_dirty = false;
	_shown_line = -1;
	Logger::log("Simulation: block " + std::to_string(_material.width()) + " x " + std::to_string(_material.height()) + " columns, step " + std::to_string(step) + " mm");
}

// ---------------------------------------------------------------------------------------------- frame
std::string ModSim::format_time(double seconds)
{
	if (seconds < 0)
		seconds = 0;
	int total = (int)(seconds + 0.5);
	char text[32];
	snprintf(text, sizeof(text), "%d:%02d", total / 60, total % 60);
	return text;
}

std::string ModSim::state_text()
{
	if (_loader.active())
		return Lang::t("SIM_TRANSLATING");
	if (!_ready)
		return Lang::t("SIM_NO_PROGRAM");
	if (_player.held_by_program())
		return Lang::t("SIM_HELD");
	switch (_player.state())
	{
	case SimPlayer::State::Playing: return Lang::t(_player.going() ? "SIM_STATE_GOING" : "SIM_STATE_PLAYING");
	case SimPlayer::State::Paused: return Lang::t("SIM_STATE_PAUSED");
	case SimPlayer::State::Finished: return Lang::t("SIM_STATE_FINISHED");
	default: return Lang::t("SIM_STATE_STOPPED");
	}
}

void ModSim::update(double seconds)
{
	if (_loader.active())
	{
		// the translation goes on by slices of time: the interface stays alive
		if (_loader.step(0.010))
		{
			if (!_loader.failed() && !_loader.cancelled())
				finish_loading();
			else if (_loader.failed())
				Logger::error("Simulation: " + _loader.error());
		}
		return;
	}

	if (!_ready)
		return;

	if (_block_dirty)
	{
		// the settings of the block or of the tool changed: it is made again and the program is played again up to where it was
		size_t index = _player.index();
		bool playing = _player.playing();
		build_block();
		if (index > 0)
			_player.go_to_command(index);
		if (playing && index == 0)
			_player.play();
	}

	_player.full_speed(_full_speed);
	_player.set_speed(SPEEDS[std::clamp(_speed_index, 0, SPEED_COUNT - 1)]);
	_player.update(seconds, 0.008);
}

void ModSim::sync_view()
{
	_view->upload(_material);
	_view->set_tool(_tool, _player.position());

	// the line of the text follows the player
	int line = _player.line();
	if (line != _shown_line && (_player.playing() || _player.going()))
		_shown_line = line;
}

void ModSim::render_GUI()
{
	double now = glfwGetTime();
	double dt = _last_time > 0 ? std::min(0.1, now - _last_time) : 0.0;
	_last_time = now;

	update(dt);
	sync_view();

	// the mouse: the movement of a drag is read here, so a drag goes on over the buttons
	ImGuiIO& io = ImGui::GetIO();

	// the space bar plays and pauses, unless a number is being typed
	if (_ready && !io.WantTextInput && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Space, false))
		_player.toggle();

	if (_rotating && io.MouseDown[0])
		_view->camera().rotate(io.MouseDelta.x, io.MouseDelta.y);
	else
		_rotating = false;
	if (_panning && io.MouseDown[1])
		_view->camera().pan(io.MouseDelta.x, io.MouseDelta.y);
	else
		_panning = false;

	// the panels are submitted first, their size tells where the scene is drawn: under the buttons and at the left of the program
	draw_controls();
	_view->feed_color = config.simFeedColor;
	_view->rapid_color = config.simRapidColor;
	draw_overlays();
	if (_ready)
		draw_cards();
	if (_show_program)
		draw_program(_top + 0.0f, 330.0f);

	float program_width = _show_program ? 330.0f : 0.0f;
	float x = _left;
	float y = _top + _controls_height;
	float w = _width - program_width;
	float h = _height - _controls_height;
	_render->clear(_view->background.r, _view->background.g, _view->background.b, 1.0f);
	if (w > 16.0f && h > 16.0f)
	{
		_render->set_bottom((std::max)(0.0f, _window->height() - y - h));
		_render->set_size(w, h);
		_view->camera().set_size(w, h);
		if (_refit)
		{
			_view->camera().fit(glm::vec3(_program.stats().low) - glm::vec3(1.0f), glm::vec3(_program.stats().high) + glm::vec3(1.0f));
			_refit = false;
		}
		_view->draw(w, h);

		// the other modules draw in the whole area of the tabs: the viewport is given back
		_render->set_bottom((std::max)(0.0f, _window->height() - _top - _height));
		_render->set_size(_width, _height);
	}
	(void)x;
}

enum SimIcon { IC_PLAY, IC_PAUSE, IC_BEGIN, IC_STEP, IC_LINE, IC_END, IC_MATERIAL, IC_FEED, IC_RAPID, IC_TOOL, IC_AXES, IC_TOP, IC_FRONT, IC_ISO, IC_SETTINGS, IC_PROGRAM };

// A button drawn with an icon (no image file): `on` shows a toggle which is active
static bool sim_icon(const char* id, SimIcon kind, bool on, const char* tip)
{
	const float S = 28.0f;
	ImVec2 p = ImGui::GetCursorScreenPos();
	bool click = ImGui::InvisibleButton(id, ImVec2(S, S));
	bool hover = ImGui::IsItemHovered();
	ImDrawList* d = ImGui::GetWindowDrawList();
	if (on || hover)
		d->AddRectFilled(p, ImVec2(p.x + S, p.y + S), ImGui::GetColorU32(on ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), 4.0f);
	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	ImVec4 soft = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	soft.w *= 0.6f;
	const ImU32 light = ImGui::GetColorU32(soft);
	ImVec2 c(p.x + S * 0.5f, p.y + S * 0.5f);
	auto P = [&](float x, float y) { return ImVec2(c.x + x, c.y + y); };
	auto tri = [&](float a, float b, float e, float f, float g, float h) { d->AddTriangleFilled(P(a, b), P(e, f), P(g, h), col); };
	auto rect = [&](float a, float b, float e, float f) { d->AddRectFilled(P(a, b), P(e, f), col); };
	auto line = [&](float a, float b, float e, float f) { d->AddLine(P(a, b), P(e, f), col, 2.0f); };
	switch (kind)
	{
	case IC_PLAY: tri(-5, -8, -5, 8, 8, 0); break;
	case IC_PAUSE: rect(-7, -8, -2, 8); rect(2, -8, 7, 8); break;
	case IC_BEGIN: rect(-8, -8, -5, 8); tri(8, -8, 8, 8, -4, 0); break;
	case IC_STEP: tri(-8, -8, -8, 8, 3, 0); rect(5, -8, 8, 8); break;
	case IC_LINE: tri(-9, -8, -9, 8, 0, 0); line(3, -5, 9, -5); line(3, 0, 9, 0); line(3, 5, 9, 5); break;
	case IC_END: tri(-9, -8, -9, 8, -1, 0); tri(-1, -8, -1, 8, 7, 0); rect(7, -8, 9, 8); break;
	case IC_MATERIAL: d->AddRectFilled(P(-8, -3), P(8, 8), col); d->AddQuadFilled(P(-8, -3), P(-4, -8), P(12, -8), P(8, -3), light); break;
	case IC_FEED: d->AddPolyline(std::vector<ImVec2>{ P(-9, 5), P(-4, -5), P(1, 5), P(6, -5), P(9, 0) }.data(), 5, col, 0, 2.0f); break;
	case IC_RAPID: line(-9, 0, -5, 0); line(-2, 0, 2, 0); line(5, 0, 9, 0); break;
	case IC_TOOL: rect(-3, -9, 3, 0); tri(-3, 0, 3, 0, 0, 9); break;
	case IC_AXES: line(-6, 6, 8, 6); line(-6, 6, -6, -8); line(-6, 6, 4, -2); break;
	case IC_TOP: d->AddRect(P(-8, -8), P(8, 8), col, 0, 0, 2.0f); rect(-5, -5, 5, 5); break;
	case IC_FRONT: d->AddRect(P(-8, -6), P(8, 6), col, 0, 0, 2.0f); rect(-8, 2, 8, 6); break;
	case IC_ISO:
		d->AddQuadFilled(P(0, -9), P(8, -4), P(0, 1), P(-8, -4), light);
		d->AddQuad(P(-8, -4), P(0, 1), P(0, 9), P(-8, 4), col, 2.0f);
		d->AddQuad(P(8, -4), P(0, 1), P(0, 9), P(8, 4), col, 2.0f);
		break;
	case IC_SETTINGS: line(-8, -5, 8, -5); line(-8, 0, 8, 0); line(-8, 5, 8, 5); d->AddCircleFilled(P(-3, -5), 2.5f, light); d->AddCircleFilled(P(4, 0), 2.5f, light); d->AddCircleFilled(P(-2, 5), 2.5f, light); break;
	case IC_PROGRAM: line(-8, -6, 8, -6); line(-8, 0, 4, 0); line(-8, 6, 8, 6); break;
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("%s", tip);
	return click;
}

void ModSim::draw_controls()
{
	float program_width = _show_program ? 330.0f : 0.0f;
	ImGui::SetNextWindowPos(ImVec2(_left, _top));
	ImGui::SetNextWindowSize(ImVec2(_width - program_width, 0));
	ImGui::Begin("MOD_SIM_CONTROLS", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	// the program
	if (ImGui::Button(Lang::l("SIM_OUTPUT")))
		load_output();
	ImGui::SameLine();
	if (ImGui::Button(Lang::l("SIM_OPEN")))
	{
		std::string path;
		if (dialog::open_file_dialog("", path, "G-code", "*.nc;*.gcode;*.txt;*.tap;*.ngc;*.cnc;*.*"))
			load_file(path);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(130);
	if (ImGui::BeginCombo("##SIM_TRANSLATOR", _translator_name.c_str()))
	{
		_translators = PythonTranslator::list(translators_folder());
		for (const std::string& name : _translators)
			if (ImGui::Selectable(name.c_str(), name == _translator_name))
				select_translator(name);
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", Lang::l("SIM_TRANSLATOR"));
	ImGui::SameLine();
	if (_loader.active())
	{
		ImGui::ProgressBar((float)_loader.progress(), ImVec2(220, 0), (std::string(Lang::l("SIM_TRANSLATING")) + " " + std::to_string((int)(_loader.progress() * 100)) + "%").c_str());
		ImGui::SameLine();
		if (ImGui::Button(Lang::l("SIM_CANCEL")))
			_loader.cancel();
	}
	else if (_ready)
	{
		const SimStats& s = _program.stats();
		ImGui::Text("%s  -  %d %s, %s", _name.c_str(), (int)_program.size(), Lang::l("SIM_COMMANDS"), format_time(s.total_time).c_str());
		if (s.errors > 0 || s.warnings > 0)
		{
			ImGui::SameLine();
			ImGui::TextColored(s.errors > 0 ? ImVec4(1.0f, 0.35f, 0.3f, 1.0f) : ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[%d %s, %d %s]", s.errors, Lang::l("SIM_ERRORS"), s.warnings, Lang::l("SIM_WARNINGS"));
		}
	}
	else
		ImGui::TextUnformatted(_name.empty() ? Lang::l("SIM_NO_PROGRAM") : _name.c_str());

	// the time and the position
	if (_ready)
	{
		ImGui::ProgressBar((float)_player.fraction(), ImVec2(-1, 0), (format_time(_player.elapsed()) + " / " + format_time(_player.total())).c_str());
		ImGui::Text("T%d   %s   %s: %d", _player.tool(),
			_player.spindle() == 0 ? Lang::l("SIM_SPINDLE_OFF") : (_player.spindle() == 1 ? Lang::l("SIM_SPINDLE_CW") : Lang::l("SIM_SPINDLE_CCW")), Lang::l("SIM_LINE"), _player.line());
		ImGui::SameLine();
		bool held = _player.held_by_program();
		ImGui::TextColored(held ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "  [%s]", state_text().c_str());
	}

	// the block and the tool
	if (_show_settings)
	{
		ImGui::Separator();
		const char* shapes[] = { Lang::l("SIM_SHAPE_FLAT"), Lang::l("SIM_SHAPE_BALL"), Lang::l("SIM_SHAPE_TORUS"), Lang::l("SIM_SHAPE_CONE"), Lang::l("SIM_SHAPE_PLASMA") };
		bool changed = false;
		ImGui::SetNextItemWidth(140);
		changed |= ImGui::Combo(Lang::l("SIM_TOOL_SHAPE"), &_tool_shape, shapes, 5);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90);
		if (ImGui::InputFloat(Lang::l("SIM_DIAMETER"), &_tool_diameter, 0.5f, 1.0f, "%.2f"))
		{
			_tool_from_group = false;
			changed = true;
		}
		if (_tool_shape == (int)SimToolShape::Torus)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(90);
			changed |= ImGui::InputFloat(Lang::l("SIM_CORNER_RADIUS"), &_tool_corner, 0.1f, 0.5f, "%.2f");
		}
		if (_tool_shape == (int)SimToolShape::Cone)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(90);
			changed |= ImGui::InputFloat(Lang::l("SIM_ANGLE"), &_tool_angle, 5.0f, 15.0f, "%.0f");
		}
		ImGui::SetNextItemWidth(90);
		changed |= ImGui::InputFloat(Lang::l("SIM_STOCK_TOP"), &_stock_top, 0.5f, 1.0f, "%.2f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90);
		changed |= ImGui::InputFloat(Lang::l("SIM_STOCK_THICKNESS"), &_stock_thickness, 0.5f, 1.0f, "%.2f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90);
		changed |= ImGui::InputFloat(Lang::l("SIM_PRECISION"), &_precision, 5.0f, 10.0f, "%.0f");
		_precision = std::clamp(_precision, 3.0f, 100.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90);
		changed |= ImGui::InputFloat(Lang::l("SIM_MAX_CELLS"), &_max_cells, 1.0f, 4.0f, "%.0f");
		_max_cells = std::clamp(_max_cells, 1.0f, 32.0f);
		_tool_diameter = std::max(_tool_diameter, 0.1f);
		if (ImGui::Checkbox(Lang::l("SIM_OPTIONAL_STOP"), &_optional_stop))
		{
			_settings.optional_stop = _optional_stop;
			_program.analyze(_settings);
		}
		if (changed)
			_block_dirty = true;
	}

	_controls_height = ImGui::GetWindowHeight();
	ImGui::End();
}

// The information of the machine, in cards over the left of the scene: time, feed rate, tool, position, state, size of the work
void ModSim::draw_cards()
{
	const float width = 150.0f;
	ImGui::SetNextWindowPos(ImVec2(_left + 8.0f, _top + _controls_height + 8.0f));
	ImGui::SetNextWindowSize(ImVec2(width + 16.0f, 0));
	ImGui::Begin("MOD_SIM_CARDS", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);

	const ImU32 border = ImGui::GetColorU32(ImGuiCol_ButtonActive);
	const ImU32 fill = IM_COL32(40, 40, 44, 200);
	ImDrawList* draw = ImGui::GetWindowDrawList();

	// a card: the title, then the lines drawn by the callback
	auto card = [&](const char* title, int lines, auto content)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		float h = ImGui::GetTextLineHeightWithSpacing() * (lines + 1) + 22.0f;
		draw->AddRectFilled(p, ImVec2(p.x + width, p.y + h), fill, 8.0f);
		draw->AddRect(p, ImVec2(p.x + width, p.y + h), border, 8.0f, 0, 2.0f);
		ImVec2 t = ImGui::CalcTextSize(title);
		draw->AddText(ImVec2(p.x + (width - t.x) * 0.5f, p.y + 4.0f), IM_COL32(255, 255, 255, 255), title);
		ImGui::SetCursorScreenPos(ImVec2(p.x + 10.0f, p.y + 4.0f + ImGui::GetTextLineHeightWithSpacing()));
		ImGui::BeginGroup();
		content(p.x, width);
		ImGui::EndGroup();
		ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 6.0f));
	};
	// a text centered in the card
	auto centered = [&](float x, float w, const std::string& text)
	{
		ImVec2 t = ImGui::CalcTextSize(text.c_str());
		ImVec2 c = ImGui::GetCursorScreenPos();
		draw->AddText(ImVec2(x + (w - t.x) * 0.5f, c.y), IM_COL32(255, 255, 255, 255), text.c_str());
		ImGui::Dummy(ImVec2(1, ImGui::GetTextLineHeightWithSpacing()));
	};
	// a light with its name
	auto light = [&](float x, const char* name, bool on, ImU32 color)
	{
		ImVec2 c = ImGui::GetCursorScreenPos();
		float r = ImGui::GetTextLineHeight() * 0.4f;
		draw->AddCircleFilled(ImVec2(x + width - 20.0f, c.y + ImGui::GetTextLineHeight() * 0.5f), r, on ? color : IM_COL32(190, 50, 45, 255));
		draw->AddText(c, IM_COL32(255, 255, 255, 255), name);
		ImGui::Dummy(ImVec2(1, ImGui::GetTextLineHeightWithSpacing()));
	};

	char text[96];
	card(Lang::l("SIM_CARD_TIME"), 1, [&](float x, float w) { centered(x, w, format_time(_player.elapsed())); });
	snprintf(text, sizeof(text), "%g", _player.feed());
	card(Lang::l("SIM_CARD_FEED"), 1, [&](float x, float w) { centered(x, w, text); });
	snprintf(text, sizeof(text), "T%d", _player.tool());
	card(Lang::l("SIM_TOOL"), 1, [&](float x, float w) { centered(x, w, text); });
	glm::dvec3 p = _player.position();
	card(Lang::l("SIM_CARD_POSITION"), 3, [&](float x, float w)
	{
		const char* names[3] = { "X", "Y", "Z" };
		for (int i = 0; i < 3; i++)
		{
			ImVec2 c = ImGui::GetCursorScreenPos();
			snprintf(text, sizeof(text), "%.3f", p[i]);
			ImVec2 t = ImGui::CalcTextSize(text);
			draw->AddText(c, IM_COL32(255, 255, 255, 255), names[i]);
			draw->AddText(ImVec2(x + w - 12.0f - t.x, c.y), IM_COL32(255, 255, 255, 255), text);
			ImGui::Dummy(ImVec2(1, ImGui::GetTextLineHeightWithSpacing()));
		}
	});
	const ImU32 green = IM_COL32(60, 190, 70, 255);
	card(Lang::l("SIM_CARD_STATE"), 4, [&](float x, float w)
	{
		light(x, Lang::l("SIM_CARD_CUTTING"), _player.playing() && !_player.held_by_program(), green);
		light(x, Lang::l("SIM_CARD_SPINDLE"), _player.spindle() != 0, green);
		light(x, Lang::l("SIM_CARD_COOLANT"), _player.coolant() != 0, green);
		light(x, Lang::l("SIM_CARD_PAUSED"), !_player.playing(), IM_COL32(230, 170, 40, 255));
	});
	const SimStats& s = _program.stats();
	card(Lang::l("SIM_CARD_SIZE"), 3, [&](float x, float w)
	{
		const char* names[3] = { "X", "Y", "Z" };
		for (int i = 0; i < 3; i++)
		{
			ImVec2 c = ImGui::GetCursorScreenPos();
			snprintf(text, sizeof(text), "%.1f", s.high[i] - s.low[i]);
			ImVec2 t = ImGui::CalcTextSize(text);
			draw->AddText(c, IM_COL32(255, 255, 255, 255), names[i]);
			draw->AddText(ImVec2(x + w - 12.0f - t.x, c.y), IM_COL32(255, 255, 255, 255), text);
			ImGui::Dummy(ImVec2(1, ImGui::GetTextLineHeightWithSpacing()));
		}
	});
	ImGui::End();
}

// The buttons over the scene: the display options at the top, the player at the bottom, the speed at the right
void ModSim::draw_overlays()
{
	const float scene_width = _width - (_show_program ? 330.0f : 0.0f);
	const float top = _top + _controls_height + 8.0f;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize;

	// what is shown
	ImGui::SetNextWindowPos(ImVec2(_left + scene_width * 0.5f, top), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.65f);
	ImGui::Begin("MOD_SIM_DISPLAY", NULL, flags);
	if (sim_icon("##SIM_MATERIAL", IC_MATERIAL, _view->show_material, Lang::l("SIM_MATERIAL"))) _view->show_material = !_view->show_material;
	ImGui::SameLine();
	if (sim_icon("##SIM_FEED", IC_FEED, _view->show_feed, Lang::l("SIM_FEED"))) _view->show_feed = !_view->show_feed;
	ImGui::SameLine();
	if (sim_icon("##SIM_RAPIDS", IC_RAPID, _view->show_rapid, Lang::l("SIM_RAPIDS"))) _view->show_rapid = !_view->show_rapid;
	ImGui::SameLine();
	if (sim_icon("##SIM_TOOL", IC_TOOL, _view->show_tool, Lang::l("SIM_TOOL"))) _view->show_tool = !_view->show_tool;
	ImGui::SameLine();
	if (sim_icon("##SIM_AXES", IC_AXES, _view->show_axes, Lang::l("SIM_AXES"))) _view->show_axes = !_view->show_axes;
	ImGui::SameLine(0, 14);
	const SimIcon views[3] = { IC_TOP, IC_FRONT, IC_ISO };
	const int view_codes[3] = { 0, 1, 3 };
	const char* view_names[3] = { Lang::l("SIM_VIEW_TOP"), Lang::l("SIM_VIEW_FRONT"), Lang::l("SIM_VIEW_ISO") };
	for (int i = 0; i < 3; i++)
	{
		if (sim_icon(("##SIM_VIEW" + std::to_string(i)).c_str(), views[i], false, view_names[i]))
		{
			_view->camera().view(view_codes[i]);
			_view->camera().fit(glm::vec3(_program.stats().low) - glm::vec3(1.0f), glm::vec3(_program.stats().high) + glm::vec3(1.0f));
		}
		ImGui::SameLine();
	}
	ImGui::SameLine(0, 14);
	if (sim_icon("##SIM_SETTINGS", IC_SETTINGS, _show_settings, Lang::l("SIM_SETTINGS"))) _show_settings = !_show_settings;
	ImGui::SameLine();
	if (sim_icon("##SIM_PROGRAM", IC_PROGRAM, _show_program, Lang::l("SIM_PROGRAM"))) _show_program = !_show_program;
	ImGui::End();

	// the player
	ImGui::SetNextWindowPos(ImVec2(_left + scene_width * 0.5f, _top + _height - 10.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.65f);
	ImGui::Begin("MOD_SIM_PLAYER", NULL, flags);
	ImGui::BeginDisabled(!_ready);
	bool playing = _player.playing() && !_player.going();
	if (sim_icon("##SIM_PLAY", playing ? IC_PAUSE : IC_PLAY, false, playing ? Lang::l("SIM_PAUSE") : Lang::l("SIM_PLAY")))
		_player.toggle();
	ImGui::SameLine();
	if (sim_icon("##SIM_BEGIN", IC_BEGIN, false, Lang::l("SIM_STOP")))
		_player.stop();
	ImGui::SameLine();
	if (sim_icon("##SIM_STEP", IC_STEP, false, Lang::l("SIM_STEP")))
		_player.step_command();
	ImGui::SameLine();
	if (sim_icon("##SIM_LINE", IC_LINE, false, Lang::l("SIM_STEP_LINE")))
		_player.step_line();
	ImGui::SameLine();
	if (sim_icon("##SIM_END", IC_END, false, Lang::l("SIM_END")))
		_player.go_to_end();
	ImGui::EndDisabled();
	ImGui::End();

	// the speed: a vertical slider, the fastest at the top, and the full speed
	ImGui::SetNextWindowPos(ImVec2(_left + scene_width - 10.0f, _top + _controls_height + (_height - _controls_height) * 0.5f), ImGuiCond_Always, ImVec2(1.0f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.65f);
	ImGui::Begin("MOD_SIM_SPEED", NULL, flags);
	// the last position of the slider is the full speed
	int position = _full_speed ? SPEED_COUNT : std::clamp(_speed_index, 0, SPEED_COUNT - 1);
	char label[32];
	snprintf(label, sizeof(label), _full_speed ? "max" : "x%g", SPEEDS[std::clamp(_speed_index, 0, SPEED_COUNT - 1)]);
	ImGui::TextUnformatted(label);
	if (ImGui::VSliderInt("##SIM_SPEED", ImVec2(28, 200), &position, 0, SPEED_COUNT, ""))
	{
		_full_speed = position >= SPEED_COUNT;
		if (!_full_speed)
			_speed_index = position;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", Lang::l("SIM_SPEED"));
	ImGui::End();
}

void ModSim::draw_program(float top, float width)
{
	float x = _left + _width - width;
	ImGui::SetNextWindowPos(ImVec2(x, _top));
	ImGui::SetNextWindowSize(ImVec2(width, _height));
	ImGui::Begin("MOD_SIM_PROGRAM", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	_window->push_default_font();
	if (_lines.empty())
		ImGui::TextUnformatted(Lang::l("SIM_NO_PROGRAM"));

	ImGuiListClipper clipper;
	clipper.Begin((int)_lines.size());
	// the line which is played is always shown
	static int last_shown = -1;
	bool follow = _shown_line != last_shown && _shown_line > 0;
	if (follow && _shown_line - 1 < (int)_lines.size())
		clipper.IncludeItemByIndex(_shown_line - 1);
	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			ImGui::PushID(i);
			int level = i < (int)_line_level.size() ? _line_level[i] : -1;
			bool current = (i + 1) == _player.line() && _player.index() > 0;
			if (level == 2) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.35f, 1.0f));
			else if (level == 1) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.25f, 1.0f));

			char label[512];
			snprintf(label, sizeof(label), "%5d  %s", i + 1, _lines[i].c_str());
			if (ImGui::Selectable(label, current, 0))
				_player.go_to_line(i + 1);
			if (follow && i + 1 == _shown_line)
				ImGui::SetScrollHereY(0.5f);

			if (level >= 1)
				ImGui::PopStyleColor();
			ImGui::PopID();
		}
	}
	if (follow)
		last_shown = _shown_line;
	_window->pop_font();

	ImGui::End();
}

void ModSim::render_menu()
{
	if (ImGui::BeginMenu(Lang::l("SIM_TITLE")))
	{
		if (ImGui::MenuItem(Lang::l("SIM_OUTPUT")))
			load_output();
		if (ImGui::MenuItem(Lang::l("SIM_OPEN")))
		{
			std::string path;
			if (dialog::open_file_dialog("", path, "G-code", "*.nc;*.gcode;*.txt;*.tap;*.ngc;*.cnc;*.*"))
				load_file(path);
		}
		if (ImGui::MenuItem(Lang::l("SIM_OPEN_FOLDER")))
			dialog::open_folder(translators_folder());
		ImGui::Separator();
		if (ImGui::MenuItem(_player.playing() ? Lang::l("SIM_PAUSE") : Lang::l("SIM_PLAY"), "Space", false, _ready))
			_player.toggle();
		if (ImGui::MenuItem(Lang::l("SIM_STOP"), NULL, false, _ready))
			_player.stop();
		if (ImGui::MenuItem(Lang::l("SIM_END"), NULL, false, _ready))
			_player.go_to_end();
		ImGui::EndMenu();
	}
}

// ---------------------------------------------------------------------------------------------- mouse and keys
void ModSim::left_button_down()
{
	_rotating = true;
}

void ModSim::right_button_down()
{
	_panning = true;
}

void ModSim::scroll(double xdelta, double ydelta)
{
	_view->camera().zoom((float)ydelta);
}

void ModSim::key_pressed(int key, int scancode, int action, int mods)
{
	// the space bar is read in render_GUI (the application does not give the keys when a window of the interface has the focus)
}

void ModSim::drop(std::vector<std::string> paths)
{
	if (!paths.empty())
		load_file(paths[0]);
}

// ---------------------------------------------------------------------------------------------- the test bridge
bool ModSim::command(const std::vector<std::string>& w)
{
	if (w.empty() || w[0] != "sim")
		return false;
	std::string what = w.size() > 1 ? w[1] : "";
	auto number = [&](size_t i) { return w.size() > i ? std::atof(w[i].c_str()) : 0.0; };

	if (what == "load" && w.size() > 2)
		load_file(w[2]);
	else if (what == "output")
		load_output();
	else if (what == "play")
		_player.play();
	else if (what == "pause")
		_player.pause();
	else if (what == "stop")
		_player.stop();
	else if (what == "step")
		_player.step_command();
	else if (what == "stepline")
		_player.step_line();
	else if (what == "end")
		_player.go_to_end();
	else if (what == "line")
		_player.go_to_line((int)number(2));
	else if (what == "speed")
		_speed_index = std::clamp((int)number(2), 0, SPEED_COUNT - 1);
	else if (what == "full")
		_full_speed = w.size() > 2 && w[2] == "on";
	else if (what == "settings")
		_show_settings = w.size() > 2 && w[2] == "on";
	else if (what == "program")
		_show_program = w.size() > 2 && w[2] == "on";
	else if (what == "view" && w.size() > 2)
	{
		int preset = w[2] == "top" ? 0 : (w[2] == "front" ? 1 : (w[2] == "side" ? 2 : 3));
		_view->camera().view(preset);
		_view->camera().fit(glm::vec3(_program.stats().low) - glm::vec3(1.0f), glm::vec3(_program.stats().high) + glm::vec3(1.0f));
	}
	else if (what == "rotate")
		_view->camera().rotate((float)number(2), (float)number(3));
	else if (what == "zoom")
		_view->camera().zoom((float)number(2));
	else if (what == "tool" && w.size() > 3)
	{
		_tool_shape = w[2] == "ball" ? 1 : (w[2] == "torus" ? 2 : (w[2] == "cone" ? 3 : (w[2] == "plasma" ? 4 : 0)));
		_tool_diameter = (float)number(3);
		_tool_from_group = false;
		_block_dirty = true;
	}
	else if (what == "stock" && w.size() > 3)
	{
		_stock_top = (float)number(2);
		_stock_thickness = (float)number(3);
		_block_dirty = true;
	}
	else if (what == "show" || what == "hide")
	{
		bool value = what == "show";
		std::string item = w.size() > 2 ? w[2] : "";
		if (item == "feed") _view->show_feed = value;
		else if (item == "rapid") _view->show_rapid = value;
		else if (item == "tool") _view->show_tool = value;
		else if (item == "material") _view->show_material = value;
		else if (item == "axes") _view->show_axes = value;
	}
	else if (what == "translator" && w.size() > 2)
		select_translator(w[2]);
	else if (what == "status")
	{
		char text[512];
		glm::dvec3 p = _player.position();
		snprintf(text, sizeof(text), "sim: ready %d, loading %d, state %d, command %d/%d, line %d, position %.3f %.3f %.3f, time %.2f / %.2f, held %d, removed %.1f mm3, columns %dx%d",
			(int)_ready, (int)_loader.active(), (int)_player.state(), (int)_player.index(), (int)_program.size(), _player.line(), p.x, p.y, p.z,
			_player.elapsed(), _player.total(), (int)_player.held_by_program(), _material.removed_volume(), _material.width(), _material.height());
		Logger::log(text);
		bridge::log(text);
	}
	else
		return false;
	return true;
}
