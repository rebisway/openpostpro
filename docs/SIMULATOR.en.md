# Simulator and translator reference

*Version française : [SIMULATOR.fr.md](SIMULATOR.fr.md).*

The **Simulation** tab plays a machine program on a block of material, in 3D, before it is sent to a machine. This is the reference for the tab and for anyone who writes a **translator**, the Python module which turns a program (G-code, or another format) into the few simple commands that the simulator plays. The reference implementation is `simulator/gcode.py`; copy it when you start. CAD scripts are in [CAD-SCRIPTS.en.md](CAD-SCRIPTS.en.md), post-processors in [POSTPROCESSOR.en.md](POSTPROCESSOR.en.md).

Covered here: installation of a translator, the Simulation tab, the life cycle of a translation, the three methods of the `Translator` class, every command of the simulator, errors and warnings, the generic G-code translator, and a complete minimal example.

## 0. Conventions

### Files and loading

| Item | Rule |
|---|---|
| Location | the `simulator` folder beside the executable |
| Loading | at startup, each `.py` file is copied to `~/.openpostpro/simulator` and used from there |
| Name | your own (`my_controller.py`), put in `~/.openpostpro/simulator`. The copy at startup **overwrites files of the same name**, so never edit a bundled file in place: copy it under another name |
| Class | the file defines a class named `Translator` with a method `next`. A `.py` file without them is a library shared by other files: it is not listed |
| Choice | the list in the Simulation tab shows *Auto* then the translators of the folder. *Auto* (the default) chooses for each file the translator which reads its extension (the `extensions()` method, below), and `gcode` if none of them says it does. Choosing a translator in the list **forces** it for every file. The choice is kept with the settings; changing it translates the program again |
| Folder | the menu **Simulation / Open the folder of the translators** opens `~/.openpostpro/simulator` |
| Reloading | the module is read again at each load of a program: edit the file, save it, load the program again, no restart |
| Errors | an exception of the translator is written in the log with its Python traceback and the line of the program; the translation goes on with the next line |
| Python | 3.12 or later, installed with "Add Python to PATH" (see the README) |

### Who calls whom

```
OpenPostPro (C++)  ->  Translator (your file)  ->  commands (dictionaries)  ->  player and material (C++)
```

The application reads the text of the program and gives **one line at a time** to your `next()`. Your class returns the commands of that line, and it keeps its own state (position, feed rate, units, modes): the simulator has no memory of the language of the program.

**The translator only translates.** Play, pause, step, going back, going to a line, the speed and the final result belong to the simulator: its buttons work whatever the program says, and a stop or a dwell of the program is a command like the others (section 4).

### Types

- A command is a Python dictionary with a key `'cmd'`; numbers are `float` or `int`, flags are `bool`, names are `str`.
- Coordinates are **absolute, in millimeters, and complete** (`x`, `y` and `z` in every move): the translator resolves relative distances, inches, work offsets and canned cycles; the simulator never needs to know a mode.
- The line given to `next()` is UTF-8 and has no end of line. A program in Windows-1252 or Latin-1 (accents in the comments, for example) is converted by the application: any line which is not valid UTF-8 is taken as one of these 8 bits encodings.

## 1. The Simulation tab

**Loading a program.** *Current program* loads the output file of the document (run the post-processor first, or use **Post-processor / Run and simulate**). *Open...* loads any G-code file, which can also be dropped on the window. The translator is chosen in the list next to the buttons.

**Player.** All these buttons act at any time, whatever the program is doing:

| Button | Effect |
|---|---|
| Play / Pause | plays at the chosen speed, or stops (the space bar does the same) |
| Start over | back to the start, the material is whole again |
| Step | plays up to the end of the next move, then pauses |
| Line | plays the commands of the next line of the program, then pauses |
| Result | goes straight to the end of the program, computing by slices so the window stays alive |
| Speed | x1 is the real speed of the machine (from the feed rates); x2 to x500 are faster. *Full speed* plays as fast as possible |
| A click on a line of the program | goes to that line (the program is played from the start up to it, quickly) and pauses |

The commands of the program are executed as a machine does when the speed is a multiple: a **dwell** (`G4`) waits its time, a **stop** (`M0`, and `M1` if *Optional stop* is on) pauses the player until Play is pressed, `M2` and `M30` end the program. When the simulator goes fast (*Full speed*, *Result*, a click on a line, *Step*) the dwells and the stops are skipped. The time shown is always the time of the machine, computed from the feed rates, and does not depend on the speed of the player. Rapid moves are computed at 5000 mm/min.

**The scene.** Left button turns the view, right button moves it, wheel zooms. *Top*, *Front* and *Iso* are standard views. The boxes show or hide the material, the feed moves (orange), the rapid moves (blue), the tool, and the axes (the origin of the program). The panel on the right is the program, with the line which is played in color (red for a line with an error, yellow for a warning).

**Settings** (box *Settings*):

| Setting | Meaning |
|---|---|
| Shape | flat end mill, ball nose, bull nose, V bit, or plasma torch (it cuts through the material, only while the spindle `M3` or the torch is on) |
| Diameter | of the tool (the kerf of a torch). With *Current program* it starts at the diameter of the first group of the document |
| Corner radius / Angle | of a bull nose / of a V bit (included angle) |
| Top of the material (Z) | height of the top of the block, 0 by default |
| Thickness | `0` is automatic: the deepest cut of the program times 1.25 plus 1 mm, at least 3 mm |
| Precision | step between two columns of the block, as a percent of the radius of the tool. The step is made larger if there would be more than 2 million columns |
| Optional stop (M1) | `M1` pauses the player when on |

The block is made around the moves which cut, with a margin of twice the radius of the tool plus 2 mm. Changing a setting makes the block again and plays the program again up to where it was.

**Messages.** Errors and warnings of the translator are written in the log with the line of the program, counted next to the name of the program, and the lines are colored in the program panel.

## 2. Life cycle of a translation

```
start(options)        once, when a program is loaded (the module is read again)
next(line)            once for each line of the program, in order, numbered from 1 by the application
close()               once, at the end or when the translation is cancelled
```

The translation is done by slices of 10 ms for each frame of the window, with a progress bar and a Cancel button: a program of 100 000 lines is translated in about 2 seconds of Python while the window stays smooth. Your methods must therefore be quick and must not wait for anything. They are all called from one thread, in order.

## 3. Methods of `Translator`

| Method | Arguments | Returns | Meaning |
|---|---|---|---|
| `start(options)` | dictionary of numbers (none are given today) | dictionary of information, or `None` | forget everything: position, modes, feed, units. The only information read is `'unit'` (`'mm'` or `'inch'`, the unit of the program) |
| `next(line)` | one line of the program, without end of line | **list** of commands, `[]` if the line gives none | translate one line. Your state changes here |
| `close()` | none | nothing | release what you use. Called even after an error |
| `extensions()` (optional) | none | list of strings | the extensions of the files the translator reads, without the dot (`['plt', 'hpgl']`), so *Auto* can choose it. Without this method the translator is never chosen by itself |

An exception raised by `next()` is not fatal: it becomes an error message on that line, and the next line is translated. An exception in `start()` stops the loading and is written in the log.

## 4. Commands

Every dictionary has `'cmd'`. The application adds the number of the line of the program, so you do not give it.

| `cmd` | Fields | Meaning |
|---|---|---|
| `rapid` | `x y z` | rapid move to the point |
| `linear` | `x y z f` | move at the feed rate `f` in mm/min (a missing or null feed is taken as 1 mm/min) |
| `arc` | `x y z cx cy cz cw f plane` | arc at the feed rate. `x y z` is the end, `cx cy cz` the **absolute** center, `cw` is `True` for clockwise, `plane` is `'xy'`, `'xz'` or `'yz'` |
| `dwell` | `seconds` | pause of the program (`G4`): the player waits that time, which counts in the time of the machine |
| `tool` | `number` | tool change, shown in the tool number |
| `spindle` | `state` (`'cw'`, `'ccw'`, `'off'`), `rpm` | spindle. A plasma torch cuts while it is not `'off'` |
| `coolant` | `state` (`'flood'`, `'mist'`, `'off'`) | coolant |
| `torch` | `state` (`'on'`, `'off'`) | plasma torch, for a translator which has its own command for it |
| `stop` | `optional`, `end` | stop of the program. `end` (`M2`, `M30`) ends the program, what follows is not played. Without `end`, the player pauses until Play is pressed, unless `optional` is set and *Optional stop* is off |
| `message` | `level` (`'info'`, `'warning'`, `'error'`), `text` | a message for the person, written in the log |

Other fields of a command (for example `a`, `b`, `c` for rotary axes) are ignored: the simulator shows a 2.5 axes machining in X, Y, Z. A command whose `'cmd'` is not in this table is noted once and ignored, so a translator can send new commands before the simulator knows them.

**Arcs.** The plane gives the two axes of the circle and their direction, as G-code does: `'xy'` (`G17`) turns from X to Y, `'xz'` (`G18`) from Z to X, `'yz'` (`G19`) from Y to Z. A positive angle is counter-clockwise (`cw` is `False`) seen from the positive side of the third axis. The start is the position after the previous command. If the third axis changes between the start and the end, the move is a helix. An end point equal to the start is a full circle. The translator converts the arcs given by a radius into a center. The center and the end should be at the same distance: if they are not, the radius changes linearly from the start to the end.

**Feed.** The feed rate is in each move, in millimeters per minute: the translator converts inches and revolutions.

## 5. Errors and warnings

A message is a command: `{'cmd': 'message', 'level': 'warning', 'text': '...'}`. It is written in the log with the line which gave it, and the program goes on, so all the problems of a file are seen in one pass. Warn **once** for a problem which repeats, or the log is flooded (see `warn_once` in `gcode.py`). An error is for what makes a line meaningless (an arc with no center), a warning for what is simulated differently (a code which is ignored), an information for the program's own messages.

## 6. The generic G-code translator (`gcode.py`)

| Codes | Meaning |
|---|---|
| `G0` `G1` `G2` `G3` | rapid, linear, clockwise and counter-clockwise arcs (center `I J K` relative, or radius `R`, with a helix) |
| `G4` | dwell, with `P` in seconds or `X` in seconds |
| `G17` `G18` `G19` | planes |
| `G20` `G21` | inches, millimeters |
| `G90` `G91`, `G90.1` `G91.1` | absolute or relative distances, absolute or relative arc centers |
| `G94` `G95` | feed in mm/min or per revolution (`G93` is taken as `G94`, with a warning) |
| `G80` to `G89`, `G73` | canned cycles, developed into moves (`G81` `G82` `G83` `G73` `G85` `G86` `G89`, and `G84` as `G81`), with `G98` `G99` |
| `M3` `M4` `M5` | spindle, with `S` |
| `M6` and `T` | tool |
| `M7` `M8` `M9` | coolant |
| `M0` `M1` `M2` `M30` | stops and end |
| comments | `(text)`, `; text`, `(MSG, text)` shows a message; `%` and lines starting with `/` are skipped; line numbers `N` are ignored |
| accepted without effect | `G40` `G43` `G49` `G54` to `G59` `G61` `G64`, `M48` `M49` |
| `G41` `G42` | the cutter radius compensation is **not applied**: the programmed path is shown (one message) |
| `G28` `G30` `G92` and others | a warning (once) and the code is ignored |

## 7. Complete minimal example

A format with one point per line, `X Y Z` and an optional feed. Save as `~/.openpostpro/simulator/points.py` (the file is `tests/sim/points_translator.py` in the sources), then choose `points` in the list of the Simulation tab and open a file of points:

```python
class Translator:
	def start(self, options):
		self.first = True
		return {'unit': 'mm'}

	def next(self, line):
		line = line.strip()
		if not line or line.startswith('#'):
			return []
		try:
			values = [float(v) for v in line.replace(',', ' ').split()]
		except ValueError:
			return [{'cmd': 'message', 'level': 'error', 'text': 'not numbers: ' + line}]
		if len(values) < 3:
			return [{'cmd': 'message', 'level': 'warning', 'text': 'a point needs X Y Z: ' + line}]
		x, y, z = values[:3]
		if self.first:
			self.first = False
			return [{'cmd': 'rapid', 'x': x, 'y': y, 'z': z}]
		feed = values[3] if len(values) > 3 else 600.0
		return [{'cmd': 'linear', 'x': x, 'y': y, 'z': z, 'f': feed}]

	def close(self):
		pass
```

## 8. Limits

- The simulator checks a program, it does not model the machine: no axes limits, no acceleration, no collision of the holder. The cutter radius compensation (`G41` / `G42`) is not applied.
- Three axes X, Y, Z. The commands of a 4 or 5 axes program can be played in X, Y, Z, but the material is then not reliable.
- The tool has one shape for the whole program (the tool changes are shown, not simulated).
- The surface of the material is a grid of columns: a very steep wall shows the steps of the grid, and a narrow cut in a very large block may look dotted when the grid is made larger to stay under 2 million columns.
