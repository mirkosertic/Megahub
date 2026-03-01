# Claude AI Assistant Guide for Megahub Project

## Project Overview

Megahub is an ESP32-based robotics experimentation platform combining LEGO WeDo/Powered Up! device support with Lua scripting and Blockly visual programming.

- **Firmware**: C++ on ESP32, built with PlatformIO
- **Frontend**: Web-based IDE (Vite, Blockly, vanilla JS) embedded in firmware flash
- **Libraries**: Custom ESP32 libs for BLE, UART, IMU, Lua, HTTP in `/lib/`

## Critical Rules

1. **ALWAYS use the `implement-review-loop` agent** for code changes
2. **ALWAYS update README.md** when adding, removing, or changing user-facing behavior, config options, or troubleshooting info
3. **NEVER duplicate README content here** — this file is for development context only
4. **Always use Context7 MCP** for library/API documentation without being asked

## Skill Files (auto-loaded when relevant)

Domain-specific context is in `.claude/skills/` and loads automatically:

| Skill | Triggers on |
|-------|-------------|
| `frontend` | Work in `/frontend/` — theme, UI, components, BLE client, build modes |
| `blockly` | Blockly blocks — block definitions, Lua generators, toolbox, BLOCKS.md |
| `firmware-core` | General firmware — PlatformIO, build scripts, library structure |
| `ble` | Bluetooth — BLE GATT server, Classic BT HID, connection handling |
| `lego-protocol` | LEGO devices — UART, device init, motor noise/corruption |
| `memory-freertos` | Memory, heap, stack, FreeRTOS tasks, synchronization |
| `lua-integration` | Lua scripting, bindings, error handling, Blockly→Lua pipeline |
| `documentation` | Doc structure, README outline, style rules, what to update and when |
| `linting` | Formatting and linting — clang-format, ESLint, Prettier, lefthook |

## Key Architecture

1. **Dual mode**: Bluetooth mode (default, Web Bluetooth API) or WiFi mode (embedded HTTP server)
2. **Embedded frontend**: Built by Vite, gzipped, embedded in firmware as C arrays
3. **Command pattern**: All device operations use a command pattern (`lib/commands/`)
4. **Port abstraction**: Generic port interface supports both LEGO and standard devices
5. **Lua sandbox**: Lua 5.4 scripts run in a controlled environment with custom bindings
6. **Blockly → Lua**: Blockly blocks generate Lua code which is then executed on the device

## Important Files

| File | Purpose |
|------|---------|
| [platformio.ini](platformio.ini) | Build configuration |
| [src/main.cpp](src/main.cpp) | Application entry point |
| [lib/btremote/src/btremote.cpp](lib/btremote/src/btremote.cpp) | BLE server + Classic BT host |
| [lib/megahub/src/libluahub.cpp](lib/megahub/src/libluahub.cpp) | Lua bindings |
| [frontend/src/index.js](frontend/src/index.js) | Frontend app controller |
| [frontend/src/bleclient.js](frontend/src/bleclient.js) | Web Bluetooth client |
| [frontend/src/theme.css](frontend/src/theme.css) | VS Code theme CSS variables |
| [frontend/src/styles.css](frontend/src/styles.css) | Global styles and all UI component CSS |
| [README.md](README.md) | User-facing documentation |

## Quick Reference

**Before any change:**
1. Read the relevant source files first — understand before modifying
2. Load the appropriate skill file for domain context
3. For firmware: consider memory (heap, stack, flash) implications
4. For frontend: check the theme system and existing UI patterns
5. Use the `implement-review-loop` agent for all significant changes

**When in doubt:** Ask. Never guess at protocol details, memory limits, or BLE behavior.
