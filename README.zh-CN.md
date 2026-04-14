# qtoxchat

[English](README.md) | 中文

这是一个使用 CMake 管理、使用 vcpkg 管理依赖，并为 Qt 开发做好准备的最小 C++20 项目。

## 环境要求

- CMake 3.30 或更高版本
- 支持 C++20 的编译器
- vcpkg
- Qt 6

## 本地 Preset 配置

仓库中会跟踪 `CMakeUserPresets.example.json` 作为示例文件。

请基于它创建你本机使用的 `CMakeUserPresets.json`，然后把其中的占位路径替换成真实路径。

Windows：

```powershell
Copy-Item CMakeUserPresets.example.json CMakeUserPresets.json
```

Linux 和 macOS：

```bash
cp CMakeUserPresets.example.json CMakeUserPresets.json
```

至少需要修改以下变量：

- `VCPKG_ROOT`
- `QT_ROOT`

## 可用 Preset

常用配置 preset：

- `windows-vs2022`
- `windows-default`
- `linux-clang`
- `linux-default`
- `macos-default`

常用构建 preset：

- `build-windows-vs2022`
- `build-windows-default`
- `build-linux-clang`
- `build-linux-default`
- `build-macos-default`

其中 `windows-default` 会让 CMake 在当前机器上自动探测默认的 Visual Studio 生成器。

## 构建方法

### Windows + Visual Studio 2022

生成构建文件：

```powershell
cmake --preset windows-vs2022
```

构建 Debug：

```powershell
cmake --build --preset build-windows-vs2022 --config Debug
```

构建 Release：

```powershell
cmake --build --preset build-windows-vs2022 --config Release
```

### Windows + 自动探测 Visual Studio

生成构建文件：

```powershell
cmake --preset windows-default
```

构建 Debug：

```powershell
cmake --build --preset build-windows-default --config Debug
```

### Linux

生成构建文件：

```bash
cmake --preset linux-default
```

构建：

```bash
cmake --build --preset build-linux-default
```

### macOS

生成构建文件：

```bash
cmake --preset macos-default
```

构建：

```bash
cmake --build --preset build-macos-default
```

## 说明

- `vcpkg.json` 启用了 manifest 模式，目前包含 `curl` 依赖。
- `CMakeUserPresets.json` 会被 Git 忽略，因为其中包含机器相关的本地路径。
- 如果 Qt 的运行时动态库没有加入 `PATH`，那么在 IDE 或已准备好的终端之外直接启动程序时，可能会因为缺少 Qt 运行库而失败。
