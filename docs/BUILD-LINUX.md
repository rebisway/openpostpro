# Building OpenPostPro on Linux

Tested: Zorin OS and Linux Mint 22.3 (Cinnamon, X11, Ubuntu 24.04 base) in a virtual machine: drawing, machining operations, output, simulation and the CAD scripts work. Any distribution with GCC >= 11 (C++20), CMake >= 3.16 and Python >= 3.12 should work.

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
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j$(nproc)
```

`-DCMAKE_BUILD_TYPE=Debug` adds the Debug menu. The program ends in `openpostpro/bin/` (the build files stay in `build-linux/`). The post-build step copies `lang`, `fonts`, `shaders`, `textures`, `postpro`, `script` and `simulator` beside the executable in `bin/`, which finds them from its own location.

## Run

```bash
./bin/openpostpro
```

User data (settings, cloned scripts and post-processors, log) are in `~/.openpostpro/`; the log is `~/.openpostpro/openpostpro.log`.

## Debian package

```bash
cd build-linux
cpack -G DEB
sudo apt install ./openpostpro_*.deb
```

The package installs the program and its resources in `/usr/lib/openpostpro` (a link in `/usr/bin`), the menu entry (Graphics) and the icons. The dependencies are computed from the libraries of the build (`dpkg-shlibdeps`), plus `python3 (>= 3.12)`; `python3-tk` and `zenity` or `kdialog` are recommended. Build it on the oldest system you want to support: the package needs the libraries of the system where it is built, so a package made on Ubuntu 24.04 or Linux Mint 22 installs on those and on the newer ones.

Debian 12 has Python 3.11 only: the package does not install there. Python 3.12 or later can be built by hand (`./configure --enable-shared --prefix=/usr/local && make && sudo make install && sudo ldconfig`), then OpenPostPro built from the sources on that system.

Not done yet: the association of `.opp` and `.dxf` files with the program, because `openpostpro` does not read a file given on the command line.

## Fonts

The text tool lists the fonts found in `/usr/share/fonts`, `/usr/local/share/fonts`, `~/.fonts` and `~/.local/share/fonts`, plus the `fonts` folder beside the executable. A font which is missing (for example Arial, in a document made on Windows) is replaced by the first of: Arial, Helvetica, Liberation Sans, Arimo, DejaVu Sans, Noto Sans, Ubuntu, FreeSans, Roboto, then any other. The name saved in the document does not change. To have Arial itself: `sudo apt install ttf-mscorefonts-installer`.

## Documents

The documents made on Windows (CR LF line ends) and on Linux (LF) open on both systems.

## OpenGL

OpenGL 3.3 core is required. Check with `glxinfo | grep "OpenGL core"` (package `mesa-utils`). Without a 3.3 driver (virtual machine, old GPU), the software renderer of Mesa can be forced with `LIBGL_ALWAYS_SOFTWARE=1 ./bin/openpostpro`. An X11 session is the safest choice for GLFW 3.3.

## Known limits

- The file dialogs block the window while they are open (they are external processes).
- The clipboard goes through GLFW (works on X11).
- The tests of `tests/cpp` still use the `.bat` files (MSVC); `python3 tests/run_tests.py` works on Linux.
- The GCC warnings are off by default (the code gives many, written with MSVC): `-DOPP_WARNINGS=ON` shows them.
- The build uses `-fpermissive` by default (code written with MSVC); disable with `-DOPP_PERMISSIVE=OFF` to see what GCC really thinks.
