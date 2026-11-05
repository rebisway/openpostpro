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

#pragma once
#include <graphic.h>
#include <anchor.h>
#include <functional>

#define PRECISION  5.0f

class Shape : public Graphic
{
private:
	bool _done = false;								// true if the shape is fully constructed
	Anchor* _modification_anchor = nullptr;			// hold current anchor that is manipulated
	std::string _parent;							// hold parent name
	int _tag = -1;

protected:
	bool _over = false;								// true if mouse is over
	geometry::rectangle _bounds;					// clip rectangle

public:
	// global variable to store the max distance to the shape for is_over method
	static float precision;

	int tag();
	void tag(int value);

	// properties
	std::string parent();
	void parent(std::string value);

	bool over();
	void over(bool value);

	geometry::rectangle& bounds();
	glm::vec2& topLeft();
	glm::vec2& bottomRight();

	// constructor/destructor
	Shape(Renderer* r) : Graphic(r) {}
	virtual ~Shape() {}
	virtual Shape* clone() { return nullptr; }

	virtual void reverse() {}

	virtual glm::vec2 first() { return glm::vec2(); }
	virtual void first(glm::vec2 value) {}
	virtual glm::vec2 last() { return glm::vec2(); }
	virtual void last(glm::vec2 value) {}

	// return anchor under ref_point
	virtual Anchor* anchor(glm::vec2 point) { return nullptr; }

	// return anchor under ref_point
	virtual std::vector<Anchor*> anchors() { return std::vector<Anchor*>(); }

	// ref_point is over shape
	virtual bool is_over(glm::vec2 point) { return false; }

	// return magnets ref_point to test, include anchors but can have more
	virtual std::vector<glm::vec2> magnets() { return std::vector<glm::vec2>(); }	

	void modification(Anchor* value) { _modification_anchor = value; }
	Anchor* modification() { return _modification_anchor; }

	virtual void move(glm::vec2 point) {}
	virtual bool point(glm::vec2 point) { return true; }
	virtual void done(bool value) { _done = value; }
	virtual bool done() { return _done; }

	virtual Shape* symmetry(glm::vec2 p1, glm::vec2 p2) { return nullptr; }
	virtual Shape* symmetry(glm::vec2 center) { return nullptr; }

	// draw the properties window: common frame, then the rows given by properties()
	void ui() override;

protected:
	// override to add the rows of the shape, using the property_* helpers
	virtual void properties() {}

	// each helper draws one row (label + input). Return true when the user validated a new value, stored in 'value'
	bool property_point(const char* label, const char* id, glm::vec2& value);
	bool property_float(const char* label, const char* id, float& value);
	bool property_bool(const char* label, const char* id, bool& value);
	bool property_string(const char* label, const char* id, std::string& value);
	// drop-down list: 'selected' is the index in 'items', returns true when another item is chosen
	bool property_combo(const char* label, const char* id, int& selected, const std::vector<std::string>& items);
	// read only row
	void property_text(const char* label, const std::string& value);

	// push the undo action, then apply the change
	void modify(const std::function<void()>& change);

private:
	void property_label(const char* label);
};