# Megahub

[![PlatformIO CI](https://github.com/mirkosertic/Megahub/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/mirkosertic/Megahub/actions/workflows/build.yml)

## Introduction

Megahub is an ESP32-based alternative hub designed for LEGO&copy; enthusiasts and robotics builders. It combines the flexibility of the ESP32 platform with native support for LEGO&copy; WeDo/Powered Up! devices, offering both Lua scripting and Blockly visual programming for easy control and automation.

This project provides a powerful, open-source alternative to proprietary solutions, giving makers complete control over their robotic creations while maintaining compatibility with popular LEGO&copy; sensors and motors.
It comes with the hardware schematics, production files and the Megahub firmware as a PlatformIO project as well.

** Build and use it at your own Risk!! **

## Key Features

- **KiCad**: All schematics and PCB production files are available in KiCad and Gerber format
- **WiFi Enabled**: Built-in wireless connectivity for remote control and programming
- **Bluetooth Enabled**: Acts as a Bluetooth BLE Server and Client
- **IDE**: Configure and program Megahub through the intuitive [Megahub IDE](https://mirkosertic.github.io/Megahub/)
- **Visual Programming Interface**: Blockly visual programming for beginners
- **SD-Card**: Built-in SD-card reader to store multiple projects (autostart possible)
- **4 UART Ports**: Connect up to 4 LEGO&copy; WeDo/Powered Up! devices simultaneously (motors, sensors, etc.)
- **Built-in IMU**: Integrated MPU6050 6-axis accelerometer and gyroscope for motion sensing
- **FastLED/NeoPixel Support**: Built-in support for addressable RGB LED strips
- **Flexible GPIO**: Additional GPIO pins available for custom sensors, buttons, and peripherals
- **ROS(Robot Operating System)**: Micro ROS support is comming soon...
- **MQTT**: MQTT support is comming soon...
- **Gamepad**: Bluetooth Gamepad support is comming soon...
- **BT/WiFi switch**: Allow configuration over Bluetooth API instead of Restful HTTP API is comming soon...

## Why not Micropython on existing hardware?

- I wanted to learn something new
- Designing Hard- and Software work together is fun
- There is no truly open system with the same capabilities available

## How It Differs from Existing Solutions

### vs. LEGO&copy; Powered Up!

- **Open Source**: Complete control over firmware and functionality
- **Expandable**: Access to additional GPIO pins for custom hardware
- **Programming Flexibility**: Choose between Lua scripting or visual programming
- **Built-in Features**: Integrated IMU and LED controller without additional hubs
- **Cost-effective**: Based on affordable ESP32 hardware

### vs. fischertechnik&copy; RX Controller

- **LEGO Compatibility**: Native support for LEGO&copy; WeDo 2.0 protocol
- **Web-based Programming**: No proprietary software required
- **Cost-effective**: Based on affordable ESP32 hardware
- **Visual Programming**: Blockly support makes it accessible for all skill levels

## WebUI Screenshots

![Project Management](docs/screenshot_ide.png)

![Example Project](docs/screenshot_project.png)

## Hardware Photos

![Assembled prototype](docs/prototype_iteration1.jpg)

## Getting Started

Documentation coming soon...

