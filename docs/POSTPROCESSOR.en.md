# Post-processor API reference

*Version française : [POSTPROCESSOR.fr.md](POSTPROCESSOR.fr.md).*

This is the reference for anyone who writes a post-processor. It lists every method, grouped by what it is for, with its arguments, when it is called, what it must return and an example. The reference implementation is `postpro/generic_mill.py`; copy it when you start. CAD scripts are in [CAD-SCRIPTS.en.md](CAD-SCRIPTS.en.md).

Covered here: installation, the life cycle of a run, every method of the `Postpro` class, the properties dialog, and a complete minimal example.

## 0. Conventions

### Files and loading

| Item | Rule |
|---|---|
| Location | the `postpro` folder beside the executable |
| Loading | at startup, each `.py` file is copied to `~/.openpostpro/postpro` and loaded from there |
| Name | your own (`my_mill.py`): the copy **overwrites files of the same name**, so never edit a bundled file in place |
| Class | the file defines a class named `Postpro` |
| Engine | `postcore.py` is the engine. It is never loaded as a post-processor. Do not edit it |
| Errors | written to `~/.openpostpro/openpostpro.log` |
| Python | 3.12 or later, installed with "Add Python to PATH" (see the README) |

### Who calls whom

```
OpenPostPro (C++)  ->  Postcore (postcore.py)  ->  Postpro (your file)
```

OpenPostPro never calls your class directly. It calls the engine, which keeps the **current position** (`x`, `y`, `z`), the line numbers and the line count, then asks your class for the text of each step. The engine hands itself to your class: it is `self.core`, set through `set_core()`.

### The rule on coordinates

Every move or coordinate change must go through the engine API (`self.core.rapid`, `linear`, `clockwise`, `counter_clockwise`, `set_axes`), never through a hand-built string such as `'G0 Z5'`. The engine records the position (`self.core.x/y/z`). A move emitted behind its back makes the later decisions (safe height, canned cycles, compensation cancel) use a wrong position, and the output becomes inconsistent.

### Types and encodings

- Every output method **returns a string**. Return `''` when there is nothing to write.
- Do not add the end of line: the engine adds `stop_line()` itself. A method that already ends with it is not doubled.
- `axes`: list of axis names, for example `['X', 'Y', 'Z']`. Circular moves add `'I'`, `'J'`, `'K'` (arc center).
- `values`: list of numbers, same size and same order as `axes`.
- Numbers are Python `float`; format them yourself (see `n()` in `generic_mill.py`).
- The output is text, converted to UTF-8 and written in text mode (on Windows `\n` is written `\r\n`). Binary files are not supported through the returned string.
- Language codes: `en-us`, `fr-fr`, `es-es`.

### Writing less: the optional axes

`pass_xy_to_linear()`, `pass_z_to_linear()`, `pass_xy_to_cw_ccw()` and `pass_z_to_cw_ccw()` choose what the engine sends in `axes` for linear and circular moves. `True` always sends the axis (X and Y together, or Z); `False` sends only the coordinates that changed since the previous move. Some controllers require all the coordinates on every move, others accept omitted ones: `True` suits the first kind, `False` gives a shorter output on the second.

## 1. Life cycle of a run

Order of the calls for one file. Indented lines repeat.

```
set_language(lang)            when the post-processor is loaded
set_core(core)                once
get_property / set_property   loading and saved values
-- generation --
start_output()                first characters of the file
comment(...)                  version, file name, date
start_program()  initial_position()
  per group:
    comment(...)  tool(n)  spindle_speed(s)
    tool_left/right_radius_compensation()  tool_length_compensation(n)
    per tool path:
      comment(...)  start_toolpath()
      rapid (Z to safe, then XY)
      feed(plunge)  linear (plunge)  feed(work)
      start_single_path()
      linear / clockwise / counter_clockwise ...
      stop_single_path()   rapid (Z to safe)
      stop_toolpath()
    tool_cancel_*_compensation()
final_position()  stop_program()
stop_output()                 last characters of the file
```

Drilling groups call `drilling`, `pecking`, `tapping` or `boring` in place of the plunge and the moves, then `cancel_canned_cycle()`.

## 2. Identity

| Method | Arguments | Returns | Meaning |
|---|---|---|---|
| `set_language(value)` | `'en-us'`, `'fr-fr'` or `'es-es'` | nothing | selects the dictionary of interface strings |
| `get_description()` | none | string | name in the menu **Post-processor / Select / Category / Description** |
| `get_category()` | none | string | menu category: `"MILL"`, `"PLASMA"`, ... |
| `get_extention()` | none | string | extension of the output file, without the dot (`'nc'`) |
| `set_core(value)` | the engine | nothing | store it in `self.core`; do not modify |

Each language is a dictionary, selected by `set_language()`:

```python
self.en_us = {'description': 'My mill'}
self.fr_fr = {'description': 'Ma fraiseuse'}
self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr}

def set_language(self, value):
    self.lang = self.Language[value]
```

## 3. Machine behaviour

| Method | Returns | Meaning |
|---|---|---|
| `get_decimal()` | number | maximum number of decimals |
| `incrementation()` | number | step of the line numbers (`N5`, `N10`, ...); `-1` for no numbering |
| `set_safe_position(value)` | nothing | safe Z height of the current group; keep it in `self.safe` |
| `pass_xy_to_linear()`, `pass_z_to_linear()` | boolean | always send X Y / Z on `rapid` and `linear` (see section 0) |
| `pass_xy_to_cw_ccw()`, `pass_z_to_cw_ccw()` | boolean | same for `clockwise` and `counter_clockwise` |
| `use_safe_pos_before_tool_change()` | boolean | lift to the safe height before a tool change |
| `get_disable_z()` | boolean | `True` for a machine without Z (plasma): no plunge passes |
| `ijk_relative()` | boolean | `True` when the arc center `I J K` is given relative, `False` when it is absolute |

## 4. Properties

Properties are the parameters of the post-processor. The user edits them in the menu **Post-processor / Configuration**. The dialog is built automatically from the number of properties, their labels and their types: you write no interface code. OpenPostPro saves the values itself and gives them back through `set_property` at the next startup, so the post-processor stores nothing.

| Method | Arguments | Returns | Meaning |
|---|---|---|---|
| `get_properties_count()` | none | number | how many properties |
| `get_property_description(index)` | index from 0 | string | label of the property |
| `get_property(index)` | index from 0 | see below | current value; its type selects the control |
| `set_property(index, value)` | index, value | nothing | receives the value edited by the user |

| Type returned by `get_property` | Control |
|---|---|
| `bool` | check box |
| `int` | integer field with - and + buttons |
| `float` | decimal field |
| `str` | text field |
| `list` of `str` | drop-down list; the first item is the selected value, the others are the choices |

For a drop-down list, see the `Unit` property of `generic_mill.py`.

## 5. File and line format

| Method | Arguments | Returns | Meaning |
|---|---|---|---|
| `start_output()` | none | string | first characters of the file, usually `%\n` |
| `stop_output()` | none | string | last characters of the file, usually `%` |
| `start_program()` | none | string | first program line: absolute mode, units, ... (`G90 G21`) |
| `stop_program()` | none | string | last program line (`M2`) |
| `initial_position()` | none | string | first move; use `self.core.rapid(...)` |
| `final_position()` | none | string | last move; use `self.core.rapid(...)` and test the position (see example) |
| `comment(text)` | the comment | string | the comment in the syntax of the controller; `''` to drop it |
| `start_line(num)` | line number | string | prefix of a numbered line (`N00005`); `''` without numbering |
| `stop_line()` | none | string | end of line, usually `\n` |

## 6. Tool paths

| Method | Called | Returns |
|---|---|---|
| `start_toolpath()` | before each tool path | string, usually `''` |
| `stop_toolpath()` | after each tool path | string |
| `start_single_path()` | after the first plunge of a tool path | string |
| `stop_single_path()` | before the lift to the safe height | string |

## 7. Moves

Each method returns the string of one move. All of them are called through the engine, which records the new position.

| Method | Arguments | Meaning | Example (generic mill) |
|---|---|---|---|
| `rapid(axes, values)` | axes, values | rapid move | `rapid(['X','Y'],[10,5.5])` -> `G0X10Y5.5` |
| `linear(axes, values)` | axes, values | working move | `linear(['Z'],[-2])` -> `G1Z-2` |
| `clockwise(axes, values)` | `X Y Z I J K` | clockwise arc | `G2X10Y0I5J0` |
| `counter_clockwise(axes, values)` | `X Y Z I J K` | counter-clockwise arc | `G3X10Y0I5J0` |
| `feed(value)` | feed rate | feed rate | `F1200` |
| `pause(time)` | seconds | dwell | `G4P2` |

The generic mill is in condensed mode (no spaces) in these examples; with the option off the same moves read `G0 X10 Y5.5`.

## 8. Tool and spindle

| Method | Arguments | Returns (generic mill) |
|---|---|---|
| `tool(number)` | tool number | `M6T3`; `''` for tool 0 |
| `start_spindle_clockwise()` | none | `M3` |
| `start_spindle_counter_clockwise()` | none | `M4` |
| `stop_spindle()` | none | `M5` |
| `spindle_speed(speed)` | rpm | `S12000` |

## 9. Compensation

| Method | Arguments | Returns (generic mill) |
|---|---|---|
| `tool_left_radius_compensation()` | none | `G41` |
| `tool_right_radius_compensation()` | none | `G42` |
| `tool_cancel_radius_compensation()` | none | lifts to the safe height if needed, then `G40` |
| `tool_length_compensation(tool_number)` | tool number | `G43H3` |
| `tool_cancel_length_compensation()` | none | lifts to the safe height if needed, then `G49` |

When a cancel method lifts the tool, it does it with `self.core.rapid(['Z'], [self.safe])`, only when `self.core.z != self.safe`.

## 10. Canned cycles

| Method | Arguments | Meaning |
|---|---|---|
| `drilling(axes, values, retract, pause)` | position, retract height, dwell | drilling (`G81`, or `G82` with a dwell) |
| `pecking(axes, values, retract, delta)` | position, retract height, peck depth | peck drilling (`G83`) |
| `tapping(axes, values, retract)` | position, retract height | tapping (`G84`) |
| `boring(axes, values, retract, pause)` | position, retract height, dwell | boring (`G85`) |
| `cancel_canned_cycle()` | none | cancels the cycle (`G80`) |

A machine that has no canned cycle can return `''` for all of them.

## 11. Other

| Method | Returns (generic mill) |
|---|---|
| `mist()` | `M7` |
| `flood()` | `M8` |
| `stop_coolant()` | `M9` |
| `plane_xy()`, `plane_xz()`, `plane_yz()` | `G17`, `G18`, `G19` |

## 12. Complete minimal example

Save as `postpro/my_mill.py`. It has the required methods and one property (the number of decimals):

```python
class Postpro:
	def __init__(self):
		self.safe = 5.0
		self.decim = 3
		self.en_us = {'description': 'My mill', 'decim': 'Decimals'}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.en_us, 'es-es': self.en_us}

	def set_language(self, value):
		self.lang = self.Language[value]

	def set_core(self, value):
		self.core = value

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'MILL'

	def get_extention(self):
		return 'nc'

	def get_decimal(self):
		return self.decim

	def incrementation(self):
		return -1                      # no line numbers

	def set_safe_position(self, value):
		self.safe = value

	def pass_xy_to_cw_ccw(self):
		return False

	def pass_z_to_cw_ccw(self):
		return False

	def pass_xy_to_linear(self):
		return False

	def pass_z_to_linear(self):
		return False

	def use_safe_pos_before_tool_change(self):
		return False

	def get_disable_z(self):
		return False

	def ijk_relative(self):
		return True

	# one property: the number of decimals
	def get_properties_count(self):
		return 1

	def get_property_description(self, index):
		return self.lang['decim']

	def get_property(self, index):
		return self.decim

	def set_property(self, index, val):
		self.decim = int(val)

	# helpers
	def n(self, val):
		text = ('%.' + str(self.decim) + 'f') % val
		text = text.rstrip('0')
		return text[:-1] if text.endswith('.') else text

	def coords(self, axes, values):
		return ' '.join(a + self.n(v) for a, v in zip(axes, values))

	# output
	def start_output(self):
		return '%\n'

	def stop_output(self):
		return '%'

	def start_program(self):
		return 'G90 G21'

	def stop_program(self):
		return 'M2'

	def initial_position(self):
		return self.core.rapid(['Z'], [self.safe])

	def final_position(self):
		if self.core.z != self.safe:           # avoid repeating a move already done
			return self.core.rapid(['Z'], [self.safe])
		return ''

	def comment(self, text):
		return '(' + text + ')'

	def start_line(self, num):
		return ''

	def stop_line(self):
		return '\n'

	def start_toolpath(self):
		return ''

	def stop_toolpath(self):
		return ''

	def start_single_path(self):
		return ''

	def stop_single_path(self):
		return ''

	def rapid(self, axes, values):
		return 'G0 ' + self.coords(axes, values)

	def linear(self, axes, values):
		return 'G1 ' + self.coords(axes, values)

	def clockwise(self, axes, values):
		return 'G2 ' + self.coords(axes, values)

	def counter_clockwise(self, axes, values):
		return 'G3 ' + self.coords(axes, values)

	def feed(self, speed):
		return 'F' + self.n(speed)

	def pause(self, time):
		return 'G4 P' + self.n(time)

	def tool(self, number):
		return 'M6 T' + self.n(number) if number else ''

	def spindle_speed(self, speed):
		return 'S' + self.n(speed)

	def start_spindle_clockwise(self):
		return 'M3'

	def start_spindle_counter_clockwise(self):
		return 'M4'

	def stop_spindle(self):
		return 'M5'
```

Restart the application: the post-processor appears in the post-processor menu under its category. The example is short on purpose: the compensation, canned cycle and coolant methods are required if your operations use them. Start from `generic_mill.py` for a complete version.
