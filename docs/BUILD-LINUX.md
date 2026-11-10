# Building OpenPostPro on Linux

Tested target: Debian/Ubuntu 24.04 (GCC 13, Python 3.12). Any distribution with GCC >= 11 (C++20), CMake >= 3.16 and Python >= 3.12 should work.

## Dependencies

```bash
sudo apt install build-essential cmake pkg-config git \
    libglfw3-dev libfreetype-dev libgl1-mesa-dev libx11-dev \
    python3-dev python3-tk zenity
```

- `python3-dev`: headers and `libpython3.x.so` (the code uses the stable ABI, any Python >= 3.12). `python3-tk` is used by some CAD scripts.
- `zenity` (or `kdialog`): the file dialogs (open, save). Without one of them, the menus File > Open/Save do nothing.
- Dear ImGui, GLM, gl3w, stb are in `openpostpro/src/third`; GLFW and FreeType come from the system.

## Build

```bash
cd openpostpro
cmake -S . -B ../build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build ../build-linux -j$(nproc)
```

`-DCMAKE_BUILD_TYPE=Debug` adds the Debug menu. The post-build step copies `lang`, `fonts`, `shaders`, `textures`, `postpro`, `script` and `simulator` beside the executable, which finds them from its own location.

## Run

```bash
../build-linux/openpostpro
```

User data (settings, cloned scripts and post-processors, log) are in `~/.openpostpro/`; the log is `~/.openpostpro/openpostpro.log`.

## OpenGL

OpenGL 3.3 core is required. Check with `glxinfo | grep "OpenGL core"` (package `mesa-utils`). Without a 3.3 driver (virtual machine, old GPU), the software renderer of Mesa can be forced with `LIBGL_ALWAYS_SOFTWARE=1 ../build-linux/openpostpro`. An X11 session is the safest choice for GLFW 3.3.

## Known limits

- The file dialogs block the window while they are open (they are external processes).
- The clipboard goes through GLFW (works on X11).
- The tests of `tests/cpp` still use the `.bat` files (MSVC); `python3 tests/run_tests.py` works on Linux.
- The build uses `-fpermissive` by default (code written with MSVC); disable with `-DOPP_PERMISSIVE=OFF` to see what GCC really thinks.
