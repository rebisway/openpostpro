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

#pragma once
#ifndef FTREE_H
#define FTREE_H

#include <geometry.h>
#include <list>


template <class T> class SearchNode
{
public:
	virtual void add(T data, geometry::rectangle& bounds) {}
	virtual std::vector<T> search(geometry::rectangle& bounds) { return std::vector<T>(); }
};


template <class T> class QuadNode : public SearchNode<T>
{
private:		
	geometry::rectangle _bounds;
	std::vector<QuadNode<T>> _children;
	geometry::rectangle _b0;
	geometry::rectangle _b1;
	geometry::rectangle _b2;
	geometry::rectangle _b3;
	std::vector<T> _objects;
	std::vector<geometry::rectangle> _objects_bounds;
	int _index = 0;
	int _depth = 0;
	bool _created = false;

	void check_children();

public:
	bool created() { return _created; }

	void create(geometry::rectangle& bounds, int index = 0, int depth = 100);
	void reset();
	void add(T data, geometry::rectangle& bounds) override;
	std::vector<T> search(geometry::rectangle& bounds) override;
};


template<class T>
inline void QuadNode<T>::check_children()
{
	if (_children.size() == 0) {
		_children.push_back(QuadNode<T>());
		_children.back().create(_b0, _index + 1, _depth);
		_children.push_back(QuadNode<T>());
		_children.back().create(_b1, _index + 1, _depth);
		_children.push_back(QuadNode<T>());
		_children.back().create(_b2, _index + 1, _depth);
		_children.push_back(QuadNode<T>());
		_children.back().create(_b3, _index + 1, _depth);
	}
}


//template<class T>
//inline void QuadNode<T>::create(glm::vec2& top_left, glm::vec2& bottom_right, int index, int depth)
//{
//	_bounds = geometry::rectangle(top_left, bottom_right);
//	create(_bounds, index, depth);
//}

template<class T>
inline void QuadNode<T>::create(geometry::rectangle& bounds, int index, int depth)
{
	_depth = depth;
	_bounds = bounds;

	float w = bounds.width() / 2;
	float h = bounds.height() / 2;

	_b0 = geometry::rectangle(bounds.left(), bounds.top(), bounds.left() + w, bounds.bottom() + h);
	_b1 = geometry::rectangle(bounds.left() + w, bounds.top(), bounds.right(), bounds.bottom() + h);
	_b2 = geometry::rectangle(bounds.left(), bounds.bottom() + h, bounds.left() + w, bounds.bottom());
	_b3 = geometry::rectangle(bounds.left() + w, bounds.bottom() + h, bounds.right(), bounds.bottom());

	_index = index;

	_created = true;
}

template<class T>
inline void QuadNode<T>::reset()
{
	_created = false;
	for (auto c : _children)
		c.reset();
	_objects.clear();
	_objects_bounds.clear();
	_children.clear();
}

template<class T>
inline void QuadNode<T>::add(T data, geometry::rectangle& bounds)
{
	if (_index < _depth)
	{
		if (_b0.contains(bounds)) {
			check_children();
			_children[0].add(data, bounds);
		}
		else if (_b1.contains(bounds)) {
			check_children();
			_children[1].add(data, bounds);
		}
		else if (_b2.contains(bounds)) {
			check_children();
			_children[2].add(data, bounds);
		}
		else if (_b3.contains(bounds)) {
			check_children();
			_children[3].add(data, bounds);
		}
		else {
			_objects.push_back(data);
			_objects_bounds.push_back(bounds);
		}
	}
	else
	{
		_objects.push_back(data);
		_objects_bounds.push_back(bounds);
	}
}

template<class T>
inline std::vector<T> QuadNode<T>::search(geometry::rectangle& bounds)
{
	std::vector<T> result;

	if (_bounds.contains(bounds) || _bounds.intersect(bounds) || bounds.contains(_bounds)) {
		if (_children.size() == 4) {
			auto c0 = _children[0].search(bounds);
			result.insert(result.end(), c0.begin(), c0.end());

			auto c1 = _children[1].search(bounds);
			result.insert(result.end(), c1.begin(), c1.end());

			auto c2 = _children[2].search(bounds);
			result.insert(result.end(), c2.begin(), c2.end());

			auto c3 = _children[3].search(bounds);
			result.insert(result.end(), c3.begin(), c3.end());
		}
		for (int i = 0; i < _objects_bounds.size(); i++) {
			geometry::rectangle r = _objects_bounds[i];
			if (r.contains(bounds) || r.intersect(bounds) || bounds.contains(r))
				result.push_back(_objects[i]);
		}
	}

	return result;
}

#endif
