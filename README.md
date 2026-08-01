# C++ Native App Project Template

A small C++23 starter for desktop applications on Windows and Linux. It provides a production-shaped application shell without turning the template into a game engine or framework.

The structure is inspired by [TheCherno/ProjectTemplate](https://github.com/TheCherno/ProjectTemplate), with pinned dependencies, local tool bootstrap, CI, tests, and Herta's GLFW custom title-bar fork.

## Included

- C++23
- Premake 5, downloaded to `External/Premake` at a pinned version by Setup
- [Herta GLFW fork](https://github.com/Solessfir/glfw) with custom title bars on Win32, X11, and Wayland
- Dear ImGui docking branch
- OpenGL 3.3 presentation backend
- Configuration-aware loose and embedded asset provider
- C++23 asset baker for single-executable Shipping builds
- Roboto Regular and Medium loaded from loose or embedded assets
- doctest unit tests
- DPI-aware custom title bar with native move, resize, system menu, minimize, maximize, Windows 11 Snap Layout, and close behavior
- Windows and Linux GitHub Actions builds
- CodeQL, dependency review, and Dependabot configuration

Source dependencies are pinned Git submodules. Normal project generation and builds do not access the network.

## Quick start

Clone with Git, then run Setup once.

### Windows

Setup prefers Visual Studio 2026 with the `v145` toolset. If it is unavailable, Setup and project generation automatically fall back to Visual Studio 2022 with `v143`:

```bat
Setup.bat
GenerateProjectFiles.bat
```

The selected `ProjectTemplate.slnx` or `ProjectTemplate.sln` is generated at the repository root. Supporting Visual Studio project files remain under `Intermediate\ProjectFiles`.

Pass a version explicitly when fallback is not wanted:

```bat
Setup.bat -VisualStudioVersion 2026
GenerateProjectFiles.bat vs2026

Setup.bat -VisualStudioVersion 2022
GenerateProjectFiles.bat vs2022
```

### Linux

Setup checks for a C++23 compiler and standard library with `<expected>` and `<print>`, Make, OpenGL, X11, Wayland, xkbcommon, `wayland-scanner`, and the normal download tools. GCC 14 or newer is required when using libstdc++. If a system package is missing, Setup prints an actionable command for Arch, Debian/Ubuntu, or Fedora.

```bash
bash ./Setup.sh
bash ./GenerateProjectFiles.sh
make --directory=Intermediate/ProjectFiles/gmake --jobs=2 config=development
```

On Ubuntu 24.04, select GCC 14 explicitly:

```bash
export CC=gcc-14 CXX=g++-14
bash ./Setup.sh
bash ./GenerateProjectFiles.sh
make --directory=Intermediate/ProjectFiles/gmake --jobs=2 config=development
```

Run the application and tests:

```bash
./Binaries/linux/x86_64/Development/StarterApp
./Binaries/linux/x86_64/Development/StarterTests
```

For backend-specific diagnostics, pass `--platform=x11` or `--platform=wayland`. Normal applications should leave platform selection to GLFW.

## Project structure

```text
ProjectTemplate/
|-- .github/                 # CI, CodeQL, dependency updates
|-- Build/Premake/           # Workspace project and toolchain policy
|-- Assets/                  # Loose source assets and embedding manifest
|-- Config/                  # Pinned downloaded-tool lock
|-- External/                # Pinned third-party sources and tools
|   `-- Premake/             # Ignored host binaries installed by Setup
|-- Scripts/                 # Setup, dependency validation, cleanup
|-- Source/
|   |-- Application/         # Window, OpenGL, ImGui, and app lifetime
|   |-- UI/                  # Pure title-bar layout and hit testing
|   `-- Main.cpp
|-- Tests/
|-- Tools/AssetBaker/        # Shipping asset code generator
|-- Cleanup.bat/.sh
|-- GenerateProjectFiles.bat/.sh
|-- Setup.bat/.sh
`-- premake5.lua
```

The runtime remains one executable target. `AssetBaker` is a dependency-free host tool used only when producing embedded Shipping assets. Split reusable runtime code into a static library only after the derived application has a real second consumer.

## Assets

Debug and Development load loose files from `Assets`. Shipping reads `Assets/EmbeddedAssets.txt`, generates one source file under `Intermediate`, and compiles the selected binary data into `StarterApp`.

List one forward-slash path per manifest line, relative to `Assets`. Blank lines and lines beginning with `#` are ignored. Regenerate project files after adding or removing manifest entries so the native build system tracks the complete input set.

Generated asset source is never committed. Application code accesses loose and embedded data through the same provider, and normal builds never download assets or tools.

Roboto is stored under `Assets/Fonts/Roboto` with its SIL Open Font License. Dear ImGui keeps the source TTF data available for dynamic glyph generation and scales fonts through its DPI-aware font system.

## Custom title bar

The GLFW window remains decorated, but its native title bar is disabled with `GLFW_TITLEBAR`. A fast, allocation-free hit-test callback returns native caption, resize, system-menu, and window-control roles while ImGui draws the pixels.

System controls are drawn with ImGui primitives because GLFW and the window system own their clicks. This preserves native behavior, including Windows 11 Snap Layout. Do not replace them with `ImGui::Button` without also returning `GLFW_HIT_TEST_CLIENT` for those rectangles.

Dear ImGui multi-viewport support is intentionally disabled. Each detached viewport would need its own custom-title-bar state, drawing, and hit-test callback. Docking inside the main window is enabled.

## Deliberately not included

### Math

The starter has no independent math domain. Standard C++ and ImGui's UI-local vector types are enough for the shell. Add GLM when a derived app gains real 2D or 3D rendering math, and keep GLM types out of application-facing interfaces.

### General SVG icons

The title-bar controls are resolution-independent vector primitives and need no files. Add [LunaSVG](https://github.com/sammycage/lunasvg) when the first application needs file-based SVG icon brushes. Keeping it opt-in avoids adding an SVG parser and rasterizer to every native app.

Store future source icons under `Resources/Icons` as SVG. Rasterize and cache them by DPI and theme instead of embedding one bitmap size into the executable.

### Custom OpenGL rendering

The starter uses only OpenGL 1.1 entry points directly; Dear ImGui owns its private OpenGL 3 loader. Add a pinned generated glad2 loader before writing custom modern OpenGL rendering. Do not call or include ImGui's private loader from application code.

Also omitted until needed: spdlog, serialization, task systems, native file dialogs, audio, networking, and scripting.

## Starting a real project

Use GitHub's `Use this template`, then replace these identifiers:

- `ProjectTemplate` for the repository, workspace, and C++ namespace
- `StarterApp` for the executable target
- `Project Template` for the displayed application title

Keep submodule revisions pinned. Update them in reviewed commits rather than tracking moving branches during Setup.

## Cleaning

Cleanup removes only explicit generated paths. It does not depend on Git and does not guess which source files are untracked.
It also removes the local Premake installation and generated Wayland protocol files, so run Setup again before regenerating projects.

```bat
Cleanup.bat
```

```bash
bash ./Cleanup.sh
```

## License

This template is available under the [MIT License](LICENSE). Third-party components retain their upstream licenses and attribution.
