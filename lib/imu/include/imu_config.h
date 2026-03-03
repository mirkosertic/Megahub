#pragma once

#include <array>

// IMU Axis Remapping Configuration
//
// Transforms physical IMU sensor axes → logical ROS REP 103 coordinate frame
// Applied in imu.cpp before any values are returned to Lua
//
// ROS REP 103 standard (right-handed coordinate system):
//   +X = forward (robot heading 0°)
//   +Y = left
//   +Z = up
//   Yaw increases counterclockwise when viewed from above

#ifdef BOARD_VARIANT_V2_ROTATED
// Example: IMU rotated 90° clockwise when viewed from top
// Physical axes: chip's +X points right, chip's +Y points forward
static constexpr std::array<std::array<float, 3>, 3> imuAxisMap = {
    {
     {{0, 1, 0}},  // logical X (forward) = physical Y
        {{-1, 0, 0}}, // logical Y (left) = -physical X
        {{0, 0, 1}}   // logical Z (up) = physical Z
    }
};
#else
// Default: MPU6050 mounted with +X forward, +Y left (standard orientation)
// This is the Megahub default and matches ROS REP 103 exactly
static constexpr std::array<std::array<float, 3>, 3> imuAxisMap = {
    {
     {{1, 0, 0}}, // logical X = physical X (forward)
        {{0, 1, 0}}, // logical Y = physical Y (left)
        {{0, 0, 1}}  // logical Z = physical Z (up)
    }
};
#endif
