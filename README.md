# Megahub

![PlatformIO CI](https://github.com/mirkosertic/Megahub/actions/workflows/build.yml/badge.svg?branch=main)

## Introduction

Megahub is an ESP32-based experimentation platform designed for robotics enthusiasts and tinkerers. It brings together the versatility of the entire ESP32 ecosystem with convenient support for LEGO© WeDo/Powered Up! devices, offering both Lua scripting and Blockly visual programming for easy control and automation.

This open-source project empowers makers to explore and build robotic creations using the full capabilities of the ESP32 platform – from WiFi and Bluetooth connectivity to a vast array of sensors and actuators – while seamlessly integrating popular LEGO© components alongside standard electronics.
It comes with the hardware schematics, production files and the Megahub firmware as a PlatformIO project as well.

**Build and use it at your own Risk!!**

## Key Features

- **KiCad**: All schematics and PCB production files are available in KiCad and Gerber format
- **WiFi Enabled**: Built-in wireless connectivity for remote control and programming
- **Bluetooth Enabled**: Acts as a Bluetooth BLE Server and Client
- **IDE**: Configure and program Megahub through the intuitive [Megahub IDE](https://mirkosertic.github.io/Megahub/)
- **Visual Programming Interface**: Blockly visual programming for beginners
- **SD-Card**: Built-in SD-card reader to store multiple projects (autostart possible)
- **4 Motors / Devices**: Connect up to 4 generic or LEGO© WeDo 2.0/Powered Up! devices simultaneously (motors, sensors, etc.)
- **Built-in IMU**: Integrated MPU6050 6-axis accelerometer and gyroscope for motion sensing
- **FastLED/NeoPixel Support**: Built-in support for addressable RGB LED strips
- **Flexible GPIO**: Additional GPIO pins available for custom sensors, buttons, and peripherals
- **Gamepad**: Can be paired with Bluetooth Classic HID compatible Gamepads (8BitDo M30, ...)
- **BT/WiFi switch**: Allow configuration over Bluetooth API instead of Restful HTTP API is comming soon...
- **ROS(Robot Operating System)**: Micro ROS support is comming soon...
- **MQTT**: MQTT support is comming soon...

## Why not Micropython on existing hardware?

- I wanted to learn something new
- Designing Hard- and Software work together is fun
- There is no truly open system with the same capabilities available

## How It Differs from Existing Solutions

### vs. LEGO© Powered Up!

- **Open Source**: Complete control over firmware and functionality
- **Expandable**: Access to additional GPIO pins for custom hardware
- **Programming Flexibility**: Choose between Lua scripting or visual programming
- **Built-in Features**: Integrated IMU and LED controller without additional hubs
- **Cost-effective**: Based on affordable ESP32 hardware

### vs. fischertechnik© RX Controller

- **LEGO Compatibility**: Native support for LEGO© WeDo 2.0 protocol
- **Web-based Programming**: No proprietary software required
- **Cost-effective**: Based on affordable ESP32 hardware
- **Visual Programming**: Blockly support makes it accessible for all skill levels

## WebUI Screenshots

**Project management**
![Project Management](docs/screenshot_ide.png)

**IMU interaction**
![IMU Interaction](docs/screenshot_imu.png)

**LEGO / Gamepad interaction**
![Lego / Gamepad interaction](docs/screenshot_lego_gamepad.png)

## Hardware Photos

![Assembled prototype](docs/prototype_iteration1.jpg)

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (either as a CLI tool or as a VSCode extension)
- An ESP32 development board
- USB cable for flashing

### Building the Firmware

1. Clone the repository:
   ```bash
   git clone https://github.com/mirkosertic/Megahub.git
   cd Megahub
   ```

2. Build the project using PlatformIO:
   ```bash
   pio run
   ```

   Or if using VSCode with the PlatformIO extension, click the **Build** button in the bottom toolbar.

### Flashing the Board

1. Connect your ESP32 board via USB.

2. Flash the firmware:
   ```bash
   pio run --target upload
   ```

   Or use the **Upload** button in the PlatformIO toolbar in VSCode.

### First Boot and Configuration

By default, the Megahub firmware runs in **Bluetooth mode**. To configure your board and start programming:

1. Open the [Megahub IDE](https://mirkosertic.github.io/Megahub/) in your web browser.

2. The web-based IDE uses the Web Bluetooth API to connect to your Megahub device. Make sure you are using a compatible browser (Chrome, Edge, or Opera on desktop).

3. Click **Connect** in the IDE and select your Megahub device from the Bluetooth pairing dialog.

4. Once connected, you can configure WiFi settings, upload projects, and program your device using either Lua scripting or Blockly visual programming.

### Serial Monitor

To view debug output from the board:
```bash
pio device monitor
```

The default baud rate is 115200.

## Documentation

### Blockly Blocks Reference

For a complete reference of all available Blockly blocks including visual examples, see the [Blockly Blocks Documentation](BLOCKS.md).
