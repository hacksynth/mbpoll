# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

mbpoll is a command-line ModBus master simulator written in C. It communicates with ModBus slaves via RTU (serial) or TCP protocols. The tool can read/write discrete inputs/outputs (coils) and input/holding registers in various formats (16-bit, 32-bit int, float, hex, string).

## Build Commands

```bash
# Linux/macOS build (requires libmodbus >= 3.1.6)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Debug build (with additional checks, no NDEBUG)
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# Install
sudo make install

# Create Debian package
make package

# Run Tests
ctest --output-on-failure

# Uninstall
sudo make uninstall
```

For Windows builds, see README-WINDOWS.md (requires Visual Studio 2022, CMake, and cloning libmodbus into the project directory).

## Dependencies

- **libmodbus >= 3.1.7** - Core ModBus library (required; 3.1.7+ needed for -Q/-X quirks options)
- **libpiduino** - Optional, for GPIO-based RS-485 RTS control on ARM platforms

## Architecture

The project is a single-file CLI application with supporting modules:

| File | Purpose |
|------|---------|
| `src/mbpoll.c` | Main application: CLI parsing, ModBus operations, polling loop |
| `src/serial.c/h` | Serial port configuration types and string conversion utilities |
| `src/custom-rts.c/h` | GPIO-based RTS control for RS-485 (when compiled with `MBPOLL_GPIO_RTS`) |
| `mbpoll-config.h` | Default values and range constants for all parameters |
| `cmake/GitVersion.cmake` | Extracts version from git tags, generates `version-git.h` |
| `tests/` | Unit tests for serial utilities (built with `BUILD_TESTING=ON`) |

### Key Data Structures (in mbpoll.c)

- `xMbPollContext` - Global context holding all runtime state: mode (RTU/TCP), function type, format, slave addresses, serial config, libmodbus context, and statistics
- `eModes`, `eFunctions`, `eFormats` - Enums for protocol mode, ModBus function codes, and output formats

### Conditional Compilation

- `MBPOLL_GPIO_RTS` - Enables GPIO-based RTS control (auto-enabled when libpiduino is found)
- `USE_CHIPIO` - Enables ChipIo I2C-to-serial support (for custom hardware)
- `MBPOLL_FLOAT_DISABLE` - Disables float format on non-IEEE-754 platforms

## Code Conventions

- Hungarian notation for types: `e` prefix for enums, `x` for structs, `i/d/s` for int/double/string
- `v` prefix for void functions, `s` for functions returning strings
- All parameter validation uses `vCheck*` helper functions that call `vSyntaxErrorExit` on failure

## Build Configuration

The CMakeLists.txt supports proper Release/Debug builds:
- **Release**: `-O2`, `-DNDEBUG`, `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`
- **Debug**: `-g`, `-O0`, no NDEBUG (assertions enabled)

Security hardening flags are enabled by default on non-MSVC compilers.

## CI/CD

GitHub Actions workflow runs on push/PR to master/main, building on:
- Ubuntu (Release + Debug)
- macOS (Release)
- Windows (Release)
