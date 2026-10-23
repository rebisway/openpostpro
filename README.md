# OpenPostPro

**An open-source CAD/CAM tool for drawing, preparing toolpaths, and generating machine code.**

OpenPostPro brings together a lightweight 2D drawing environment, CAM operations, and configurable Python post-processors. Import a DXF drawing or create geometry directly in the application, prepare machining or cutting operations, then generate output adapted to your machine through a post-processor.

The project is in an **early development stage**. Some geometry and toolpath algorithms still need improvement, so generated output should always be checked carefully before being used on a real machine.

![OpenPostPro](https://github.com/user-attachments/assets/fc224d77-bbef-4d37-98aa-c54a5e4b89e1)

## ✏️ Draw and prepare geometry

OpenPostPro includes tools to create and edit 2D geometry directly:

- Points, lines, circles, arcs, ellipses, polylines, splines, and text
- Geometric transformations such as offset, symmetry, scaling, and rotation
- Chamfer operations
- Boolean operations to combine, subtract, or intersect supported shapes
- DXF import for supported drawing entities, including lines, points, circles, arcs, ellipses, polylines, splines, and text

## 🐍 Python scripting for CAD
OpenPostPro also supports Python scripts at the CAD level. Scripts can create geometry directly inside the application, making it possible to automate repetitive work or generate complex shapes parametrically instead of drawing every element by hand.

For example, the project includes a demo script that generates oblong holes (slots). Scripts can be adapted or written to create the specific elements your design requires.

This CAD scripting capability is separate from Python post-processors: CAD scripts generate drawing geometry, while post-processors translate CAM operations into machine-specific output.

## ⚙️ CAM operations

Create toolpath operations from your drawing, including:

- **Follow** — follow selected geometry
- **Offset** — create interior or exterior offsets
- **Pocket** — generate pocketing toolpaths
- **Drilling** — configure drilling cycles, including peck drilling, tapping, and boring options
- **Spiral** — create spiral toolpaths
- **Move to** — add a positioning move

CAM operations have configurable parameters. Because this is an early release, the results, especially outline and offset calculations, may not be reliable for every shape or machining situation.

## 🧩 Python post-processors

Post-processors are written in Python, making machine-output customization accessible without changing the C++ application.

The repository includes generic post-processors for:

- **Milling / router**
- **Plasma cutting**

Post-processor settings include output file extension, decimal precision, line numbering, comments, header and footer, coordinate units, and relative or absolute arc-centre coordinates. The plasma post-processor also exposes settings for torch commands, start delay, reference setup, and Z-axis handling.

These are generic starting points, not guaranteed profiles for every controller. Review and adapt the output to your machine and controller before use.

## 🌍 Interface and technology

The interface language is managed through key/value files in `openpostpro/lang`. English, French, and Spanish language resources are included.

OpenPostPro is written in **C++20** and uses:

- **OpenGL 3** for rendering
- **Dear ImGui** for the user interface
- **GLFW** for window and input handling
- **GLM** for mathematics
- **FreeType** for font rendering
- **Python 3.13** embedded for post-processors and scripts

## 🖥️ Running on Windows

The current project files and bundled runtime libraries target Windows.

You need:
- The **Python 3.13 runtime**
- The **Microsoft Visual C++ x64 Redistributable**

Download or clone the repository, then run the application using a compatible build. The project includes a Visual Studio solution at `openpostpro/openpostpro.sln`.

When compiling, the `copy_libs_output.bat` script copies the required runtime resources into the output directory, including fonts, language files, post-processors, scripts, textures, and shaders.

## 🛠️ Building from source

1. Install Visual Studio 2022 with C++ desktop development tools.
2. Open `openpostpro/openpostpro.sln`.
3. Build the solution for the appropriate x64 configuration.
4. Ensure the required resource folders and runtime libraries are present beside the executable. The included `copy_libs_output.bat` handles copying the project resources as configured by the solution.

The source contains relatively few Windows-specific parts, and Linux or macOS builds may be possible, but **neither has been tested yet**.

## 🚧 Project status

OpenPostPro is an early release built as an independent open-source project. The main priority for future improvement is the robustness and precision of geometry and outline calculations. Some algorithms currently use floating-point precision and need further work.

Please treat generated toolpaths and machine code as output that requires verification. Simulate or inspect the program and check machine settings, tool definitions, clearances, feeds, and speeds before running it on physical equipment.

Bug reports, feedback, and contributions are welcome.

## 📜 License

OpenPostPro is released under the **GNU General Public License v3.0 (GPL-3.0)**. See [`LICENSE`](LICENSE) for the full license text.

## 🔗 Links

- **Repository:** https://github.com/rebisway/openpostpro
- **Contact:** contact@rebisway.com

I had great fun building OpenPostPro. I hope you enjoy exploring it, using it, and helping it improve.
