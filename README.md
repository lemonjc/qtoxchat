# qtoxchat

English | [中文](README.zh-CN.md)

Minimal C++20 project managed by CMake, using vcpkg for dependencies and prepared for Qt development.

## Requirements

- CMake 3.30 or newer
- A C++20 compiler
- vcpkg
- Qt 6

## Local Preset Setup

This repository tracks `CMakeUserPresets.example.json` as an example.

Create your local `CMakeUserPresets.json` from it and replace the placeholder paths with real local paths:

Windows:

```powershell
Copy-Item CMakeUserPresets.example.json CMakeUserPresets.json
```

Linux and macOS:

```bash
cp CMakeUserPresets.example.json CMakeUserPresets.json
```

Update at least:

- `VCPKG_ROOT`
- `QT_ROOT`

## Available Presets

Common configure presets:

- `windows-vs2022`
- `windows-default`
- `linux-clang`
- `linux-default`
- `macos-default`

Common build presets:

- `build-windows-vs2022`
- `build-windows-default`
- `build-linux-clang`
- `build-linux-default`
- `build-macos-default`

`windows-default` lets CMake auto-detect the default Visual Studio generator on the current machine.

## Build

### Windows with Visual Studio 2022

Configure:

```powershell
cmake --preset windows-vs2022
```

Build Debug:

```powershell
cmake --build --preset build-windows-vs2022 --config Debug
```

Build Release:

```powershell
cmake --build --preset build-windows-vs2022 --config Release
```

### Windows with Auto-Detected Visual Studio

Configure:

```powershell
cmake --preset windows-default
```

Build Debug:

```powershell
cmake --build --preset build-windows-default --config Debug
```

### Linux

Configure:

```bash
cmake --preset linux-default
```

Build:

```bash
cmake --build --preset build-linux-default
```

### macOS

Configure:

```bash
cmake --preset macos-default
```

Build:

```bash
cmake --build --preset build-macos-default
```

## Notes

- `vcpkg.json` enables manifest mode and currently installs `curl`.
- `CMakeUserPresets.json` is intentionally ignored by Git because it contains machine-specific paths.
- If Qt runtime DLLs are not on `PATH`, launching the executable directly may fail outside an IDE or a prepared shell.
