# Repository Guidelines

## Project Structure & Module Organization

Core project files live at the repository root: `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, and the README files. Application sources are currently under `src/`; the entry point is `src/main.cpp`. Local machine paths belong in `CMakeUserPresets.json`, which is intentionally ignored by Git. Use `CMakeUserPresets.example.json` as the template to copy and edit. Vendored third-party code lives in `third_party/` and should be treated as external unless a change there is explicitly required.

## Build, Test, and Development Commands

Prepare local presets first:

```powershell
Copy-Item CMakeUserPresets.example.json CMakeUserPresets.json
```

Configure and build on Windows with VS2022:

```powershell
cmake --preset windows-vs2022
cmake --build --preset build-windows-vs2022 --config Debug
```

Use the default Visual Studio detected on the machine:

```powershell
cmake --preset windows-default
cmake --build --preset build-windows-default --config Debug
```

On Linux or macOS, use `linux-default` or `macos-default` with the matching `build-*` preset.

## Coding Style & Naming Conventions

Use C++20 and follow the Google C++ Style Guide for naming, formatting, and header/source organization. Prefer 4-space indentation. Store source files, headers, CMake files, and Qt project files in UTF-8 without BOM; use non-ASCII text only when it is required for user-visible strings, translations, or test data. Use `PascalCase` for types and most functions, `snake_case` for variables and function parameters, trailing underscores for class data members, and `kPascalCase` for constants. Name C++ source files, headers, test files, and Qt-related files in `snake_case`, for example `chat_window.h`, `chat_window.cpp`, `chat_window_test.cpp`, `chat_window.ui`, and `app_resources.qrc`; keep matching header/source basenames aligned. Keep CMake declarative and minimal; add new targets and dependencies in `CMakeLists.txt` rather than hardcoding paths in source files.

## Testing Guidelines

There is no first-party test target yet. For now, validate changes by configuring and building with the relevant preset before opening a PR. When adding tests, place them under a top-level `tests/` directory and register them with CTest. Name test files after the unit under test, for example `network_client_test.cpp`.

## Commit & Pull Request Guidelines

Recent commits use short, imperative subjects, for example: `Bootstrap CMake, vcpkg, and Qt example project`. Keep commit messages concise and specific. PRs should summarize user-visible changes, list the preset and platform used for verification, and note any required local setup changes such as new `VCPKG_ROOT` or `QT_ROOT` expectations.

## Configuration Tips

Do not commit `CMakeUserPresets.json`, build outputs, or machine-specific Qt/vcpkg paths. Keep shared configuration in `CMakePresets.json` and local overrides in `CMakeUserPresets.json`.

Documentation changes must stay in sync across `README.md` and `README.zh-CN.md`. If one is updated, update the other in the same change unless the difference is intentional and documented.

When mentioning repository files in `AGENTS.md`, `README.md`, or `README.zh-CN.md`, use relative paths such as `src/main.cpp` or `CMakeUserPresets.example.json`. Do not use absolute filesystem paths.

# AGENTS.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.
