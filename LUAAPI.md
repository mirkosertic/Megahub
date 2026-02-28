# Megahub Lua Scripting API Reference

This document describes every function and constant available inside a Lua program running on Megahub. All programs run under Lua 5.4 in a sandboxed environment — only the standard library functions that are safe and relevant for embedded use are available.

Programs are generated automatically by the Blockly editor (see [BLOCKS.md](BLOCKS.md)), but advanced users can write or inspect Lua code directly in the **Lua Preview** panel.

> **Tip:** Every Blockly block generates a Lua program using the functions documented here. You can inspect the generated code live in the **Lua Preview** panel while building your program — it is a good way to learn the API from blocks you already know.

---

## Table of Contents

- [Global Functions](#global-functions)
- [Constants](#constants)
- [Module: `hub` — Hardware I/O and Threading](#module-hub--hardware-io-and-threading)
- [Module: `lego` — LEGO Powered Up Ports](#module-lego--lego-powered-up-ports)
- [Module: `imu` — Orientation and Acceleration](#module-imu--orientation-and-acceleration)
- [Module: `fastled` — Addressable LEDs](#module-fastled--addressable-leds)
- [Module: `gamepad` — Bluetooth Gamepad](#module-gamepad--bluetooth-gamepad)
- [Module: `ui` — Frontend Display](#module-ui--frontend-display)
- [Module: `alg` — Algorithms](#module-alg--algorithms)
- [Module: `deb` — Debug Utilities](#module-deb--debug-utilities)
- [Threading Model](#threading-model)
- [Debugging](#debugging)

---

## Global Functions

These functions are available without any module prefix.

### `wait(ms)`

Pause execution for `ms` milliseconds. Yields the FreeRTOS scheduler during the delay.

```lua
wait(500)   -- pause for 0.5 seconds
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `ms` | integer | Delay in milliseconds |

---

### `print(str)`

Send a string to the Logger panel in the IDE and to the serial console.

```lua
print("Motor speed set")
print("Value: " .. tostring(speed))
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `str` | string | Message to log |

---

### `millis()`

Return the number of milliseconds since the device last booted.

```lua
local start = millis()
wait(100)
local elapsed = millis() - start
print("Elapsed: " .. elapsed .. " ms")
```

**Returns:** integer

---

## Constants

These integer constants are passed as arguments to API functions. The Blockly editor uses them automatically — you only need them when writing Lua directly.

### Port numbers

| Constant | Value | Description |
|----------|-------|-------------|
| `PORT1` | `1` | LEGO port 1 |
| `PORT2` | `2` | LEGO port 2 |
| `PORT3` | `3` | LEGO port 3 |
| `PORT4` | `4` | LEGO port 4 |

### GPIO pin constants

Named constants for the ESP32 GPIO pins exposed by the Megahub hardware. Use these instead of raw integers.

#### Output-capable GPIO pins

These pins support input and output, including FastLED.

| Constant | GPIO number |
|----------|-------------|
| `GPIO13` | 13 |
| `GPIO16` | 16 |
| `GPIO17` | 17 |
| `GPIO25` | 25 |
| `GPIO26` | 26 |
| `GPIO27` | 27 |
| `GPIO32` | 32 |
| `GPIO33` | 33 |

#### Input-only GPIO pins

These pins are hardware input-only on the ESP32 (no output buffer). They can only be used with `hub.digitalRead()`, not with `hub.digitalWrite()`, `hub.pinMode()`, or `fastled.addleds()`.

| Constant | GPIO number |
|----------|-------------|
| `GPIO34` | 34 |
| `GPIO35` | 35 |
| `GPIO36` | 36 |
| `GPIO39` | 39 |

### UART expansion pin constants

GPIO pins exposed via the LEGO WeDo hardware connected to LEGO ports. `UART1_GP*` pins are on the device connected to port 1; `UART2_GP*` pins are on the device connected to port 3.

These pins support `hub.pinMode()`, `hub.digitalRead()`, and `hub.digitalWrite()`, but **not** `fastled.addleds()`.

| Constant | Value | Description |
|----------|-------|-------------|
| `UART1_GP4` | `10000` | Port 1 hardware GPIO 4 |
| `UART1_GP5` | `10001` | Port 1 hardware GPIO 5 |
| `UART1_GP6` | `10002` | Port 1 hardware GPIO 6 |
| `UART1_GP7` | `10003` | Port 1 hardware GPIO 7 |
| `UART2_GP4` | `10004` | Port 3 hardware GPIO 4 |
| `UART2_GP5` | `10005` | Port 3 hardware GPIO 5 |
| `UART2_GP6` | `10006` | Port 3 hardware GPIO 6 |
| `UART2_GP7` | `10007` | Port 3 hardware GPIO 7 |

### FastLED strip type

| Constant | Value | Description |
|----------|-------|-------------|
| `NEOPIXEL` | `1000` | WS2812B / NeoPixel strip |

### Pin modes

| Constant | Value | Description |
|----------|-------|-------------|
| `PINMODE_INPUT` | `3000` | Digital input, floating |
| `PINMODE_INPUT_PULLUP` | `3001` | Digital input with internal pull-up |
| `PINMODE_INPUT_PULLDOWN` | `3002` | Digital input with internal pull-down |
| `PINMODE_OUTPUT` | `3003` | Digital output |

### Gamepad identifiers

| Constant | Value | Description |
|----------|-------|-------------|
| `GAMEPAD1` | `4000` | First paired Bluetooth gamepad |

### Gamepad buttons

| Constant | Value |
|----------|-------|
| `GAMEPAD_BUTTON_1` | `5000` |
| `GAMEPAD_BUTTON_2` | `5001` |
| `GAMEPAD_BUTTON_3` | `5002` |
| `GAMEPAD_BUTTON_4` | `5003` |
| `GAMEPAD_BUTTON_5` | `5004` |
| `GAMEPAD_BUTTON_6` | `5005` |
| `GAMEPAD_BUTTON_7` | `5006` |
| `GAMEPAD_BUTTON_8` | `5007` |
| `GAMEPAD_BUTTON_9` | `5008` |
| `GAMEPAD_BUTTON_10` | `5009` |
| `GAMEPAD_BUTTON_11` | `5010` |
| `GAMEPAD_BUTTON_12` | `5011` |
| `GAMEPAD_BUTTON_13` | `5012` |
| `GAMEPAD_BUTTON_14` | `5013` |
| `GAMEPAD_BUTTON_15` | `5014` |
| `GAMEPAD_BUTTON_16` | `5015` |

### Gamepad axes

| Constant | Value | Description |
|----------|-------|-------------|
| `GAMEPAD_LEFT_X` | `6000` | Left stick horizontal axis |
| `GAMEPAD_LEFT_Y` | `6001` | Left stick vertical axis |
| `GAMEPAD_RIGHT_X` | `6002` | Right stick horizontal axis |
| `GAMEPAD_RIGHT_Y` | `6003` | Right stick vertical axis |
| `GAMEPAD_DPAD` | `6004` | D-pad state |

### IMU value types

| Constant | Value | Unit | Description |
|----------|-------|------|-------------|
| `YAW` | `7000` | ° | Rotation around vertical axis |
| `PITCH` | `7001` | ° | Forward/backward tilt |
| `ROLL` | `7002` | ° | Left/right tilt |
| `ACCELERATION_X` | `7003` | m/s² | Acceleration along X axis |
| `ACCELERATION_Y` | `7004` | m/s² | Acceleration along Y axis |
| `ACCELERATION_Z` | `7005` | m/s² | Acceleration along Z axis |

### UI format types

| Constant | Value | Description |
|----------|-------|-------------|
| `FORMAT_SIMPLE` | `2000` | Plain value display |

---

## Module: `hub` — Hardware I/O and Threading

General device control: GPIO, motor speed, and Lua thread management.

---

### `hub.setmotorspeed(port, speed)`

Set the speed of a LEGO motor connected to the given port.

```lua
hub.setmotorspeed(PORT1, 80)   -- forward at speed 80
hub.setmotorspeed(PORT1, 0)    -- stop
hub.setmotorspeed(PORT2, -50)  -- reverse at speed 50
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `port` | integer | Port number (1–4, use `PORT1`–`PORT4`) |
| `speed` | integer | Motor speed: -127 (full reverse) to +127 (full forward), 0 = stop |

---

### `hub.pinMode(pin, mode)`

Configure a GPIO pin as input or output.

```lua
hub.pinMode(GPIO26, PINMODE_OUTPUT)
hub.pinMode(GPIO33, PINMODE_INPUT_PULLUP)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `pin` | integer | GPIO pin constant |
| `mode` | integer | One of `PINMODE_INPUT`, `PINMODE_INPUT_PULLUP`, `PINMODE_INPUT_PULLDOWN`, `PINMODE_OUTPUT` |

**Accepted pins:** output-capable GPIO pins (`GPIO13`–`GPIO33`) and UART expansion pins (`UART1_GP4`–`UART2_GP7`). Input-only pins (`GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`) are not supported.

---

### `hub.digitalRead(pin)`

Read the current logic level of a digital input pin.

```lua
local state = hub.digitalRead(GPIO33)
if state == 1 then
    print("Button pressed")
end
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `pin` | integer | GPIO pin constant |

**Returns:** integer — `1` for HIGH, `0` for LOW

**Accepted pins:** all GPIO pins (`GPIO13`–`GPIO39`) and UART expansion pins (`UART1_GP4`–`UART2_GP7`).

---

### `hub.digitalWrite(pin, value)`

Write a logic level to a digital output pin.

```lua
hub.digitalWrite(GPIO26, 1)   -- HIGH
hub.digitalWrite(GPIO26, 0)   -- LOW
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `pin` | integer | GPIO pin constant |
| `value` | integer | `1` for HIGH, `0` for LOW |

**Accepted pins:** output-capable GPIO pins (`GPIO13`–`GPIO33`) and UART expansion pins (`UART1_GP4`–`UART2_GP7`). Input-only pins (`GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`) are not supported.

---

### `hub.init(function)`

Run the provided function once as an initialization block before the main program loop. Used by the Blockly editor to separate setup code from loop code.

```lua
hub.init(function()
    hub.setmotorspeed(PORT1, 0)
    fastled.addleds(NEOPIXEL, GPIO13, 8)
end)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `function` | function | Zero-argument function to execute once |

---

### `hub.startthread(name, blockId, stackSize, profiling, function)`

Start a new concurrent Lua thread running on a dedicated FreeRTOS task. Returns a task handle that can be used with `hub.stopthread()`.

```lua
local motorThread = hub.startthread("motor", "block_1", 4096, false, function()
    hub.setmotorspeed(PORT1, 60)
    wait(500)
    hub.setmotorspeed(PORT1, 0)
    wait(500)
end)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Human-readable thread name (for diagnostics) |
| `blockId` | string | Blockly block ID (used for profiling overlay in the IDE) |
| `stackSize` | integer | FreeRTOS stack size in bytes (minimum ~4096) |
| `profiling` | boolean | If `true`, reports min/avg/max execution time to the IDE every 10 seconds |
| `function` | function | Zero-argument function called in a loop on each task tick |

**Returns:** task handle (userdata) — pass to `hub.stopthread()` to stop the thread

**Notes:**
- The thread function is called repeatedly in a tight loop with a 1 ms `vTaskDelay` between iterations
- If the thread function raises a Lua error, the thread exits and all LEGO device ports are reinitialized
- Each thread has its own Lua coroutine state

---

### `hub.stopthread(handle)`

Stop a running thread started with `hub.startthread()`.

```lua
hub.stopthread(motorThread)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `handle` | userdata | Task handle returned by `hub.startthread()` |

---

## Module: `lego` — LEGO Powered Up Ports

Read sensor data and configure device modes on LEGO Powered Up ports.

---

### `lego.selectmode(port, mode)`

Select the active measurement mode for the device connected to a port. Mode IDs are device-specific — see the Port Status panel in the IDE for the modes available on your connected device.

```lua
lego.selectmode(PORT1, 0)   -- select mode 0 (often the default sensor mode)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `port` | integer | Port number (1–4) |
| `mode` | integer | Mode index (device-specific, 0-based) |

---

### `lego.getmodedataset(port, dataset)`

Read a value from the currently selected mode on a port. A mode may provide multiple datasets (e.g. RGB channels from a colour sensor are three separate datasets).

```lua
lego.selectmode(PORT2, 0)
local color = lego.getmodedataset(PORT2, 0)
print("Color index: " .. color)
```

```lua
-- Read all three RGB channels (mode 6 on BOOST color/distance sensor)
lego.selectmode(PORT3, 6)
local r = lego.getmodedataset(PORT3, 0)
local g = lego.getmodedataset(PORT3, 1)
local b = lego.getmodedataset(PORT3, 2)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `port` | integer | Port number (1–4) |
| `dataset` | integer | Dataset index within the current mode (0-based) |

**Returns:** number — sensor value for the selected mode/dataset. Returns `0` if no device is connected or the dataset is unavailable.

---

### Common sensor modes quick reference

Mode IDs and dataset layouts are device-specific. The **Port Status** panel in the IDE lists every mode reported by the connected device, including its name, units, and number of datasets — that is always the authoritative source.

The table below documents confirmed modes for commonly used devices.

#### BOOST Color and Distance Sensor (device ID 37)

| Mode | Name | Units | Datasets | Description |
|------|------|-------|----------|-------------|
| `0` | `COLOR` | IDX | 1 | Detected colour index (0–10; 0 = no colour) |
| `1` | `PROX` | DIS | 1 | Proximity distance (0–10; 0 = very close) |
| `6` | `RGB I` | RAW | 3 | Raw RGB intensities — dataset 0=R, 1=G, 2=B |

```lua
-- Read proximity on port 2
lego.selectmode(PORT2, 1)
local prox = lego.getmodedataset(PORT2, 0)

-- Read raw RGB on port 3
lego.selectmode(PORT3, 6)
local r = lego.getmodedataset(PORT3, 0)
local g = lego.getmodedataset(PORT3, 1)
local b = lego.getmodedataset(PORT3, 2)
```

> Mode data for WeDo 2.0, SPIKE, and Technic devices follows the same pattern. Connect the device, open the Port Status panel, and read the mode list directly from the device. Contributions to this table are welcome.

---

## Module: `imu` — Orientation and Acceleration

Read data from the on-board MPU6050 6-axis IMU. Values are updated every 100 ms.

---

### `imu.value(type)`

Read an IMU measurement.

```lua
local yaw   = imu.value(YAW)
local pitch = imu.value(PITCH)
local roll  = imu.value(ROLL)
local ax    = imu.value(ACCELERATION_X)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | integer | One of the IMU value constants (see [Constants](#imu-value-types)) |

**Returns:** number — the requested measurement. Returns `0` for unsupported types.

| Type | Unit | Typical range |
|------|------|---------------|
| `YAW` | ° | 0–360 |
| `PITCH` | ° | -90–+90 |
| `ROLL` | ° | -180–+180 |
| `ACCELERATION_X/Y/Z` | m/s² | varies |

---

## Module: `fastled` — Addressable LEDs

Control WS2812B (NeoPixel) LED strips. Supported output pins: `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33`.

---

### `fastled.addleds(type, pin, count)`

Initialize a LED strip. Must be called once before using `fastled.set()` or `fastled.show()`.

```lua
fastled.addleds(NEOPIXEL, GPIO13, 8)   -- 8 NeoPixels on GPIO 13
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | integer | Strip type — currently only `NEOPIXEL` (1000) is supported |
| `pin` | integer | GPIO output pin — one of the output-capable GPIO constants: `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33`. UART expansion pins and input-only pins are not supported. |
| `count` | integer | Number of LEDs in the strip |

**Note:** `addleds` automatically calls `fastled.clear()` after initializing.

---

### `fastled.set(index, r, g, b)`

Set the colour of a single LED. Changes are not visible until `fastled.show()` is called.

```lua
fastled.set(0, 255, 0, 0)   -- first LED red
fastled.set(1, 0, 255, 0)   -- second LED green
fastled.set(2, 0, 0, 255)   -- third LED blue
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | integer | LED index (0-based) |
| `r` | integer | Red channel (0–255) |
| `g` | integer | Green channel (0–255) |
| `b` | integer | Blue channel (0–255) |

---

### `fastled.show()`

Push all buffered LED colours to the strip. Nothing is displayed until this is called.

```lua
fastled.set(0, 255, 128, 0)
fastled.show()
```

---

### `fastled.clear()`

Set all LEDs to off (black). Does not call `show()` — call `fastled.show()` afterwards to apply.

```lua
fastled.clear()
fastled.show()
```

---

## Module: `gamepad` — Bluetooth Gamepad

Read the state of a paired Bluetooth Classic HID gamepad. Pair the gamepad via the **Bluetooth Devices** panel in the IDE before using these functions.

---

### `gamepad.connected(index)`

Check whether the gamepad is currently connected.

```lua
if gamepad.connected(GAMEPAD1) then
    print("Gamepad ready")
end
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | integer | Gamepad identifier — currently only `GAMEPAD1` (4000) is supported |

**Returns:** boolean

> **Known issue:** Due to a firmware bug, `gamepad.connected()` may not return a value when the gamepad is connected. The return value is only reliable when the gamepad is *not* connected (returns `false`). Use `gamepad.buttonsraw()` as a workaround to verify connectivity.

---

### `gamepad.buttonpressed(index, button)`

Check whether a specific button is currently pressed.

```lua
if gamepad.buttonpressed(GAMEPAD1, GAMEPAD_BUTTON_1) then
    hub.setmotorspeed(PORT1, 80)
end
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | integer | Gamepad identifier (`GAMEPAD1`) |
| `button` | integer | Button constant (`GAMEPAD_BUTTON_1` – `GAMEPAD_BUTTON_16`) |

**Returns:** boolean — `true` if pressed, `false` otherwise

---

### `gamepad.value(index, axis)`

Read an analog axis value.

```lua
local x = gamepad.value(GAMEPAD1, GAMEPAD_LEFT_X)
local y = gamepad.value(GAMEPAD1, GAMEPAD_LEFT_Y)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | integer | Gamepad identifier (`GAMEPAD1`) |
| `axis` | integer | Axis constant (see [Gamepad axes](#gamepad-axes)) |

**Returns:** number
- Stick axes: -32768 to +32767
- D-pad (`GAMEPAD_DPAD`): device-specific integer encoding direction

> **Known issue:** Due to a firmware bug, `GAMEPAD_RIGHT_X` and `GAMEPAD_RIGHT_Y` currently return the **left** stick values instead of the right stick. Use only `GAMEPAD_LEFT_X` and `GAMEPAD_LEFT_Y` for reliable readings until this is fixed.

---

### `gamepad.buttonsraw(index)`

Read the raw button bitmask. Bit 0 = button 1, bit 1 = button 2, etc.

```lua
local bits = gamepad.buttonsraw(GAMEPAD1)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | integer | Gamepad identifier (`GAMEPAD1`) |

**Returns:** integer bitmask

---

## Module: `ui` — Frontend Display

Send values to the IDE for real-time display in the **Logger** panel.

---

### `ui.showvalue(label, format, value)`

Display a labelled value in the IDE. The value is queued and pushed to the frontend asynchronously.

```lua
ui.showvalue("Motor speed", FORMAT_SIMPLE, speed)
ui.showvalue("Connected", FORMAT_SIMPLE, gamepad.connected(GAMEPAD1))
ui.showvalue("Sensor", FORMAT_SIMPLE, lego.getmodedataset(PORT1, 0))
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | string | Label displayed before the value |
| `format` | integer | Display format — currently only `FORMAT_SIMPLE` (2000) |
| `value` | number, string, or boolean | Value to display |

---

## Module: `alg` — Algorithms

Mathematical algorithms for control applications.

---

### `alg.PID(blockId, setpoint, pv, kp, ki, kd, outMin, outMax)`

Compute one step of a PID controller. Each unique `blockId` string maintains its own independent controller state (integral accumulator, previous error, and timestamp). The first call initialises the state; subsequent calls update it.

The implementation uses:
- **Derivative on measurement** (not on error) to avoid setpoint kick
- **Anti-windup** via back-calculation clamping of the integral term

```lua
-- Follow a target distance of 30 cm using a distance sensor
local setpoint = 30
local pv = lego.getmodedataset(PORT1, 0)
local output = alg.PID("distance_ctrl", setpoint, pv, 2.0, 0.1, 0.05, -127, 127)
hub.setmotorspeed(PORT2, math.floor(output))
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `blockId` | string | Unique ID for this controller instance |
| `setpoint` | number | Target (desired) value |
| `pv` | number | Process variable (current measured value) |
| `kp` | number | Proportional gain |
| `ki` | number | Integral gain |
| `kd` | number | Derivative gain |
| `outMin` | number | Lower clamp on the output |
| `outMax` | number | Upper clamp on the output |

**Returns:** number — control output, clamped to [`outMin`, `outMax`]. Returns `0` on the first call (dt is zero on the first iteration).

---

### `alg.resetPID(blockId)`

Reset the state (integral, previous error, and timestamp) of a specific PID controller.

```lua
alg.resetPID("distance_ctrl")
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `blockId` | string | ID of the controller to reset |

---

### `alg.clearAllPID()`

Clear the state of all PID controller instances.

```lua
alg.clearAllPID()
```

---

## Module: `deb` — Debug Utilities

Diagnostic helpers for development.

---

### `deb.freeHeap()`

Return the number of bytes of free heap memory currently available on the ESP32.

```lua
print("Free heap: " .. deb.freeHeap() .. " bytes")
```

**Returns:** integer

---

## Threading Model

Megahub programs use cooperative multitasking via FreeRTOS tasks.

- The **main Lua program** runs on a single thread and executes to completion (or until `wait()` yields).
- **Additional threads** created with `hub.startthread()` each run on their own FreeRTOS task with a dedicated Lua coroutine state.
- All threads share the same device state (ports, LED strip, etc.), so call order is not guaranteed if multiple threads control the same device simultaneously.
- When the program is stopped from the IDE, all running threads are cancelled and all LEGO device ports are reinitialized.
- If any thread exits due to a Lua error, all ports are also reinitialized automatically.

**Typical program structure:**

```lua
-- Initialization: runs once
hub.init(function()
    hub.setmotorspeed(PORT1, 0)
    fastled.addleds(NEOPIXEL, GPIO13, 8)
    fastled.clear()
    fastled.show()
end)

-- Thread 1: motor control loop
local motorThread = hub.startthread("motor", "blk_motor", 4096, false, function()
    local speed = gamepad.value(GAMEPAD1, GAMEPAD_LEFT_Y) / 256
    hub.setmotorspeed(PORT1, math.floor(speed))
    wait(20)
end)

-- Thread 2: LED feedback loop
local ledThread = hub.startthread("leds", "blk_leds", 4096, false, function()
    local yaw = imu.value(YAW)
    local hue = math.floor(yaw / 360 * 255)
    for i = 0, 7 do
        fastled.set(i, hue, 0, 255 - hue)
    end
    fastled.show()
    wait(50)
end)
```

---

## Debugging

### Where errors appear

All errors from running Lua code are sent to the **Logger** panel in the IDE and to the serial console (115200 baud). The format is:

```
[string]:LINE: ERROR MESSAGE
```

Example:

```
[string]:5: attempt to perform arithmetic on a nil value (global 'speed')
```

The line number refers to the line in your Lua program (visible in the Lua Preview panel). The error appears in the Logger immediately when the thread or program crashes.

### What triggers port reinitialization

Any of the following causes **all four LEGO ports to reinitialize** (motors stop, sensors reset):

| Event | Ports reinit? |
|-------|--------------|
| Click **Execute** | Yes — always |
| Click **Stop** | Yes — always |
| Runtime error in a thread | Yes |
| Runtime error in main program body | Yes |
| Error inside `hub.init()` | **No** — the error is logged but execution continues |

> `hub.init()` errors are silently swallowed after logging. If your initialization seems to have no effect, check the Logger for a `[WARN] Error processing Lua function` message.

### Using `print()` for step-by-step tracing

Insert `print()` calls to trace execution flow and variable values:

```lua
print("Starting motor loop")
local speed = gamepad.value(GAMEPAD1, GAMEPAD_LEFT_Y)
print("Raw stick Y: " .. tostring(speed))
hub.setmotorspeed(PORT1, math.floor(speed / 256))
```

Every `print()` call appears immediately in the Logger panel.

### Checking memory with `deb.freeHeap()`

If a program crashes after running for a while, a memory leak may be the cause. Log the free heap regularly to watch for a downward trend:

```lua
local ledThread = hub.startthread("monitor", "blk_mon", 2048, false, function()
    print("Free heap: " .. deb.freeHeap() .. " bytes")
    wait(5000)
end)
```

A healthy program holds a roughly constant heap. If the number drops steadily, the program is allocating memory it never frees (for example, building up large strings or tables inside a tight loop).

### Profiling thread performance

Pass `profiling = true` to `hub.startthread()` to receive timing statistics every 10 seconds in the IDE:

```lua
hub.startthread("motor", "blk_motor", 4096, true, function()
    -- ...
    wait(20)
end)
```

The IDE displays a `thread_statistics` event with `min`, `avg`, and `max` iteration times in **microseconds**. Use this to confirm your thread is keeping up with its intended update rate — for example, a motor control loop running at 20 ms intervals should show an avg well below 20 000 µs.

### Common pitfalls

| Symptom | Likely cause |
|---------|-------------|
| LEDs don't change colour | `fastled.set()` was called but `fastled.show()` was not |
| Sensor always returns `0` | `lego.selectmode()` was not called before `lego.getmodedataset()` |
| Program does nothing, no error | Error inside `hub.init()` — check Logger for `[WARN]` |
| Device crashes / reboots | Thread stack too small; try increasing `stackSize` (minimum ~4096 bytes) |
| Motor doesn't stop after program ends | Not expected — motors always stop on Execute, Stop, or crash |
| `hub.init()` changes have no effect | `hub.init()` must be called **before** `hub.startthread()` — it runs synchronously |
