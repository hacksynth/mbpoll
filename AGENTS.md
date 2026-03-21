# AGENTS.md

## Repository Overview

- `mbpoll` is a C/CMake command-line Modbus master for RTU and TCP.
- Main implementation lives in `src/`.
- `src/mbpoll.c` is the CLI entry point and main protocol flow.
- Shared helpers live in `src/serial.c`, `src/utils.c`, `src/custom-rts.c`, and `src/chipio.c`.
- Unit tests live in `tests/` and currently cover `serial` and `utils`.
- CI definitions live in `.github/workflows/`.
- User-facing docs live in `README.md`, `README-WINDOWS.md`, and `doc/mbpoll.1`.

## Build and Test

- Current development environment is Arch Linux.
- Linux prerequisites: `cmake`, `pkg-config`, `libmodbus-dev`.
- macOS prerequisites: `cmake`, `pkg-config`, `libmodbus`.
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --config Release`
- Run tests: `ctest --test-dir build --build-config Release --output-on-failure`
- Smoke-check the CLI after behavior changes: `./build/bin/mbpoll -V` and `./build/bin/mbpoll -h || true`
- For debugging, prefer `-DCMAKE_BUILD_TYPE=Debug`.
- There is no dedicated lint target; keep compiler warnings clean under the flags defined in `CMakeLists.txt`.

## Engineering Conventions

- Follow the existing C99 style and 2-space indentation.
- Keep changes focused and consistent with the current structure; avoid broad refactors unless the task requires them.
- Preserve cross-platform behavior across Linux, macOS, and Windows.
- On Unix, `libmodbus` is resolved via `pkg-config`. On Windows, CI builds against cloned `libmodbus` sources.
- If a required tool is missing on the local Arch Linux environment, search for and install it with `yay`.
- When multiple tool/runtime versions may be needed, prefer version managers such as `uv` and `nvm` instead of ad hoc system-wide switching.
- Update `README.md` and `doc/mbpoll.1` when CLI options, help text, installation steps, or user-visible behavior change.

## Constraints

- Version 1 is effectively in maintenance mode. Treat bug fixes, compatibility work, and documentation as safer than large new feature additions.
- Pull requests should target `dev`. `master` is reserved for stable releases.
- After each completed round of code changes, create a git commit with a focused message.
- Do not commit `build/` output or other generated artifacts.
- Avoid adding new dependencies or changing packaging/install behavior unless the task explicitly requires it.

## Done Means

- The relevant target builds successfully.
- `ctest` passes for the affected configuration.
- If CLI behavior changed, version/help smoke checks still work.
- If user-visible behavior changed, the corresponding docs were updated.
- Review the diff for RTU/TCP regressions, register addressing changes, timeout or poll-rate defaults, and platform-specific side effects.
