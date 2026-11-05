# CAD script API reference

*Version française : [CAD-SCRIPTS.fr.md](CAD-SCRIPTS.fr.md).*

This is the reference for anyone who writes a CAD script: a Python file that creates geometry (lines, circles, arcs) in the drawing, at the position of the mouse. It lists every method of the `Cadscript` class and every call of the engine, with arguments, when it is called and an example. The bundled `script/oblong.py` (an oblong hole, with a `tkinter` dialog) is the reference implementation. Post-processors are in [POSTPROCESSOR.en.md](POSTPROCESSOR.en.md).

Covered here: installation, the life cycle of a run, the methods of `Cadscript`, the drawing calls of the engine and a complete minimal example.

## 0. Conventions

### Files and loading

| Item | Rule |
|---|---|
| Location | the `script` folder beside the executable |
| Loading | at startup, each `.py` file is copied to `~/.openpostpro/script` and loaded from there |
| Name | your own (`my_rectangle.py`): the copy **overwrites files of the same name** |
| Class | the file defines a class named `Cadscript` |
| Engine | `cadcore.py` is the engine. It is never loaded as a script. Do not edit it |
| Errors | written to `~/.openpostpro/openpostpro.log` |
| Python | 3.12 or later, installed with "Add Python to PATH" (see the README) |
| `tkinter` | write `import tkinter`: `cadcore.py` already points Python to the Tcl/Tk libraries of the installed Python |

### Who calls whom

```
OpenPostPro (C++)  ->  Cadcore (cadcore.py)  ->  Cadscript (your file)
```

The engine hands itself to your class: it is `self.core`, set through `set_core()`. Your class draws by calling `self.core`; the engine collects the drawing as text and returns it to the application when `run()` ends.

### Types and encodings

- Coordinates are numbers in drawing units (`float`).
- `cw` of an arc is a boolean: `True` for clockwise.
- Language codes: `en-us`, `en-uk`, `fr-fr`, `es-es`.

## 1. Life cycle of a run

```
set_language(lang)    when the script is loaded
set_core(core)        once
get_description() get_category()    to build the Scripts menu
get_property / set_property         loading and saved values
-- when the user launches the script --
core.mouse_x / mouse_y are set      position of the mouse
run()                               the script draws and may open a dialog
-> the drawing returned by the engine is added to the document
```

## 2. Methods of `Cadscript`

| Method | Arguments | Returns | Meaning |
|---|---|---|---|
| `set_language(value)` | `'en-us'`, `'en-uk'`, `'fr-fr'` or `'es-es'` | nothing | selects the dictionary of interface strings |
| `set_core(value)` | the engine | nothing | store it in `self.core`; do not modify |
| `get_description()` | none | string | name shown in the Scripts menu |
| `get_category()` | none | string | menu category, usually `'CAD'` |
| `get_properties_count()` | none | number | number of parameters of the script |
| `get_property_description(index)` | index from 0 | string | label of the parameter |
| `get_property(index)` | index from 0 | `bool`, `int`, `float`, `str` or `list` of `str` | current value |
| `set_property(index, value)` | index, value | nothing | receives the saved value at startup, or the edited one |
| `run()` | none | nothing | creates the geometry through `self.core` |

Properties are saved by OpenPostPro between sessions. The script is responsible for any input window it wants (the bundled example builds a `tkinter` dialog in `run()`, reads the values, then draws). `use_properties()` and `configurate()` exist in `oblong.py` but are not called by the application.

## 3. Engine calls (`self.core`)

### Position of the mouse

| Attribute | Type | Meaning |
|---|---|---|
| `self.core.mouse_x` | `float` | X of the mouse when the script is launched |
| `self.core.mouse_y` | `float` | Y of the mouse when the script is launched |

Use it as the origin of the shape.

### Drawing

| Call | Arguments | Effect |
|---|---|---|
| `self.core.line(x1, y1, x2, y2)` | start, end | adds a line |
| `self.core.circle(x, y, r)` | center, radius | adds a circle |
| `self.core.arc(x1, y1, cx, cy, x2, y2, cw)` | start, center, end, direction | adds an arc from `(x1, y1)` to `(x2, y2)` around `(cx, cy)`; `cw` is `True` for clockwise |
| `self.core.append_output(text)` | raw text | appends text to the drawing output (advanced) |

The engine writes one line of text per element, in this format:

| Element | Text |
|---|---|
| line | `L:x1:y1:x2:y2` |
| circle | `C:x:y:r` |
| arc | `A:x1:y1:cx:cy:x2:y2:clock` where `clock` is `1` for clockwise, `0` otherwise |

`append_output` lets a script write such lines itself. The geometry is added to the drawing when `run()` returns.

## 4. Complete minimal example

Save as `script/my_rectangle.py`. It has no dialog and draws from its two properties:

```python
class Cadscript:
	def __init__(self):
		self.width = 100.0
		self.height = 50.0
		self.en_us = {'description': 'Rectangle', 'width': 'Width', 'height': 'Height'}
		self.fr_fr = {'description': 'Rectangle', 'width': 'Largeur', 'height': 'Hauteur'}
		self.es_es = {'description': 'Rectangulo', 'width': 'Ancho', 'height': 'Alto'}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)   # unknown language: English

	def set_core(self, value):
		self.core = value

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'CAD'

	def get_properties_count(self):
		return 2

	def get_property_description(self, index):
		return self.lang['width'] if index == 0 else self.lang['height']

	def get_property(self, index):
		return self.width if index == 0 else self.height

	def set_property(self, index, val):
		if index == 0:
			self.width = float(val)
		else:
			self.height = float(val)

	def run(self):
		x = self.core.mouse_x
		y = self.core.mouse_y
		w = self.width
		h = self.height
		self.core.line(x, y, x + w, y)
		self.core.line(x + w, y, x + w, y + h)
		self.core.line(x + w, y + h, x, y + h)
		self.core.line(x, y + h, x, y)
```

For a script with its own input window, see `script/oblong.py`: it builds a `tkinter` dialog in `run()`, reads the values, then calls `compute()` to emit lines and arcs.
