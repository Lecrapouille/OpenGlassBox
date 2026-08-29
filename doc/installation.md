# Compiling OpenGlassBox

## Prerequisites: Installing system packages

- **Operating systems**: Linux, macOS. Should compile on Windows as well.
- **Build tools**: C++17 compiler (`g++` or `clang++`), GNU Make, Git. C++17 is required by the public headers, which use `std::optional`.
- **Debug library** (debug builds only): [backward-cpp](https://github.com/bombela/backward-cpp): automatically downloaded and built by the Makefile (not installed system-wide).
- **Building tests** (optional): [Google Test](https://github.com/google/googletest) (must be downloaded, built, and installed manually), plus coverage tools (see below).
- **Makefile helper** [MyMakefile](https://github.com/Lecrapouille/MyMakefile): automatically fetched when cloning with `--recursive`.

GLFW and Dear ImGui were chosen for the demo because they were the quickest way to see the simulation run; feel free to plug in your own renderer.

- **Renderer libraries**: GLFW 3, GLEW, and OpenGL 3.3 (system packages; demo only).
- **GUI libraries**: [Dear ImGui](https://github.com/ocornut/imgui) (docking), [ImPlot](https://github.com/epezent/implot), and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog): downloaded and built by the Makefile (demo only).

### Debian / Ubuntu

```sh
# Required to build the demo
sudo apt-get install build-essential git pkg-config libglfw3-dev libglew-dev

# Optional: debug builds (backward-cpp) and code coverage
sudo apt-get install libdw-dev lcov
```

### Fedora

```sh
# Required to build the demo
sudo dnf install gcc-c++ make git pkgconf-pkg-config glfw-devel glew-devel

# Optional: debug builds (backward-cpp) and code coverage
sudo dnf install elfutils-devel lcov cmake
```

`cmake` is only needed if you build and install Google Test from source (see the CI workflow for an example).

## Download and compile OpenGlassBox

Clone the repository recursively:

```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

Build the project (libraries + demo):

```sh
cd OpenGlassBox/
make download-external-libs
make -j8
```

This creates a `build` folder with executables and libraries. On macOS, a bundle application is also created inside the build folder.

Adjust `-j8` to the number of cores on your machine, or pick a compiler with `make CXX=clang++ -j8`. Builds are optimized with debug symbols by default (`COMPILATION_MODE := normal` in `Makefile.common`). Use `make COMPILATION_MODE=debug -j8` to step through the code, and `make COMPILATION_MODE=release -j8` to ship. The mode matters: a layer rule runs over every cell of a city, so the Chicago save with its three hundred thousand cells is an order of magnitude slower to simulate when compiled without optimizations.

(Optional) Install OpenGlassBox on your system:

```sh
sudo make install
```

(Optional) Run unit tests with code coverage:

```sh
cd OpenGlassBox/tests
make coverage -j8
```

Run the demo:

## Run the demo

```sh
./build/OpenGlassBox-demo
./build/OpenGlassBox-demo demo/data/Simulations/chicago.ogc
```

## Simulation Engine Libraries

After `make`, the build folder contains `libOpenGlassBox.a` and, on most platforms, `libOpenGlassBox.so`. Headers live in `include/OpenGlassBox/`.

After `make install`, pkg-config can find the shared library:

```sh
pkg-config --cflags --libs OpenGlassBox
```

Or find the static library:

```sh
pkg-config --cflags --libs --static OpenGlassBox
```

You can use these libraries inside your game using the pkg-config. See [Integration guide](integration.md).
