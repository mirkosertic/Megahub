# Claude AI Assistant Guide for Megahub Project

## Project Overview

Megahub is an ESP32-based robotics experimentation platform that combines LEGO WeDo/Powered Up! device support with Lua scripting and Blockly visual programming. The project consists of:

- **Firmware**: C++ embedded code running on ESP32 (PlatformIO)
- **Frontend**: Web-based IDE built with Vite, Blockly, and vanilla JavaScript
- **Libraries**: Custom ESP32 libraries for Bluetooth, UART, IMU, etc.

## Critical Development Guidelines

### Memory Management

**ESP32 memory is severely constrained. Always be mindful of:**

1. **Stack Usage**: The ESP32 has limited stack space (~8KB per task). Avoid large local arrays or deep recursion.
2. **Heap Fragmentation**: Minimize dynamic allocations. Reuse buffers where possible.
3. **PSRAM**: The board may have PSRAM, but don't assume it's always available.
4. **String Operations**: Be careful with String class; prefer fixed buffers or std::string with reserve().
5. **Lua VM Memory**: The Lua interpreter runs in constrained memory. Large scripts can cause crashes.

**Memory Best Practices:**
- Use the already existing custim `INFO`, `ERROR` or `WARN` logging macros instead of excessive `Serial.print()`
- Avoid creating temporary String objects in loops
- Monitor free heap with `ESP.getFreeHeap()` during development
- Use `const` and `PROGMEM` for large constant data

### Bluetooth BLE Connectivity

**Bluetooth is complex and fragile on ESP32. Follow these rules strictly:**

1. **Connection State Management**: Always track connection state properly. Don't assume connections persist.
2. **MTU Negotiation**: Handle MTU size properly. Default is 23 bytes, negotiated up to 512.
3. **Notification Callbacks**: BLE notifications are asynchronous. Handle them in a non-blocking manner.
4. **Pairing & Bonding**: The Web Bluetooth API has specific pairing requirements.
5. **Multiple Connections**: The ESP32 can handle multiple BLE connections, but each consumes memory.
6. **Classic BT vs BLE**: The project uses both Classic Bluetooth (for HID gamepads) and BLE (for LEGO devices and Web Bluetooth API).

**Bluetooth Best Practices:**
- Never block in BLE callbacks
- Always implement proper error handling for disconnections
- Test with actual devices, not just simulators
- Be aware of Web Bluetooth API browser compatibility (Chrome/Edge/Opera only)
- Handle reconnection scenarios gracefully
- Monitor connection events and clean up resources on disconnect

### LEGO Device Communication

**LEGO Powered Up protocol specifics:**

1. **UART Protocol**: Uses custom UART communication at specific baud rates
2. **Device Detection**: Devices send identification messages on connection
3. **Command Timing**: Some devices require specific timing between commands
4. **Power Management**: Devices can enter sleep mode; handle wake-up properly
5. **Port Configuration**: Track which devices are on which ports (0-3)

**LEGO Best Practices:**
- Implement proper device initialization sequences
- Handle device disconnection/reconnection
- Respect command rate limits to avoid overwhelming devices
- Parse device responses correctly (byte order, message structure)

### Build System & Project Structure

**PlatformIO Configuration:**
- Build environment: `esp32dev`
- Framework: Arduino + ESP-IDF hybrid
- Custom partition table: [custom_4mb_noota.csv](custom_4mb_noota.csv)
- Pre-build scripts: [gitversion.py](gitversion.py), [buildfrontend.py](buildfrontend.py), [embedfiles.py](embedfiles.py)

**Directory Structure:**
```
/lib/                   Custom ESP32 libraries
  /btremote/           Bluetooth BLE server & Classic BT HID host
  /commands/           Command processing system
  /configuration/      WiFi & system configuration
  /hubwebserver/       HTTP server (PsychicHttp)
  /imu/                MPU6050 IMU support
  /lpfuart/            LEGO Powered Up UART protocol
  /lua/                Lua 5.4 interpreter
  /megahub/            Main application library
  /portstatus/         Port status tracking
  /inputdevices/       Gamepad/input device support
  /statusmonitor/      System monitoring

/src/                  Main application entry point
  main.cpp             Arduino setup() and loop()

/frontend/             Web-based IDE
  /src/                JavaScript source files
  package.json         NPM dependencies (Blockly, Vite)
  vite.config.js       Vite build configuration
```

**Build Process:**
1. Pre-build scripts run (git version, frontend build, file embedding)
2. Frontend is built and embedded into firmware as compressed files
3. PlatformIO compiles C++ code
4. Single binary is flashed to ESP32

### Frontend Development

**Frontend Tech Stack:**
- **Vite**: Build tool and dev server
- **Blockly**: Visual programming blocks
- **Vanilla JS**: No framework (intentionally lightweight)
- **Web Bluetooth API**: Browser-based BLE communication
- **Prism.js**: Syntax highlighting for Lua code

**Frontend Build Modes:**
- `dev`: Development mode with hot reload
- `bt`: Bluetooth mode (Deployed as Megahub IDE on Github)
- `web`: Web mode (WebServer is running on firmware)

**Frontend Best Practices:**
- Keep bundle size small (embedded in ESP32 flash)
- Minimize dependencies
- Handle Web Bluetooth API errors gracefully
- Test in supported browsers (Chrome, Edge, Opera)
- Use compression (gzip) for embedded files

### Code Review & Implementation

**IMPORTANT: When making code changes to this project:**

For significant code changes or new features, please use the **implement-review-loop** agent. This ensures:
- Code is reviewed before being committed
- Potential issues are caught early
- Changes follow project conventions
- Memory and Bluetooth considerations are validated

To invoke this agent, Claude should use:
```
Task tool with subagent_type="implement-review-loop"
```

**When to use implement-review-loop:**
- Adding new features to firmware or frontend
- Modifying Bluetooth communication code
- Changing memory-critical sections
- Refactoring core libraries
- Adding new Lua bindings

**When NOT to use it:**
- Trivial documentation updates
- Simple bug fixes (< 10 lines)
- Configuration file changes

### Common Pitfalls to Avoid

1. **Bluetooth Race Conditions**: Always use proper synchronization when accessing BLE state from multiple tasks
2. **Memory Leaks**: Every `new` or `malloc` must have corresponding `delete` or `free`
3. **Stack Overflow**: Don't allocate large arrays on the stack; use heap or static storage
4. **String Concatenation**: Avoid repeated `+=` operations on String objects
5. **Blocking Operations**: Never block in BLE callbacks, ISRs, or FreeRTOS tasks without proper timeout
6. **Lua Errors**: Always wrap Lua execution in error handlers to prevent firmware crashes
7. **Frontend Bundle Size**: Keep JavaScript bundles small; the ESP32 flash is limited
8. **Web Bluetooth Permissions**: User gesture required to trigger Bluetooth pairing
9. **LEGO Device Timeouts**: Some LEGO devices timeout if no command is received within ~1 second
10. **IMU Calibration**: MPU6050 requires proper initialization sequence; don't skip delays

### Testing & Debugging

**Serial Monitor:**
- Baud rate: 115200
- Use `pio device monitor` or VSCode PlatformIO extension
- Log levels controlled by `CORE_DEBUG_LEVEL` (currently set to 2)

**Common Debugging Techniques:**
- Monitor free heap: `ESP.getFreeHeap()`
- Check task stack: `uxTaskGetStackHighWaterMark()`
- Use ESP32 exception decoder for crash analysis
- Web browser console for frontend debugging
- Bluetooth HCI logs (enable in ESP-IDF menuconfig)

**Unit Testing:**
- Framework: Unity (PlatformIO)
- Test files should be in `/test` directory
- Run with `pio test`

### Git Workflow

- Main branch: `main`
- Commit messages should be clear and descriptive
- Include "Co-Authored-By: Claude <noreply@anthropic.com>" when applicable
- Don't commit secrets or credentials (use `config/secrets.ini`)

### External Dependencies

**PlatformIO Libraries:**
- ArduinoJson 7.4.2
- PubSubClient 2.8.0 (MQTT)
- PsychicHttp 2.1.1 (HTTP server)
- SC16IS752 (UART expander)
- MPU6050 (IMU)
- FastLED (LED control)

**Frontend Dependencies:**
- Blockly 12.3.1
- Vite 5.0.0
- Prism.js 1.30.0

### Important Files Reference

- [platformio.ini](platformio.ini) - Build configuration
- [src/main.cpp](src/main.cpp) - Application entry point
- [lib/btremote/src/btremote.cpp](lib/btremote/src/btremote.cpp) - Bluetooth implementation
- [lib/megahub/src/libluahub.cpp](lib/megahub/src/libluahub.cpp) - Lua bindings
- [frontend/src/bleclient.js](frontend/src/bleclient.js) - Web Bluetooth client
- [frontend/src/index.js](frontend/src/index.js) - Frontend main application

### Key Architecture Concepts

1. **Dual Mode Operation**: The device can operate in Bluetooth mode (default) or WiFi mode
2. **Embedded Web Server**: Frontend files are embedded in firmware and served via PsychicHttp
3. **Command Pattern**: All operations use a command pattern (see [lib/commands](lib/commands))
4. **Port Abstraction**: Generic port interface supports both LEGO and standard devices
5. **Lua Sandbox**: Lua scripts run in a controlled environment with custom bindings
6. **Blockly Code Generation**: Blockly blocks generate Lua code which is then executed

### Performance Considerations

- **WiFi vs Bluetooth**: WiFi has better throughput but higher power consumption
- **Lua Execution**: Lua is interpreted; performance-critical code should be in C++
- **BLE MTU**: Larger MTU = better throughput but requires negotiation
- **FastLED**: Can consume significant CPU time; use carefully in time-critical code
- **I2C Bus Speed**: IMU and other I2C devices share a bus; consider timing

### Resources & Documentation

- [README.md](README.md) - User-facing documentation
- [BLOCKS.md](BLOCKS.md) - Blockly blocks reference, automatically generated
- ESP32 Documentation: https://docs.espressif.com/projects/esp-idf/
- Web Bluetooth API: https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API
- Lua 5.4 Reference: https://www.lua.org/manual/5.4/
- Blockly Documentation: https://developers.google.com/blockly

---

## Quick Reference for Claude

**Before making changes:**
1. Read relevant source files to understand current implementation
2. Consider memory implications (heap, stack, flash)
3. For Bluetooth code, review connection state management
4. For LEGO code, verify protocol compliance
5. Use implement-review-loop agent for significant changes

**When in doubt:**
- Ask the user for clarification
- Check existing code for patterns and conventions
- Test on actual hardware when possible
- Document assumptions and limitations

**Remember:**
- ESP32 memory is limited - optimize aggressively
- Bluetooth is fragile - handle errors gracefully
- LEGO protocol is strict - follow specifications
- Frontend must be lightweight - minimize bundle size
- Always consider multi-threading implications (FreeRTOS)
