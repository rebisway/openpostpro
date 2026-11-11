# OpenPostPro user manual

*Version française : [MANUAL.fr.md](MANUAL.fr.md).*

OpenPostPro is a CAD/CAM program: you draw a part (or import a drawing), prepare the machining operations, generate the machine program with a post-processor, then check it in 3D before sending it. To program in Python, see the guides listed in chapter 13.

## Contents

1. [The path from drawing to machine](#1-the-path-from-drawing-to-machine)
2. [First start and layout of the interface](#2-first-start-and-layout-of-the-interface)
3. [The menus](#3-the-menus)
4. [Drawing](#4-drawing)
5. [Machines and tools](#5-machines-and-tools)
6. [The operations](#6-the-operations)
7. [The types of machining](#7-the-types-of-machining)
8. [The post-processor](#8-the-post-processor)
9. [The scripts](#9-the-scripts)
10. [The simulator](#10-the-simulator)
11. [Step-by-step examples](#11-step-by-step-examples)
12. [Automatic nesting](#12-automatic-nesting)
13. [Going further](#13-going-further)

## 1. The path from drawing to machine

1. **Draw**: sketch the shapes, or import a DXF, SVG, Gerber or Excellon file, or run a CAD script.
2. **Prepare the machine and the tools**: choose the machine (table, zero, direction of the axes) and the tools (diameter, feed rates).
3. **Create the operations**: an operation is a tool with its depth, pass depth and feed rates; you add machining to it (follow a line, pocket, drilling...).
4. **Generate** the output file with the post-processor of your controller.
5. **Simulate** the file on a block of material in 3D, and fix what you see.
6. **Send** the file to the machine.

The diagram shows who does what: green boxes are Python code you write, purple ones are Python code supplied with the program (the engines), grey ones are the application itself.

```mermaid
flowchart TB
  subgraph CAD["CAD"]
    s["CAD script<br/>(user Python)"]:::user -->|"core.line(...), core.circle(...)"| cc["cadcore.py<br/>(provided engine Python)"]:::eng
    cc -->|"text commands<br/>M:id:A:... D:id G:text Y:layer"| doc["Application: reads the commands,<br/>changes the Document"]:::app
    doc --> par["Toolpaths<br/>(pockets, outlines)"]:::app
  end
  subgraph PP["Post-processor"]
    app2["Application: walks<br/>the toolpaths"]:::app -->|"start_loop, rapid, linear..."| pc["postcore.py<br/>(provided engine Python)<br/>state: position, line number, feed"]:::eng
    pc <-->|"calls the methods,<br/>gets text back"| post["Post-processor<br/>(user Python, one per machine)"]:::user
    pc --> out["Output file<br/>(G-code, HPGL)"]:::app
  end
  subgraph SIM["Simulator"]
    app3["Application: gives<br/>one line at a time"]:::app -->|"start, next, close, extensions"| tr["Translator<br/>(user Python, one per format)"]:::user
    tr -->|"dictionaries"| cv["Application: simple commands<br/>(sim_translator.cpp)"]:::app
    cv --> pl["Player"]:::app --> mat["Material model<br/>(grid or tiles)"]:::app --> vue["3D view"]:::app
  end
  par --> app2
  out ==>|"border: machine | check"| app3
  classDef user fill:#99d5cf,stroke:#0f766e,color:#052e2b
  classDef eng fill:#d9ccf2,stroke:#6d4fc2,color:#2a1a5c
  classDef app fill:#d9dde2,stroke:#5b6670,color:#1f2933
```

The output file links the generation of the program (the post-processor) to its check (the simulator).

### The example project

The whole manual uses a 400 × 200 mm plate, machined on a 600 × 400 mm router: an outline with rounded corners, four mounting holes, a rosette (two engraved rings, a 12-point star hollowed out as a pocket) and two slots. The shapes are in five **layers** (Outline, Holes, Rings, Star, Slots) and machined by five **operations**:

| Operation | Tool | Depth | Machining |
|---|---|---|---|
| Star pocket | T2, 3 mm end mill | −3 mm | pocket with offsets |
| Rings engraving | T1, 1 mm end mill | −0.5 mm | follow the line, centered |
| Slots | T2, 3 mm end mill | −3 mm | zigzag pocket |
| Holes | T2, 3 mm end mill | −4 mm | four drillings |
| Cut out | T3, 6 mm end mill | −8 mm | follow the line on the outside, four tabs |

## 2. First start and layout of the interface

At the very first start, a **Welcome** window asks for the **language** of the interface and the default **unit** of new documents (millimetre or inch). The language changes as soon as you pick it, and **Ok** confirms. You can change both choices later in **Edit / Preferences**.

![The OpenPostPro screen with the example project](img/ui-overview.en.png)

The screen is made of:

- **The menu bar** at the top (chapter 3).
- **The module tabs**, under the menu bar: **Drawing** (CAD and CAM), **Output** (the generated program), **Simulation** (the 3D simulation) and **Log** (events, warnings and errors, for example those of a script or a post-processor). The log can also be shown at the bottom of the screen (**Display / Log**).
- **The Project window**, top left, with two tabs: **Layer** (the layers and their shapes) and **Operation** (the operations and their machining).
- **The properties window**, just below: it shows the settings of what is selected in the project or in the drawing.
- **The drawing area**, in the middle. Left button: select and draw; right button: move the view; wheel: zoom. The yellow rectangle is the table of the chosen machine, and the mark at the bottom left is the zero of the machine.
- **The icon bars**: at the top center (display) and on the right (drawing, modification, machining), see below.
- **The status bar**, at the bottom: position of the mouse in the drawing (in the unit of the document) and zoom level.

### The Project window

![The project: layers and shapes](img/ui-layers.en.png)

The **Layer** tab lists the layers (the triangle folds or unfolds) and their shapes. A click selects a shape and its properties appear below; **Ctrl-click** adds a shape to the selection and **Shift-click** removes one. The buttons **+**, **−**, **▲** and **▼** add, remove and move layers. A layer has a name and a colour, and can be visible or hidden, locked or not. The shapes of a locked layer cannot be moved, edited, deleted, cut or extended, but the magnet still snaps to them. The shapes of a hidden or locked layer cannot be selected with the mouse (see "Groups", chapter 4).

![The operations and their machining](img/ui-operations.en.png)

The **Operation** tab lists the operations and, under each, its machining (pocket, follow the line, drilling...). The buttons **+**, **−**, **▲** and **▼** add, remove and reorder the operations; **the order of the operations is the order of the program**.

### The icon bars

<img src="img/ui-toolbar-top.en.png" alt="Display bar" width="240">

The top bar acts on the display: **fit the drawing** in the window (Home key), **show or hide all the toolpaths**, show the machining **direction**, show their **start**, and **update**: recompute the texts linked to a curve, then the toolpaths from their shapes (useful after a change in the drawing). A **Group** icon follows them: it gathers the selection into a group, then becomes **Ungroup** (checked) when the selection is a group (see "Groups", chapter 4). When a text is selected, a **Link** icon appears after it: it links the text to a curve and stays checked while it is linked; a click unchecks it and removes the link.

<img src="img/ui-toolbar-right.en.png" alt="Drawing tools bar" width="70">

The right column has three series of icons, separated by a line:

1. **Create**: point, line, circle, arc, ellipse, polyline, text.
2. **Modify**: move the origin, offset, symmetry, scale, rotation, chamfer / fillet, extend, cut.
3. **Machine**: move to position, drilling, follow the line, go around the line, pocket machining.

A greyed-out icon is not available: for example, the current operation uses a torch, which neither drills nor clears a pocket. Leave the mouse on an icon for a moment to show its name.

## 3. The menus

### File

![File menu](img/menu-file.en.png)

**New**, **Open** and **Save** (`.opp` documents); **Open recent** lists the last ten documents; **Examples** opens the example projects of the manual, as a copy to save under another name. **Import** opens a DXF file; **Export** writes, in DXF format, the shapes (only the selection, if there is one). SVG, Gerber and Excellon import and export are scripts (chapter 9). **Quit**: Ctrl+Q.

### Edit

![Edit menu](img/menu-edit.en.png)

**Undo / Redo** (Ctrl+Z, Ctrl+Y), **Cut / Copy / Paste / Delete**. **Duplicate** copies the selection a little further; **Array copy...** makes a grid of copies (columns, rows and spacings). The **Select** submenu has: *All* (Ctrl+A), *The objects of the current layer*, *The layer* (picks a layer by its name), *Contour* (selects all the shapes connected to the selection) and *Invert selection*. **Preferences** opens the preferences (language, unit, look).

### Drawing

![Drawing menu](img/menu-drawing.en.png)

Add, delete and rename a layer (F2); **Check the drawing** (see chapter 4); reverse the direction of the selected objects; convert the whole drawing to inches (or millimeters). With a selection, the menu also offers to cut at intersections, the boolean operations (union, intersection, subtraction), the conversion to a spline, and the link of a text to a curve. It also offers **Group / Ungroup** (see "Groups", chapter 4).

### Machining

![Machining menu](img/menu-machining.en.png)

Add, delete and rename an **operation**; open the table of **machines** and the table of **tools** (chapter 5); create the machinings (move to the position, drilling, follow the line, go around the line, pocket), as with the icons; delete the selected machining; **Automatic radius compensation** and **Organize the toolpaths** (chapter 7); **Nesting...** (chapter 12).

### Post-processor

![Post-processor menu](img/menu-postprocessor.en.png)

**Generate** writes the output file with the chosen post-processor (its name is in parentheses); **Run and simulate** generates it and plays it in the simulator; **Configure** opens its settings; **Manage** gathers the editing, the cloning (an editable copy) and the opening of the folder; **Select** chooses the post-processor by category. See chapter 8.

### Scripts

![Scripts menu](img/menu-scripts.en.png)

The delivered scripts are sorted by category: *Import*, *Modify*, *Generation*, *Geometry*, *Examples*, *Export*. **Clone a script** makes an editable copy. See chapter 9.

### Display and Help

![Display menu](img/menu-display.en.png)

**Display** fits the drawing (Home), shows or hides the grid, the machinings, their direction and their start, chooses the style of the interface (classic, dark, light) and shows or hides the Output and Log windows. **Help** opens the online manual, the list of keyboard shortcuts, the OpenPostPro web site and the project site (GitHub), and gives the version and the build date in **About**.

![The preferences](img/win-preferences.en.png)

In the preferences: open the last file at start, small axes, grid, construction lines, crosses instead of points, display of the machining direction and start, folder of Python (`PYTHON_PATH`, if Python is not in the PATH), language and default unit; the **Colour** tab sets the colours of the drawing, the toolpaths and the simulation.

## 4. Drawing

### Choosing and entering

Click an icon of the right column, then click in the drawing. A **magnet** snaps the mouse to the notable points of the drawing (ends, centers, intersections); a small square shows the point aimed at. Numeric values are typed in the properties window, which shows the shape as soon as it exists. **Esc** cancels the creation in progress.

### The shapes

| Icon | Shape | How to create it |
|---|---|---|
| ▪ | **Point** | one click |
| ╱ | **Line** | two clicks: start and end |
| ◯ | **Circle** | the center, then a point of the circle |
| ◜ | **Arc** | *by the centre* or *by three points* (chosen in the window which opens) |
| ⬭ | **Ellipse** | a few clicks, then the values in the properties (center, major and minor axes, angles) |
| ⬡ | **Polyline** | a series of clicks; the **space bar** or the *Add* button confirms |
| T | **Text** | one click, then the text and the font in the properties |

A text has a font, a height, bold and italic, a position relative to the line (above, aligned, below), a spacing between letters and a mirror. It can **follow a curve** (line, arc, circle or a chain of connected shapes): select the text then **Drawing / Link the text to the object** and click the curve; the start of the text is set with a slider.

### The properties of a shape

![Properties of an arc](img/ui-prop-arc.en.png)

Each shape shows its dimensions in numeric fields you can edit: a **line** has its two points P1, P2 and its length; a **circle** its center, radius and diameter; an **arc** its start, end, center, radius, angle, direction, length and chord. Points are written `x;y`.

![Properties of a circle](img/ui-prop-circle.en.png)


### Modifying

The icons of the second series act on the selection:

- **Move the origin**: chooses the point which becomes the zero of the drawing.
- **Offset**: copies the selection at a given distance (on each side).
- **Symmetry**: mirror about an axis you point out.
- **Scale**: enlarges or reduces by a factor.
- **Rotation**: turns by the angle you enter around a point.
- **Chamfer / fillet**: rounds or cuts a corner between two lines (the radius is set in the window which opens).
- **Extend**: lengthens a line or an arc, from the end nearest to the mouse, up to the nearest shape; the result is shown under the mouse before the click.
- **Cut**: cuts a line, arc, circle, polyline or ellipse at the clicked point (the magnet stays on) or at the crossing of the shapes under the click.

### Groups

A **group** gathers shapes under a name: clicking, hovering over, moving, turning or mirroring one shape of a group acts on the whole group. A group is how you declare that a set of shapes is **one part** (an outline and its holes, for example): the nesting (chapter 12) works on groups.

- **Group**: select at least two shapes, then click the **Group** icon of the top bar, or **Drawing / Group**. The group is called *Block_1*, *Block_2*... ; **F2** on a group of the project list renames it.
- **Ungroup**: when the selection is a group, the same icon (checked) and the same menu entry release its shapes.
- In the **Project** window, the shapes of a group are no longer listed one by one: the group appears instead, with its number of shapes (`[Plate] (9)`), under the layer of its first shape. Each shape stays in its own layer.
- A group is copied and pasted as a whole.
- Groups are saved in the `.opp` file.

The shapes of a **hidden** or **locked** layer are selected neither by a selection rectangle, nor by a click, nor by *Select all* or *Invert selection*. The **Edit / Select / The layer** submenu is not affected: use it to pick the shapes of a locked layer to machine them.

### Checking the drawing

**Drawing / Check the drawing** looks for problems which get in the way of machining and marks them on the drawing: shapes of **zero length**, **duplicate** shapes and **free ends** (open contours). A "follow the line" or "pocket" machining expects closed contours. The log shows "the drawing is ready for machining" when nothing is found; *Clear the check markers* removes them.

### Import and export

**File / Import** reads a DXF: lines, points, circles, arcs, ellipses, polylines, splines (B-splines and NURBS), texts and blocks (with position, scale and rotation). Ends less than 0.01 apart are joined. **File / Export DXF** writes an AutoCAD R12 DXF. The scripts of **Scripts / Import** and **Export** read and write SVG, Gerber (RS-274X) and Excellon.

## 5. Machines and tools

### The machines

![The machines window](img/win-machines.en.png)

**Machining / Machines...** opens the table of machines: the list on the left, the settings of the chosen machine on the right. A machine gives:

- the **travel** in X, Y and Z: the surface of the table is drawn under the drawing (yellow rectangle), and a toolpath which leaves it is flagged "Out of the area of the machine";
- the **origin** (the zero of the machine): bottom left, bottom right, top left, top right or center;
- the **direction of the axes**: "X counts from right to left", "Y counts from top to bottom", "Z counts downwards". The directions are the ones of the operator in front of the drawing. A reversed axis changes the sign of the program values; a single reversed axis changes the direction of the arcs. The post-processor and the simulator take this into account.

**Apply** assigns the machine to the document; **No machine** removes it.

### The tools

![The table of tools](img/win-tools.en.png)

**Machining / Tools...** opens the table of tools: the list (T1, T2...) and the settings of the chosen tool:

| Field | Role |
|---|---|
| Number | the number `Tn` of the program; unique |
| Name | free text |
| Type | end mill, plasma torch, water jet, wire EDM |
| Diameter | diameter of the cutter, or **kerf width** of a torch |
| Feed, Plunge | cutting feed and plunge feed (mm/min) |
| Pass depth | depth removed at each pass |
| Speed, Clockwise, Cooling | spindle and coolant (cutter only) |
| Teeth, Material | for **Compute from the table**: the program suggests the speed and feed from the diameter, the number of teeth and the material (indicative values for carbide cutters, to adjust) |

Changes are saved at once. The details (torches, oblique plunge, cutting table) are in [TOOLS-MACHINES.en.md](TOOLS-MACHINES.en.md).

## 6. The operations

An **operation** gathers the settings of one tool and the machining which uses it. Create one with **Machining / Add an operation**, then choose it in the **Operation** tab: its properties appear.

![Properties of an operation](img/ui-group.en.png)

| Setting | Effect |
|---|---|
| Visible | shows or hides the toolpaths of the operation in the drawing |
| Generate | when checked, the operation is written in the program; unchecked, it is ignored |
| Colour | colour of the toolpaths in the drawing |
| Clockwise path | direction of the closed contours (clockwise or counter-clockwise) |
| Tool | the tool of the table; its settings are copied into the operation, which you can then edit |
| Radius | radius of the tool (used by offsets, pockets and compensation) |
| Speed, Clockwise | spindle speed and direction |
| Cooling | none, flood or mist |
| Feed, Plunge | cutting feed, plunge feed |
| Length comp. | number of the length offset (leave empty otherwise) |
| Origin | Z height of the surface of the material |
| Depth | final depth (negative, under the origin) |
| Pass depth | depth removed at each pass: the depth is reached in several passes |
| Pre-command, Post-command | free text written before and after the operation (for example M8 / M9) |
| Start | where the closed paths begin: normal, or shifted |

The machining of an operation is done in the order of the list. **Machining / Delete the machining** removes the selected one.

### Creating a machining

1. Choose the operation in the **Operation** tab.
2. Select the shapes (a click, a rectangle, or **Edit / Select / The layer**).
3. Click the icon of the machining in the right column.

The machining appears under the operation, with its own properties. If you change the shapes afterwards, the *update* button of the top bar recomputes the toolpaths.

## 7. The types of machining

### Follow the line

![Properties of "follow the line"](img/ui-follow.en.png)

Makes the tool follow the selected shapes. One machining is created **for each shape or chain of connected shapes**: the two rings of the example give two machinings. Settings:

- **Clockwise movement**: direction of travel.
- **Radius compensation**: *Centered* (the axis of the tool is on the line, for engraving for example), *Outside* or *Inside* (the tool stays outside or inside the shape: this is cutting out a contour). The side is deduced from the direction of travel. The post-processor writes G41 / G42, and for a torch the kerf is compensated the same way.
- **Start position**: where the path begins on the shape.
- **Entry and exit**: a line or a quarter circle, to enter and leave the material without marking the part.

On a selection with **holes** (contours inside another one, at any depth: a hole, an island in the hole, a hole in the island...) or on a group, the entry and the exit are **inside the holes** and outside the outer contours and the islands: the **Inside** box is checked for the holes and unchecked for the others. It only appears when the **Start** of the operation is *Line* or *Arc* (chapter 6). The radius compensation stays *Centered*; set it separately, machining by machining or with the menu below.

#### Automatic radius compensation and order of machining

Two entries of the **Machining** menu set several machinings at once. They act on the selected machinings, otherwise on those of the selected operations, otherwise on all of them.

- **Automatic radius compensation**: for every closed *follow the line* and *go around the line*, the tool is placed **outside** an outer contour or an island and **inside** a hole. The level of a contour is the number of contours around it in its operation. This replaces the choice made when the machining was created. Ctrl+Z undoes it in one step.
- **Organize the toolpaths**: sorts the machinings of each operation in the order in which a part is machined: each contour comes after the contours it contains, at any depth (the islands, then the holes, then the outer contour). Only consecutive contours are sorted: drillings, pockets and moves keep their place, and contours of the same depth keep their order. Ctrl+Z does not undo this.

The log shows how many machinings changed.

![The tabs of a cut-out](img/ui-follow-tabs.en.png)

On a closed contour, the **tabs** (holding bridges) keep the part in place so that it does not move at the end of the cut: *Tabs count*, *Length*, *Height*, then *Selected tab* and its *position*, which you move with a slider. The tool rises to the height of the tab and goes down again after it. In the program, the cut-out of the plate reads:

```
(TOOLPATH [Follow the line_1])
G41
F530
G1Z-8
F1326
G2X100Y125J25
G1Y230.305054
Z-7.7
Y230.425049
Z-8
```

`G41` is the radius compensation, `Z-7.7` the passage over a tab (height 0.3 mm).

### Pocket machining

Clears the inside of a closed shape, **islands** respected (the central disc of the star stays in place). Three strategies, chosen in *Mode*:

![Properties of a pocket](img/ui-pocket.en.png)

| Mode | Principle |
|---|---|
| **Zigzag** | parallel lines inside the shape, at any angle; settings: *Stepover* (distance between two lines), *Angle*, and *Contours* (with or without going along the contour) |
| **Offset** | concentric contours which shrink toward the center |
| **Spiral** | a spiral inward or outward |

*Finishing* adds a last finishing pass along the contour, machined last. A pocket uses the settings of the operation (radius, depth, pass depth): a 3 mm deep pocket with a 1.5 mm pass depth is made in two passes. The icon is greyed out when the operation uses a torch.

### Drilling

![Properties of a drilling](img/ui-drilling.en.png)

Click the **drilling** icon, then click the points to drill in the drawing (the magnet aims at the centers of circles); each click creates a drilling. Settings: the **point**, the **mode** (*Manual*, *Drilling*, *Peck drilling*, *Tapping*, *Boring*), the **retract** (height of withdrawal) and the **dwell**. The modes use canned cycles of the program:

```
(TOOLPATH [Drilling_1])
G0Y280
G81G99Z-4R0
```

Press **Esc** after the last point to leave the creation of drillings.

### Move to position

Adds to the operation a positioning move to a point clicked in the drawing.

### Go around the line and spiral

*Go around the line* creates interior or exterior offsets of the chosen shape; *spiral* creates spiral toolpaths.

## 8. The post-processor

The **post-processor** turns the machining into a program for your controller. It is written in Python and is edited without touching the program (see [POSTPROCESSOR.en.md](POSTPROCESSOR.en.md)).

### Choosing

**Post-processor / Select** lists the post-processors by category: *Mill / Router* (generic milling), *Plasma* (generic plasma cutting) and variants for controllers: **Mach3** (`.tap` file, absolute arc centers with `G90.1`), **Num 1060** (spaces between the words, no `G64`, dwell with `F`), **Burny 2.5** (every arc gives `X Y I J`) for milling and for plasma, and **HPGL** for plotters and vinyl cutters (the tool number is the pen). They are starting points: read the output and adapt it to your machine before use.

### Configuring

![The post-processor settings](img/win-postconfig.en.png)

**Post-processor / Configure** opens the settings of the chosen post-processor. For generic milling:

| Setting | Effect on the file |
|---|---|
| Numbering lines | adds an `N` number at the start of each line |
| Increment | step between two numbers (`N5`, `N10`, ...) |
| Condensed | no space between the words: `G1X10Y5` instead of `G1 X10 Y5` |
| Decimals number | maximum number of decimals of the written numbers |
| File extension | extension of the output file (`nc`, `tap`...) |
| Output comments | writes the `(OPERATION [...])` and `(TOOLPATH [...])` lines |
| File header, File footer | characters written at the start and end of the file (often `%`) |
| IJK Relative | the center of the arcs is given relative to the start (checked), or absolute |

Plasma post-processors add torch settings (ignition commands, pierce time, reference, Z axis handling). The settings are kept from one session to the next.

### The order of the contours

So that a part which is cut out does not move before the end of the cut, the post-processor machines **the contours which are inside another one before it**, at any depth: the islands, then the holes, then the outer contour of the part. The *follow the line*, *go around the line* and drillings which follow each other are still sorted to shorten the rapid moves, but a contour is never machined before the ones it contains. With several passes in depth, the rule is kept at each pass. The **Machining / Organize the toolpaths** menu (chapter 7) puts the list of the operation in the same order, to see it in the Project window.

### Generating

**Post-processor / Generate** writes the file at the output location of the document (its extension comes from the *File extension* setting), and the **Output** tab shows its content:

![The Output tab](img/ui-output.en.png)

The program of the example starts like this:

```
%
(Created with OpenPostPro 0.6.0)
G90G64G94G21
G0Z5
X0Y0
(OPERATION [Star pocket])
M6T2
S20000
(TOOLPATH [Pocket machining_1])
X317Y200
F440
G1Z-1.5
F1100
G3I-17
```

It shows three levels: the operation (`M6T2`: tool 2; `S20000`: the spindle), the machining (the comment), then the moves. The operations are written in the order of the list of the **Operation** tab, and the tool is changed at each operation.

**Post-processor / Clone** copies a delivered (read-only) post-processor under a new name so you can adapt it to your controller with the built-in editor (**Compile** checks the syntax).

## 9. The scripts

A **script** is a small Python program which creates or modifies shapes in the drawing: a rectangle, a gear, an array, a fillet, a bitmap tracing... The delivered scripts are in the **Scripts** menu:

- **Generation**: rectangle, hexagon, star, oblong hole, wheel spokes, exterior spur gear, airfoil from a coordinate file, grid of holes.
- **Modify**: linear and polar array of the selection, chamfer, fillet.
- **Geometry**: center of gravity, scale of circles.
- **Import / Export**: SVG, Gerber and Excellon (pads, tracks, regions, holes), bitmap tracing with Potrace.
- **Examples**: small scripts to learn from (middle of lines, rectangle...).

To run a script, choose it in the menu; a window asks for its parameters (for example the width and height of a rectangle), and **Ok** runs it. The script receives the selection, the position of the mouse and the unit of the document; the shape is created at the position of the mouse. **Each run is undone in a single step** (Ctrl+Z).

The delivered scripts are read-only: **Scripts / Clone a script** makes an editable copy, with the built-in editor (**Save**, **Compile**). A Python error is written in the log with the traceback and the line. To write a script, see [CAD-SCRIPTS.en.md](CAD-SCRIPTS.en.md).

## 10. The simulator

The **simulator** plays the program on a block of material in 3D, before it is sent to the machine. **Post-processor / Run and simulate** generates the program of the document and opens it in the **Simulation** tab; **Current program** reloads the output of the document, **Open...** loads any G-code file (a file can also be dropped on the window).

![The simulation of the example, result](img/sim-result.en.png)

The screen reads as follows:

- **The 3D view**, in the middle: the block of material, the feed moves (green) and rapid moves (red), the tool. Left button: turn; right button: move; wheel: zoom.
- **The cards**, on the left: elapsed time and length travelled, feed rate, tool, X Y Z position, state (cutting, spindle, coolant, paused) and dimensions of the machined area.
- **The program**, on the right: the line being played is in colour (red for an error, yellow for a warning); a click on a line goes to that line.
- **The player buttons**, at the bottom: play / pause (space bar), back to the start, **step** (up to the end of the next move), **line** (up to the end of the next line) and **result** (goes to the end of the program). The slider on the right sets the speed, from ×1 (the real speed of the machine) to ×500, or full speed.
- **The top bar**: the translator (*Auto*, or a forced translator), the name of the program, the number of commands, and the total time.

![Top view](img/sim-top.en.png)

The icons of the bar above show or hide the **material**, the **feed** and **rapid** moves, the **tool** and the **axes**, and choose the **top**, **front** or **iso** view.

![The simulator settings](img/sim-settings.en.png)

The last group of icons opens the **settings**: the shape of the tool (flat end mill, ball nose, bull nose, V bit, plasma torch), its diameter, the corner radius of a bull nose or the angle of a V bit, the top of the material (Z), the thickness (0 = automatic), the precision as a percent of the radius of the tool, and the optional stop `M1`. Changing a setting makes the block again and plays the program again.

The simulator plays the programmed path as the controller will follow it, reports the errors and warnings of the program and computes the machining time from the feed rates. It is not a complete model of the machine: the cutter radius compensation (G41 / G42) is not applied, and the axes A, B, C are ignored. The translator (Python) which reads the program is chosen from the extension of the file, or forced in the list; see [SIMULATOR.en.md](SIMULATOR.en.md) to write your own.

## 11. Step-by-step examples

Three example projects complete the plate: `examples/medal.en.opp`, `examples/sign.en.opp` and `examples/tray.en.opp` (open them with **File / Open**). They illustrate **text pockets**, **text around a circle**, **tabs**, **shifted starts** and **pockets with islands**. They use the same router table and the same tools as the plate.

### Example A: a medal, text around a circle

![The medal: round text, central text and outline](img/ex-medal-view.en.png)

The drawing has four layers: **Edge** (the outer circle, which will be cut out), **Guide** (the grey circle, radius 88 mm, which the text wraps around), **Ring** (the engraved circle in the middle) and **Text** (the round text and the central text "OPP").

**1. The text around the circle.**

1. Create the guide circle, then a text with the **T** tool (Arial, bold, height 16 mm).
2. Select the text, choose **Drawing / Link the text to the object** (or the icon of the top bar) and click the guide circle. The text follows the curve; its letters keep their size.
3. In the properties of the text, **Position** chooses the side: *Above* puts the letters outside the circle, *Below* inside. **Start** moves the start of the text along the curve, **Spacing** spreads the letters. On a closed curve the text goes round; on an open curve it goes on in a straight line past the end.
4. The text follows its curve when the curve is moved with the mouse or when the origin or the unit is changed; in the other cases, the *update* button of the top bar reads the curve again. A text shorter than the circle leaves a gap: the example repeats its pattern to go all the way round.

**2. The text pockets.** Create the operation *Text pockets* (tool T1, 1 mm end mill, depth −0.8 mm). Select both texts (**Edit / Select / The layer / Text**), then the pocket icon, mode **Zigzag**.

![Text pockets of the medal](img/ex-medal-textpocket.en.png)

The letters are hollowed out, keeping their outlines: the tool must be **smaller than the strokes of the letters**, otherwise they are not cleared. With a 1 mm cutter, a 16 mm high text is cleared without trouble.

**3. A shifted start.** The operation *Ring engraving* follows the central circle (*follow the line*, *Centered*). A circle has no natural beginning: the **start** is the place where the tool goes down. The *Start position* moves it along the contour with a slider; here it is shifted, which can be seen from the direction arrow and the start mark on the drawing.

![Ring engraving, shifted start](img/ex-medal-ring.en.png)

**4. The cut-out with tabs.** The operation *Cut out* (tool T3, 6 mm end mill, depth −6 mm) follows the outer circle, *Outside*, with **three tabs** 12 mm long and 2 mm high. They are spread along the contour, and each one can be moved with the *Tab position* slider.

![Cut-out of the medal, three tabs](img/ex-medal-cut.en.png)

### Example B: a sign

![The sign: text pocket, engraved frame, holes, outline](img/ex-sign-view.en.png)

Four layers: **Outline**, **Text** ("WORKSHOP"), **Frame** and **Holes**. Four operations:

| Operation | Tool | Depth | Machining |
|---|---|---|---|
| Text pocket | T2, 3 mm | −2 mm | **zigzag** pocket of the text |
| Frame engraving | T1, 1 mm | −0.5 mm | follow the line, *Centered*, **shifted start** |
| Holes | T2, 3 mm | −4 mm | **spiral** pocket in each hole |
| Cut out | T3, 6 mm | −6 mm | follow the line *Outside*, **4 tabs**, **arc** entry |

**Text pocket.** The text "WORKSHOP" (Arial bold, 55 mm) is cleared in zigzag; with letters of that size, the 3 mm cutter is fine.

![Text pocket of the sign](img/ex-sign-textpocket.en.png)

**Engraved frame, shifted start.** The frame is a rectangle with rounded corners made of connected lines and arcs: *follow the line* makes a single path of it. The start position was moved along the frame, away from the corners, so the entry mark of the tool is in the middle of a side.

![Engraving of the frame](img/ex-sign-frame.en.png)

**Holes as spiral pockets.** The four mounting holes are small circles cleared by a **spiral** pocket (*Mode: Spiral*, inward or outward): the 3 mm cutter makes a spiral in an 8 mm hole.

![Holes in spiral](img/ex-sign-holes.en.png)

**Cut-out with tabs and arc entry.**

![Cut-out of the sign](img/ex-sign-cut.en.png)

Four tabs 15 mm long hold the plate. The entry of the tool is set in the **operation**:

![Operation Cut out: arc start](img/ex-sign-start.en.png)

- **Start**: *Normal* (the tool goes down on the first point of the contour), *Line* or *Arc* (the tool reaches the contour along a line or an arc, which avoids marking the edge of the part).
- **Offset**: the size of this entry (8 mm here).
- **Exit**: the same at the end of the contour.
- **Oblique plunge**: the tool goes down at an angle along the entry instead of plunging.

![The settings of the operation Cut out](img/ex-sign-group.en.png)

The *Start* of the operation is how the tool reaches the contour; the *Start position* of a machining is where on the contour it begins.

### Example C: a tray, offset pocket with islands

![The tray: a pocket with four islands](img/ex-tray-view.en.png)

The project `examples/tray.en.opp` is a 340 × 220 mm tray. The layer **Pocket** contains the outline of the pocket (a rectangle with rounded corners) and, **inside it**, four shapes which must stay in place: two circles, a rounded rectangle and a triangle. These shapes are **islands**.

1. Create the operation *Pocket with islands* (tool T2, 3 mm end mill, depth −4 mm, pass depth 2 mm).
2. Select **all** the shapes of the layer **Pocket**: the outline and the islands (**Edit / Select / The layer / Pocket**). A closed shape **inside** another one is recognised as an island, a shape inside an island becomes material to clear again, and so on.
3. Click the pocket icon and choose the mode **Offset**.

![The offset pocket around the islands](img/ex-tray-pocket.en.png)

The cutter clears the pocket by **concentric contours**: it follows the outline inward, goes around each island at the distance of the tool radius, and the paths join between the islands. The islands stay untouched, including the corners of the triangle and of the rounded rectangle, which you can check afterwards in the simulator. 

Useful settings: *Finishing* adds a last pass along the walls and the islands; the **radius of the tool** must be smaller than the narrowest width to clear (between two islands, or between an island and the wall), otherwise that space is not machined.

The operation *Cut out* (tool T3, 6 mm end mill, depth −8 mm) follows the outer contour, *Outside*, with four tabs 15 mm long.

![Cut-out of the tray](img/ex-tray-cut.en.png)

## 12. Automatic nesting

The **nesting** places copies of parts on a sheet to lose as little of it as possible. It starts from **Machining / Nesting...** and creates new layers: the original drawing is not modified.

The example `docs/examples/nesting.en.opp` holds six types of parts (a group each) to try the nesting.

### Preparing the parts

1. Draw or import each part, with its holes.
2. **Group** the shapes of each part (see "Groups", chapter 4). A part is a group.
3. Select the groups to nest (a rectangle around the drawing, or **Edit / Select / Select all**) then choose **Machining / Nesting...**. Without a group in the selection, a message says so.

For each group, the **largest closed contour** is the outline of the part, and the closed contours inside it are its holes, its islands in the holes, and so on: a part can be placed in the hole of another one. The open shapes of a group are copied with it but do not count for the placement.

### The window

![The nesting window](img/nesting-window.en.png)

Leave the mouse on the **(?)** next to a setting to read its explanation.

| Setting | Role |
|---|---|
| Width, Height | Size of the sheet, in the unit of the drawing. Its lower left corner is at (0, 0). |
| Margin to the edge | Smallest distance between the parts and the edge of the sheet. |
| Distance between parts | Smallest distance between two parts: at least the cutting width of the tool. |
| Algorithm | *Fast (greedy)*, *Search* or *Overlap minimization* (see below). |
| Search time (s) | For the last two algorithms: the time given to the calculation. It can be stopped before. |
| Rotations | The angles tried for each part: none, 0 and 180 degrees, every 90, 15, 5 or 1 degree. The more there are, the better the result can be, but the longer the calculation: 90 degrees is enough for rectangular parts; 15, 5 or 1 degree for slanted parts. 1 degree is very slow. |
| Use the holes of the parts | Allows a part to be placed in the holes of the other parts. |
| Search step | Distance between two positions tried when the program goes over the sheet to place a part. The smaller it is, the more small places it finds (in a hole, between two parts), but the slower it is. The position found is then refined with smaller and smaller moves, down to 0.1. |
| Parts | One line per group: its name, the size of its outline and the **quantity** to place. |

The settings are not saved in the document: the `.opp` file only keeps the drawing.

### The algorithms

- **Fast (greedy)**: places the parts one by one, from the biggest to the smallest, each at its best place, the rotations being tried in parallel. A few seconds. The result is always the same for the same data.
- **Search**: starts from the fast result, then tries many other orders of the parts for the time chosen, on all the cores of the processor, and keeps the layout which uses the least length of the sheet.
- **Overlap minimization**: starts from the fast result, makes the sheet shorter, which makes parts overlap, then moves and turns the parts which overlap to the places where they overlap the least, until nothing overlaps. A layout is only kept if an exact test of the contours confirms it and it is shorter. It is the most efficient, also on all the cores, and it tries 36 angles at most.

To give an idea, on a set of 85 mixed parts, the search made the sheet about 2.6 % shorter than the fast result, and the overlap minimization about 5 % in one to two minutes. The result depends on the parts: nested circles are placed less well than rectangles. The two search algorithms use chance: two calculations do not give exactly the same result.

### During the calculation

![The waiting window](img/nesting-wait.en.png)

The calculation runs in the background: a window shows a turning circle, the progress, the **length of the sheet used** by the best layout found and the number of tries. The drawing cannot be modified during this time. Two buttons:

- **Stop and keep the result**: the calculation stops and the best layout found is put in the drawing (for the fast algorithm, the parts which are not placed yet stay unplaced).
- **Cancel**: the calculation stops and nothing is added.

### The result

![A nesting result](img/nesting-result.en.png)

- A **Sheet_WxH** layer (for example `Sheet_1000x500`) is created once: it holds the rectangle of the sheet, from (0, 0) to (width, height). It is visible and **locked**. A sheet of the same size reuses this layer.
- The copies of the parts are in a new layer **Nesting_1**, then *Nesting_2*... Each copy is a group, named like the part followed by a number.
- All the other layers are **hidden** to see the result alone: check their *Visible* box to see them again. The original shapes are not modified.
- One Ctrl+Z removes the copies and the new layers; it does not restore the visibility of the hidden layers.
- The log gives the number of parts placed and not placed, the use rate of the sheet, the length used and the time of the calculation. A shape which cannot be copied is reported.

### After the nesting

To machine the result: create the machinings on the *Nesting_n* layer (*follow the line* on the selection of the groups), then set them with **Machining / Automatic radius compensation** and **Machining / Organize the toolpaths** (chapter 7). The post-processor machines the holes of each part before its outer contour (chapter 8).

## 13. Going further

- [CAD-SCRIPTS.en.md](CAD-SCRIPTS.en.md): writing a CAD script in Python.
- [POSTPROCESSOR.en.md](POSTPROCESSOR.en.md): writing or adapting a post-processor.
- [SIMULATOR.en.md](SIMULATOR.en.md): the simulator and writing a translator.
- [TOOLS-MACHINES.en.md](TOOLS-MACHINES.en.md): tools, torches, machines and the cutting table.

Always read the output file and run the simulation before you start the machine. The generic post-processors are starting points, not guaranteed profiles for every controller, and the machining algorithms can fail on difficult shapes.
