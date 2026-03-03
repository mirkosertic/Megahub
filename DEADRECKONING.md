# Dead Reckoning in Megahub

**A practical guide — from intuition to math to Blockly programs.**

You can stop reading at any section and still have something useful. Each section builds on
the previous, but every section stands on its own.

---

## 1. Introduction — What is Dead Reckoning?

### The idea

Imagine you are in a dark room. You cannot see anything. But you know where you started,
and you count every step you take and every turn you make. After 10 steps forward and a
left turn, you have a pretty good idea of where you are — even without looking.

That is dead reckoning. The robot does the same thing: it counts how far each wheel has
turned, combines that with a heading from the gyroscope, and continuously computes its
(x, y) position on a 2D map.

### Why it matters for robots

- No GPS required — GPS does not work indoors, and requires network or satellite access.
- Works offline — all computation runs on the ESP32 with no external services.
- Zero extra hardware — Megahub already has wheel encoders (via LEGO POS mode) and an IMU.
- Immediate visual feedback — the live map panel shows the robot's trail as it moves.

### Honest limitations

Dead reckoning always drifts. Every measurement has a small error, and those errors add
up over time. The longer the robot runs without a correction, the less certain the
position estimate becomes.

Rule of thumb: expect roughly **2–5% position error per meter** on a smooth surface.
This means after driving 2 meters, the position could be off by 4–10 cm.

Dead reckoning is a starting point, not a solution. For longer or more precise runs,
combine it with landmark-based corrections (see Section 7) or a full SLAM system.

---

## 2. The Robot Model — Differential Drive Kinematics

### What kind of robot does this apply to?

Dead reckoning in Megahub is designed for **differential-drive robots**: two independently
driven wheels on opposite sides of a chassis, with a passive caster or ball at the front
or back for balance. Most LEGO-built wheeled robots fit this description.

### Top-down diagram

```
                    ← wheelbase d →

        ┌─────────────────────────────┐
        │                             │
  LEFT  ●  ─── ─── center ─── ─── ──  ●  RIGHT
  WHEEL │          point              │  WHEEL
        │             ▲ forward       │
        └─────────────────────────────┘
```

- The **left wheel** and **right wheel** each have an encoder that measures how far the
  wheel has rotated.
- The **center point** (midpoint between the two wheels) is the robot's tracked position.
- The **wheelbase d** is the distance between the two wheel contact points (in meters).
- The robot faces **forward** (the direction it starts in is heading = 0°).

### Qualitative explanation

- If both wheels turn the same amount → the robot goes straight.
- If the right wheel turns more than the left → the robot curves left.
- If the left wheel turns more than the right → the robot curves right.
- If both wheels turn equally in opposite directions → the robot turns in place.

The key insight: **we only need wheel distances and a heading to compute a new position.**
No camera, no GPS, no ultrasound — just encoders and a gyroscope.

---

## 3. Sensors Used

### Wheel encoders (LEGO motor POS mode)

LEGO Powered Up motors have a built-in encoder that measures the absolute angular position
of the motor shaft in **degrees**. This is called **POS mode** (mode 2).

**What it measures:** The total rotation of the motor shaft, in degrees, since the last
reset. This is an absolute value — it increases as the motor turns forward and decreases
as it turns backward.

**How to read it in Blockly:**
1. In your `hub.init()` block, call `lego select mode` with the port and mode **2** (POS).
2. In your main loop, call `lego get mode dataset` with the port and dataset **0**.
   This returns the current encoder position in degrees.

**What `mPerTick` means:** The encoder measures degrees, but we need meters of wheel
travel. `mPerTick` is the conversion factor: **meters of linear travel per degree of
encoder rotation**. This is robot-specific because it depends on wheel circumference and
gear ratios.

**Calibration procedure:**
1. Mark the robot's starting position on the floor.
2. Drive the robot forward exactly 1 meter (use a ruler or tape measure).
3. Read the encoder value (the tick delta since start).
4. Compute: `mPerTick = 1.0 / delta_ticks`

Starting guesses: `mPerTick = 0.0005` (typical for medium LEGO wheels), `wheelbase = 0.12`
(12 cm between wheels). Adjust based on calibration.

### IMU yaw

The on-board MPU6050 measures the robot's absolute orientation.

**What it measures:** Yaw is the rotation around the vertical axis — which direction the
robot is facing, in degrees, relative to its power-on orientation.

**Why IMU heading is preferred over wheel-derived heading:** Wheel slip, surface
irregularity, and encoder quantization errors compound rapidly in wheel-based heading
estimates. The IMU's gyroscope integrates rotation cleanly for short-to-medium durations.
At `imuWeight = 0.8` (default), 80% of the heading update comes from the IMU.

**IMU drift:** The IMU itself drifts over long periods (minutes), especially with
temperature changes. For runs under a few minutes, IMU drift is small. For longer runs,
landmark corrections (Section 7) become important.

**How to read it:** The `mh_imu_value(YAW)` block returns yaw in **degrees**. The DR
update block accepts this directly — no conversion needed.

---

## 4. The Math

You do not need to understand this section to use dead reckoning. But if you want to know
exactly what the firmware computes on every loop iteration, here it is.

### Step 1 — Tick delta to distance

The encoder gives absolute positions. We need the distance traveled since the last update:

```
DeltaL = (leftTicks  - prevLeftTicks)  * mPerTick   [meters]
DeltaR = (rightTicks - prevRightTicks) * mPerTick   [meters]
```

### Step 2 — Center arc distance

The center point of the robot travels the average of the two wheel distances:

```
Deltad = (DeltaL + DeltaR) / 2   [meters]
```

For equal wheel turns this is just the straight-line distance traveled. For unequal turns,
it is the arc length of the center point. This approximation works well when the loop rate
is fast relative to the robot's turning speed.

### Step 3 — Heading update (hybrid IMU + wheel)

Two ways to estimate how much the heading changed:

**From wheel speed difference:**
```
dTheta_wheels = (DeltaR - DeltaL) / wheelbase        [radians]
```

**From IMU yaw delta:**
```
DeltaYaw      = yaw_deg - prevYaw_deg                [degrees, raw delta]
// Normalize to (-180, +180] to handle IMU wrap-around:
while DeltaYaw >  180: DeltaYaw -= 360
while DeltaYaw < -180: DeltaYaw += 360
dTheta_IMU    = DeltaYaw * pi/180                    [radians, wrap-safe]
```

**Blend both sources:**
```
dTheta = (1 - imuWeight) * dTheta_wheels + imuWeight * dTheta_IMU
theta += dTheta
```

**IMU wrap-around explained:** Most IMU libraries report yaw in the range [-180°, +180°]
or [0°, 360°]. When the robot crosses the ±180° boundary (for example, turning from +179°
to -179°), the raw delta `yaw - prevYaw` jumps by approximately ±360° instead of the
tiny actual rotation. The normalization loop above collapses any delta outside (-180°,
+180°] to the equivalent short-arc rotation. This is safe for all realistic turn rates —
the robot cannot physically rotate more than 180° between two loop iterations at normal
speeds.

At `imuWeight = 0.8` (default): 80% IMU + 20% wheel odometry.
- Set to `0.0` for pure wheel-derived heading (no IMU).
- Set to `1.0` for pure IMU heading (ignores wheel speed difference).

### Step 4 — Position update

Project the center arc distance onto the X and Y axes using the current heading:

```
x' = x + Deltad * cos(theta)
y' = y + Deltad * sin(theta)
```

**Why this works geometrically:** `cos(theta)` is the forward component of unit motion
projected onto the X axis, and `sin(theta)` is the component projected onto the Y axis.
When the robot faces exactly forward (theta = 0°), `cos(0) = 1` and `sin(0) = 0`, so
all motion goes into X and none into Y — exactly right for a forward-facing robot.

---

## 5. Coordinate System and ROS Conventions

Megahub follows the [ROS REP 103](https://www.ros.org/reps/rep-0103.html) standard for
all coordinate frames. This ensures compatibility with the broader robotics ecosystem and
eliminates unit conversion when bridging to ROS-based tools.

### Coordinate axes (right-handed system)

Imagine looking down at your robot from above:

```
        +Y (left)
         |
         |
         |
         +----------> +X (forward, heading = 0 deg)
       (0,0)
```

- **+X axis** = the direction the robot faces at heading = 0° (forward)
- **+Y axis** = left of the robot (90° counterclockwise from +X)
- **+Z axis** = up (perpendicular to the ground, pointing toward the ceiling)

This is a **right-handed coordinate system**: if you point your right thumb along +Z
(up), your fingers curl from +X toward +Y — the same direction as positive rotation.

### Yaw angle convention

| Yaw value | Robot faces |
|-----------|------------|
| 0°        | +X (forward) |
| 90°       | +Y (left) |
| 180°      | -X (backward) |
| -90° or 270° | -Y (right) |

**Yaw increases counterclockwise** when viewed from above (right-hand rule about +Z).
- Turn left → yaw increases.
- Turn right → yaw decreases.

### Euler angles and which one dead reckoning uses

| Angle | Rotation about | Movement | Used in DR? |
|-------|---------------|----------|-------------|
| **Roll** | X-axis (forward) | Tilt left/right | No — assumes flat surface |
| **Pitch** | Y-axis (left) | Nose up/down | No — assumes flat surface |
| **Yaw** | **Z-axis (up)** | **Turn left/right** | **Yes — this is the heading** |

For a differential-drive robot on a flat floor, only yaw matters. The Z-axis points up;
rotating about it changes the heading — that is exactly what turning means.

### ROS frame naming (REP 105)

| DR concept | ROS frame name | Description |
|---|---|---|
| DR pose (x, y, theta) | `odom` → `base_link` | Continuous, drifts over time |
| SLAM-corrected pose (future) | `map` → `base_link` | Globally consistent |
| Robot body | `base_link` | Origin at robot center |

Because Megahub follows REP 103/105 exactly, when connecting to ROS via micro-ROS or a
BLE bridge, **no unit or axis conversion is needed**. The pose from `alg.drGet()` can
be published directly as a ROS `nav_msgs/Odometry` message (after converting heading
from degrees to radians, which is done internally by the firmware).

### Units reference

All units are SI (International System of Units), per ROS REP 103:

| Value | Unit | Notes |
|-------|------|-------|
| Position X, Y | **meters** | Not centimeters or inches |
| Heading (Blockly API) | **degrees** | Converted to/from radians inside firmware |
| Heading (internal storage) | **radians** | ROS REP 103 compliant |
| Wheelbase | **meters** | Center-to-center wheel distance |
| mPerTick | **meters / encoder degree** | Robot-specific calibration constant |

---

## 6. Error Accumulation and Uncertainty

### Why dead reckoning always drifts

Every measurement contains a tiny error. When you add up thousands of measurements over
a long run, those errors accumulate. This is fundamental — it cannot be eliminated, only
reduced.

The two main error sources:

1. **Wheel slip** (random, hard to model): If a wheel skids on a slippery surface, the
   encoder still counts rotation but the robot has not actually traveled that distance.
   This introduces a random position error on every step.

2. **IMU drift** (systematic over time): The gyroscope integrates angular rate to estimate
   yaw. Integration amplifies low-frequency bias — after several minutes, the accumulated
   heading error can be noticeable. Temperature changes make this worse.

### Rule of thumb

On a smooth floor, with reasonable calibration:
- **2–5% position error per meter traveled.**
- After 1 m: position could be off by 2–5 cm.
- After 3 m: position could be off by 6–15 cm.

### What this means in practice

Dead reckoning is reliable for **short maneuvers (under 2–3 meters)**. For longer runs,
the error grows to the point where the position estimate is no longer useful without
corrections.

The uncertainty grows with every step. There is no internal "confidence score" that
decreases back toward zero — each new measurement adds more uncertainty.

**When to use it:**
- Short navigation sequences (go forward 0.5 m, turn left, go forward 0.5 m).
- Detecting drift and triggering corrections at known landmarks.
- Teaching and visualization — even imperfect DR produces useful, intuitive feedback.

---

## 7. Correcting Drift — Landmark-based Position Fixes

### The concept

When your robot reaches a location you know precisely (a colored mat, a wall, a charging
dock, a starting line), you can inject the known pose into the DR system. The DR algorithm
then continues from the correct position, and accumulated drift is discarded.

This is called **landmark-based localization** — conceptually simple and surprisingly
effective in structured environments like a classroom track with colored tape.

### The `Set DR pose` block

The `Set DR pose of [handle]` block resets x, y, and heading to the values you provide.
On the next `DR update` call, deltas are computed from the current encoder values, so
there is no discontinuity in the motion estimate.

### Example Blockly pattern

```
if [color sensor = RED] then
    Set DR pose of [myRobot]   x: 0.30   y: 0.0   heading: 0.0
end
```

This says: "when the robot detects the red marker on the floor, I know I am at position
(0.30, 0.0) facing forward — correct the estimate to that."

After the correction, the robot continues navigating from the known position. Any drift
accumulated before the landmark is discarded.

### Practical tips

- Place landmarks at known positions along the route (colored tape, physical stops,
  touch sensor triggers).
- Use `Reset DR pose` (resets to origin) at the beginning of each run to start fresh.
- Use `Set DR pose` (inject known values) mid-run to correct at checkpoints.

---

## 8. Blockly Usage Guide

### Prerequisites

Before using the DR blocks, you must select POS mode on the motor ports in `hub.init()`:

```
hub.init():
    lego select mode   port: PORT1   mode: 2   ← POS mode, reads absolute encoder degrees
    lego select mode   port: PORT2   mode: 2
    wait 500 ms                                 ← give LEGO sensors time to initialise
```

Without `lego select mode ... 2`, the encoder dataset will return 0 or incorrect values.

> **Note:** The IMU is initialised automatically during the Megahub startup sequence before
> any Lua program runs. There is no need to wait for the IMU in `hub.init()`.

### Block reference

#### `Init DR` (`mh_alg_dr_init`)

Creates a new dead reckoning instance and returns a unique handle. Call this once before your main loop to initialize the DR system.

**No inputs.** Returns a handle string (e.g., "dr_0", "dr_1", ...).

**Usage pattern:**
```
set [myRobot] to [Init DR]
```

Store the handle in a variable and use it with all subsequent DR operations.

#### `DR update` (`mh_alg_dr_update`)

Updates the dead reckoning pose with new sensor readings. Call this once per loop iteration.

**7 inputs:**

| Input | What to connect | Starting value |
|-------|----------------|----------------|
| handle | variable with DR handle | `myRobot` |
| left ticks | `lego get mode dataset PORT1 0` | — |
| right ticks | `lego get mode dataset PORT2 0` | — |
| yaw (deg) | `IMU value YAW` | — |
| m/tick | constant number | `0.0005` |
| wheelbase (m) | constant number | `0.12` |
| IMU weight | constant number | `0.8` |

**Usage pattern:**
```
DR update  handle: myRobot  left:... right:... yaw:... ...
```

This is a statement block (does not return a value). It updates the internal state of the DR instance identified by the handle.

#### `DR pose X/Y/HEADING of` (`mh_alg_dr_get`)

Reads one component of the pose from a handle.

```
[DR pose  X       of  myRobot]   → meters (float)
[DR pose  Y       of  myRobot]   → meters (float)
[DR pose  HEADING of  myRobot]   → degrees (float)
```

Use these as inputs to `Map: plot position` or `Print to Logger`.

#### `Reset DR pose of` (`mh_alg_dr_reset`)

Resets x, y, and heading to 0. Also re-bootstraps the previous encoder values so the
next `DR update` produces a zero delta rather than a large jump.

Call this after `hub.init()` completes and before the main loop starts.

#### `Set DR pose of` (`mh_alg_dr_set_pose`)

Injects known x, y, heading values into the DR state. Units: meters for x and y,
degrees for heading. Use at landmarks to correct accumulated drift.

#### `Map: plot position` (`ui_map_update`)

Sends an (x, y, heading) pose to the live map panel in the IDE sidebar.

```
Map: plot position   x: [DR pose X of myRobot]   y: [DR pose Y of myRobot]   heading: [DR pose HEADING of myRobot]
```

The block takes raw numbers — it is not tied to the DR system and can visualize any
(x, y, heading) source. The firmware rate-limits map updates to 5 Hz internally; you
can call this block at any rate in your loop.

#### `Map: clear` (`ui_map_clear`)

Clears the trail from the map panel. Call this at the start of each run (alongside
`Reset DR pose`) to start with a clean map.

### Complete wiring example

```
hub.init():
    lego select mode   PORT1   mode: 2
    lego select mode   PORT2   mode: 2
    wait 500 ms

hub.startthread():
    set [myRobot] to [Init DR]    ← create DR instance, get handle
    Reset DR pose of [myRobot]    ← reset pose before run starts
    Map: clear                    ← clear map trail

    repeat forever:
        DR update
            handle:       myRobot
            left ticks:   lego get mode dataset PORT1 0
            right ticks:  lego get mode dataset PORT2 0
            yaw (deg):    IMU value YAW
            m/tick:       0.0005
            wheelbase:    0.12
            IMU weight:   0.8

        Map: plot position
            x:       DR pose X of myRobot
            y:       DR pose Y of myRobot
            heading: DR pose HEADING of myRobot

        wait 50 ms    ← ~20 Hz update rate
```

### Calibrating mPerTick

1. Place the robot at a known starting point.
2. Set all encoder values to 0 by rebooting or running `hub.init()`.
3. Drive the robot forward exactly **1.0 meter** (measure with a ruler).
4. Stop the robot and read the encoder value from PORT1 using `Print to Logger`.
5. Compute: `mPerTick = 1.0 / delta_ticks`

For example, if the encoder reads 2400 degrees after 1 meter: `mPerTick = 1.0 / 2400 = 0.000417`.

**Calibrating wheelbase:**
1. Drive the robot in a full 360° circle (program it to turn in place).
2. Read the final heading from `DR pose HEADING`.
3. If heading is not back at 0° (or 360°), adjust `wheelbase` proportionally.
   - Final heading too high (e.g., 380°)? Increase `wheelbase` slightly.
   - Final heading too low (e.g., 340°)? Decrease `wheelbase` slightly.

---

## 9. Advanced: Configuring IMU Axis Mapping

### Why this section exists

The standard Megahub board mounts the MPU6050 with its axes already aligned to the
ROS coordinate frame: chip +X points forward, chip +Y points left, chip +Z points up.
For this board, no axis remapping is needed and `mh_imu_value(YAW)` returns the correct
value without any configuration.

If you have a custom board where the IMU is mounted in a different orientation (for
example, rotated 90°, or with the chip upside down), the raw IMU readings won't match
the expected coordinate convention. The firmware provides a configurable axis mapping
to correct this.

### How axis mapping works

Before any IMU value reaches Lua, the firmware applies a 3×3 rotation matrix that
transforms the physical chip axes into the logical ROS frame. For the standard board,
this matrix is the identity (no transformation). For a rotated board, you configure a
different matrix.

### Configuring a custom axis mapping

1. Place your board flat on a table with the robot facing forward.
2. Identify which physical IMU axis (X, Y, or Z as labeled on the chip) points:
   - **Forward** → this maps to logical +X
   - **Left** → this maps to logical +Y
   - **Up** → this maps to logical +Z
   - Note the sign (positive or negative) for each.

3. Open or create `lib/imu/include/imu_config.h` and add a board variant:

```cpp
#pragma once

#ifdef BOARD_VARIANT_MY_CUSTOM_BOARD
    // Example: IMU rotated 90 degrees clockwise when viewed from top
    // Physical: chip +X points right, chip +Y points forward, chip +Z points up
    const float IMU_AXIS_MAP[3][3] = {
        {0,  1,  0},   // logical X (forward) = physical Y
        {-1, 0,  0},   // logical Y (left) = -physical X (right inverted)
        {0,  0,  1}    // logical Z (up) = physical Z
    };
#else
    // Default: identity mapping (standard Megahub board)
    const float IMU_AXIS_MAP[3][3] = {
        {1,  0,  0},
        {0,  1,  0},
        {0,  0,  1}
    };
#endif
```

**How to read the matrix:** Each row is a logical axis. Each column is a physical axis
(X, Y, Z of the chip). A value of `1` means "take this physical axis as-is." A value of
`-1` means "take this physical axis and flip the sign." A value of `0` means "ignore
this physical axis for this logical axis."

Row 1 is logical X (forward). Row 2 is logical Y (left). Row 3 is logical Z (up).

4. Define `BOARD_VARIANT_MY_CUSTOM_BOARD` in `platformio.ini`:

```ini
build_flags =
    -DBOARD_VARIANT_MY_CUSTOM_BOARD
```

### Verifying the mapping

After configuring:
1. Place the robot on a flat surface facing forward.
2. Open the IMU readout in the IDE and confirm:
   - Yaw = 0° when robot faces forward (the direction you defined as +X).
   - Yaw increases when you rotate the robot counterclockwise (to the left).
   - Roll and pitch are near 0° when the robot is flat.
3. If yaw increases when turning right instead of left, negate the Z axis contribution
   in the matrix (row 3, or the sign on the input for the yaw axis).

---

## Further Reading

- **Lua API reference:** [LUAAPI.md](LUAAPI.md) — `alg.initDR`, `alg.updateDR`, `alg.drGet`,
  `alg.drReset`, `alg.drSetPose`, `alg.clearAllDR`, `ui.mappoint`, `ui.mapclear`.
- **All Blockly blocks:** [BLOCKS.md](BLOCKS.md) — visual reference with screenshots.
- **ROS REP 103:** https://www.ros.org/reps/rep-0103.html — SI units and axis conventions.
- **ROS REP 105:** https://www.ros.org/reps/rep-0105.html — coordinate frame naming for
  mobile platforms.
