# Signal Conditioning Filters in Megahub

**A practical guide — from intuition to math to Blockly programs.**

Four filters are covered here: **Hysteresis**, **Debounce**, **Rate Limiter**, and **1D Kalman**.
They all solve the same class of problem: raw sensor and actuator signals are messy in the real
world. These blocks clean them up before they reach your control logic.

You can read any section independently. Each builds on the previous but stands on its own.

---

## Contents

1. [The Signal Conditioning Problem](#1-the-signal-conditioning-problem)
2. [Hysteresis (Schmitt Trigger)](#2-hysteresis-schmitt-trigger)
3. [Debounce](#3-debounce)
4. [Rate Limiter (Slew Rate)](#4-rate-limiter-slew-rate)
5. [1D Kalman Filter](#5-1d-kalman-filter)
6. [Combining Filters — Pipeline Patterns](#6-combining-filters--pipeline-patterns)
7. [Quick Reference](#7-quick-reference)

---

## 1. The Signal Conditioning Problem

### Why raw signals are not ready to use

Real sensor readings are never perfectly stable. A LEGO color sensor hovering over a line
edge alternates between "on line" and "off line" many times per second even when nothing is
moving. A push button generates 5–20 rapid pulses when pressed once. A PID controller may
suddenly command a motor from 0 to 100 — causing wheel slip and encoder skips.

These are not bugs in the sensors or the firmware. They are physical realities. Signal
conditioning is the practice of accepting that reality and adapting your program to it.

### The four tools and when to use each

| Filter | Problem it solves | Input → Output |
|--------|------------------|----------------|
| **Hysteresis** | A threshold triggers repeatedly when the signal hovers near it | Continuous → 0 or 1 (latching) |
| **Debounce** | A digital signal bounces multiple times on a single physical event | 0/1 → stable 0 or 1 |
| **Rate Limiter** | An output changes too abruptly, causing mechanical or control problems | Continuous → smoothly changing continuous |
| **1D Kalman** | A continuous signal is noisy; you want the best statistical estimate | Continuous → smoothed continuous |

A useful mental model: **Hysteresis** and **Debounce** fix false state transitions.
**Rate Limiter** and **Kalman** fix the magnitude of change. Reach for the right tool for
the right problem — they are not interchangeable.

---

## 2. Hysteresis (Schmitt Trigger)

### The intuition

Imagine a light switch that has a tiny crack in it. When the handle is right at the middle
position, the lights flicker on and off rapidly because the internal contact is right at the
switching threshold. A small vibration is enough to flip it repeatedly.

A hysteresis filter solves this by requiring the input to go **clearly past** the threshold
before the output changes. Instead of one threshold, there are two: a high threshold to switch
on, and a lower threshold to switch off. Between those two levels — the **dead band** — the
output stays whatever it last was.

```
output = 1
────────────────────────────────────────────── highThresh (e.g. 600)
                    ██████████████
    ░░░░░░░░░░░░░░░░            ░░░░░░░░░░░░░ ← dead band: output holds
────────────────────────────────────────────── lowThresh (e.g. 400)
                                    ████████
output = 0
```

The dead band is the key feature: a signal that oscillates inside it cannot toggle the output.

### State machine

Hysteresis has exactly two states — output is 0 or output is 1:

```
                 value > highThresh
    ┌─────────────────────────────────►  output = 1
    │                                        │
    │                            value < lowThresh
    │                                        │
output = 0  ◄─────────────-──────────────────┘
```

Everything else (the dead band) stays in the current state.

### First call initialization

On the very first call for a new handle, the output initializes to:
- `1` if `value > highThresh`
- `0` otherwise (including when `value` is inside the dead band)

This ensures the output is in a valid state immediately rather than starting at an arbitrary
default.

### Blockly usage

```
hub.init():
    set [myHyst] to [Initialize hysteresis]

hub.startthread():
    repeat forever:
        set [onLine] to [Hysteresis  myHyst  value: [lego get mode dataset PORT1 0]
                                             low: 400   high: 600]
        if [onLine = 1] then
            set motor speed PORT3: 30
        else
            set motor speed PORT3: 50
        end
```

### Tuning guide

**Choose thresholds based on sensor behavior:**

1. Read the sensor while it is clearly in the "off" state. Note the typical reading — call it `v_off`.
2. Read the sensor while it is clearly in the "on" state. Note the typical reading — call it `v_on`.
3. Set `lowThresh` somewhere between `v_off` and the midpoint.
4. Set `highThresh` somewhere between the midpoint and `v_on`.

**Rule of thumb:** make the dead band at least as wide as the typical noise amplitude of the sensor.
If the sensor oscillates ±30 counts near the threshold, the dead band should be at least 60 counts wide.

**LEGO color sensor (ambient light / reflected light):**

| Scenario | Typical v_off | Typical v_on | Suggested thresholds |
|----------|--------------|-------------|---------------------|
| Black line on white surface | 700–900 | 100–200 | low=300, high=600 |
| White line on black surface | 100–200 | 700–900 | low=400, high=650 |

These are starting points. Always measure your specific sensor on your specific surface.

**What happens if lowThresh ≥ highThresh?** The firmware logs a WARN and collapses both
thresholds to their midpoint. The filter degrades to a simple threshold with no hysteresis band.
Avoid this — check your values.

---

## 3. Debounce

### The intuition

Mechanical contacts bounce. When a button is pressed, the metal contact makes and breaks
electrical connection 5–20 times in the first 2–10 milliseconds before settling. To a computer
reading GPIO at megahertz speeds, this looks like rapid presses and releases.

A debounce filter ignores short-lived state changes. It waits until the input has been
**stable for a minimum amount of time** before updating the output. A 20 ms debounce window
filters out all bounce events because button bounce typically settles within 10 ms.

```
Raw signal:   ▔▔▁▁▔▁▔▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁
              ↑ button pressed: bounce phase
Debounced:    ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▁▁▁▁▁▁▁▁▁
                                ↑ stable for 20ms → output updates
```

The output does not change until the input has held the same value for the full `stableMs`
window. If the input bounces back before the window expires, the timer resets.

### State and timing

The debounce filter maintains:
- `lastOutput` — the committed, stable output value
- `pendingState` — the new value being timed
- `pendingStartMs` — when the pending transition began

Each call to `alg.debounce()`:
1. If the input equals `lastOutput` → nothing to do, return `lastOutput`.
2. If the input differs from `lastOutput`:
   - If the input differs from `pendingState` → a reversal: restart the timer with the new value.
   - If the input equals `pendingState` and `millis() - pendingStartMs >= stableMs` → commit: `lastOutput = pendingState`.
3. Return `lastOutput`.

Note: the debounce filter uses **wall-clock time** (`millis()`), not call count. The `stableMs`
parameter is always in real milliseconds, regardless of how often your loop runs. This makes it
predictable even if your loop rate varies.

### Blockly usage

```
hub.init():
    set [myDebounce] to [Initialize debounce]

hub.startthread():
    repeat forever:
        set [btnStable] to [Debounce  myDebounce  signal: [digital read PIN12]
                                                  stable: 20 ms]
        if [btnStable = 1] then
            -- button is pressed and has been for 20ms
        end
        wait 5 ms
```

### Tuning guide

**Choosing `stableMs`:**

| Source | Typical bounce duration | Recommended `stableMs` |
|--------|------------------------|------------------------|
| Mechanical push button | 5–15 ms | 20–50 ms |
| LEGO touch sensor | 2–10 ms | 15–30 ms |
| Reed switch | 1–5 ms | 10–20 ms |
| Capacitive touch | 0 ms (no bounce) | 5–10 ms (for electrical noise) |

**Lower `stableMs` = faster response, less filtering.** Higher `stableMs` = more filtering,
slower response. For state machines where each press advances a step, 20 ms is almost always
the right starting point.

**Loop rate interaction:** Debounce uses `millis()` so the loop rate does not affect the
`stableMs` window. However, if your loop runs at 500 ms per iteration and `stableMs` is 20 ms,
you will still see the settled value — just with an added 500 ms display lag. For button
debouncing, use a fast loop (5–20 ms) to minimise total input latency.

---

## 4. Rate Limiter (Slew Rate)

### The intuition

A PID controller may legitimately output 0 in one iteration and 100 in the next — for example,
the first time a far-off setpoint is set. Sending 0 → 100 to a motor instantly causes:
- Wheel slip (the motor accelerates faster than the wheel can grip)
- Current spikes that stress the motor driver
- Encoder jitter that poisons dead reckoning

A rate limiter adds a maximum speed of change. If the output can only move 5 units per call,
a step from 0 to 100 takes 20 calls to complete — a smooth ramp rather than a cliff.

```
Target:   ─────────────────────────────── 100
                           ╱▔▔▔▔▔▔▔
Rate-limited output:  ────╱           ← ramp at ≤5 units per call
```

### The math

On each call:

```
delta  = clamp(target − prevOutput, −maxDelta, +maxDelta)
output = prevOutput + delta
```

Where `clamp(x, lo, hi) = min(hi, max(lo, x))`.

This is all the state that is needed: a single `prevOutput` float.

**First call behavior:** On the very first call for a new handle, `target` is returned
immediately without ramping. The rate limiter does not ramp up from zero at startup — it starts
wherever the target is, then limits subsequent changes. This prevents an undesirable ramp from
zero when the first target is, say, 50.

### Loop rate dependency

**The rate limiter is call-rate dependent.** If your loop runs at 10 Hz (100 ms per iteration)
and `maxDelta = 5`, the slew rate is 5 × 10 = 50 units per second. If your loop runs at 50 Hz
(20 ms per iteration), the slew rate is 5 × 50 = 250 units per second.

The `maxDelta` parameter is **units per call**, not units per second. Choose it based on how
often your loop calls the block:

```
maxDelta = desired_slew_rate_per_second / loop_frequency_hz
```

**Example:** Motor speed in range -100 to +100. Desired ramp: 200 units/second (0→100 in 0.5s).
Loop at 20 Hz (50 ms): `maxDelta = 200 / 20 = 10`.

### Blockly usage

```
hub.init():
    set [myRL] to [Initialize rate limiter]

hub.startthread():
    repeat forever:
        set [pidOut] to [PID compute  myPID  ...]

        -- Smooth the PID output before sending to the motor
        set [motorCmd] to [Rate limit  myRL  target: pidOut  max Δ per call: 5]

        set motor speed PORT1: motorCmd
        wait 50 ms   ← 20 Hz loop; maxDelta=5 → slew rate = 100 units/sec
```

### Tuning guide

**Starting values by application:**

| Application | Loop rate | Suggested maxDelta | Resulting slew rate |
|-------------|-----------|-------------------|---------------------|
| LEGO motor soft start | 20 Hz | 5 | 100 units/sec (0→100 in 1s) |
| LEGO motor moderate ramp | 20 Hz | 10 | 200 units/sec (0→100 in 0.5s) |
| Servo / fast actuator | 50 Hz | 2 | 100 units/sec |
| Map/visualization smoothing | 10 Hz | 0.002 | 0.02 m/s (position filter) |

**If the robot feels sluggish:** increase `maxDelta`.
**If motors still jerk or encoders skip:** decrease `maxDelta`.

**What happens if maxDelta ≤ 0?** The firmware logs a WARN and uses `max(|maxDelta|, 0.001)`.
Avoid this — always pass a positive value.

---

## 5. 1D Kalman Filter

### The intuition

A moving average gives equal weight to all recent samples. If the last 10 readings include 9
good ones and 1 large spike, the spike contaminates the average as much as any other sample.

A Kalman filter is smarter: it knows *how noisy* the sensor is (measurement noise `R`) and
*how much the true value is expected to drift* (process noise `Q`). On each step, it computes
a **Kalman gain** `K` that determines how much to trust the new measurement versus the current
estimate.

- **High measurement noise (large R):** trust the estimate more, update slowly. Good for a
  jittery sensor reading a stable physical quantity.
- **High process noise (large Q):** trust new measurements more, update faster. Good when the
  true value changes quickly.

The filter automatically adjusts `K` over time based on accumulated evidence. Initially it
trusts measurements more (covariance is high); once it is confident in the estimate, it trusts
measurements less.

### The math

The 1D (scalar) Kalman filter has two steps per call:

**Predict step** — the estimate's uncertainty grows because time has passed and the true value
may have drifted:

```
P  = P + Q           ← errorCovariance grows by processNoise each call
```

**Update step** — incorporate the new measurement:

```
K  = P / (P + R)     ← Kalman gain: how much to trust the measurement
x  = x + K * (z − x) ← update estimate (z = measurement)
P  = P * (1 − K)     ← uncertainty shrinks after incorporating measurement
```

Where:
- `x` = current estimate (what the filter outputs)
- `P` = error covariance (the filter's uncertainty about `x`)
- `K` = Kalman gain (0 = ignore measurement, 1 = fully trust measurement)
- `z` = new measurement (the raw sensor reading)
- `Q` = process noise (how much `x` is expected to change per call)
- `R` = measurement noise (variance of the sensor readings)

**First call initialization:** `x = measurement`, `P = R`. The filter starts at the first reading
and assumes uncertainty equal to the measurement noise.

### What K means intuitively

`K` is always between 0 and 1. Think of it as the fraction of the correction you apply:

| K value | Meaning |
|---------|---------|
| 0.0 | Completely ignore the measurement; hold the estimate |
| 0.5 | Split the difference evenly between estimate and measurement |
| 1.0 | Fully trust the measurement; set estimate = measurement |

The filter picks K automatically based on P and R. You do not set K directly.

### Blockly usage

```
hub.init():
    set [myKF] to [Initialize Kalman filter]

hub.startthread():
    repeat forever:
        set [rawAccel] to [IMU value ACCELERATION_X]
        set [filtAccel] to [Kalman filter  myKF
                                           measurement: rawAccel
                                           process noise: 0.1
                                           measure noise: 1.0]
        -- filtAccel is now a smoothed acceleration value
        wait 50 ms
```

### Tuning guide — Q and R

**Q (process noise) — how fast does the true value change?**

| Physical situation | Suggested Q |
|--------------------|-------------|
| Static object, sensor noise only | 0.001–0.01 |
| Slowly moving object | 0.05–0.1 |
| Moderately dynamic motion | 0.5–1.0 |
| Fast-changing signal | 5–10 |

**R (measurement noise) — how noisy is the sensor?**

| Sensor / situation | Suggested R |
|--------------------|-------------|
| LEGO color sensor (static, good lighting) | 5–15 |
| LEGO distance sensor (static target) | 5–20 |
| IMU accelerometer (stationary) | 0.5–2.0 |
| IMU accelerometer (moving) | 1.0–5.0 |
| IMU gyroscope (stationary) | 0.01–0.1 |

**Starting values for common LEGO robot scenarios:**

| Scenario | Q | R |
|----------|---|---|
| Color sensor smoothing | 0.01 | 10 |
| Distance sensor smoothing | 0.05 | 20 |
| IMU acceleration filtering | 0.1 | 1.0 |

**Rule of thumb for tuning:**
- If the filter output is too slow to follow real changes → increase Q.
- If the filter output is still too noisy → increase R.
- If the filter is too slow AND too noisy → your sensor is unusually bad for this application;
  consider hardware fixes (better mounting, averaging multiple sensors).

**How Q and R relate to each other:** only the ratio Q/R matters for the steady-state behavior,
not their absolute magnitudes. `Q=0.01, R=10` and `Q=0.001, R=1` converge to the same
steady-state gain. Use absolute magnitudes to match real physical units (sensor variance in
sensor units squared, drift in units squared per call).

### Kalman vs. moving average — which to use?

| Factor | Moving average | Kalman |
|--------|--------------|--------|
| Configuration | Window size (integer) | Q and R (floats, physical meaning) |
| Startup behavior | Pre-filled on first call — no ramp | Starts at first measurement, adapts |
| Lag | Fixed lag = window size / 2 calls | Variable lag based on Q/R ratio |
| Noise rejection | Equal weight to all samples in window | Weighted by noise model |
| Spike rejection | Poor — spikes in window affect all | Better — large deviations are down-weighted |
| Computational cost | O(1), trivially cheap | O(1), slightly more arithmetic |

**Use moving average** when: you want a simple, predictable smooth with no parameters to tune
beyond window size, and the true value is approximately constant between filter calls.

**Use Kalman** when: you have physical intuition about sensor noise and process dynamics, or
when spikes from the sensor are worse than steady noise (Kalman handles this better than moving
average because large deviations reduce K temporarily).

---

## 6. Combining Filters — Pipeline Patterns

Filters compose naturally. Wire the output of one block into the input of the next.

### Pattern 1: Noisy sensor → clean threshold

Raw color sensor → Kalman → Hysteresis → motor decision

```
set [rawColor] to [lego get mode dataset PORT1 0]
set [filtColor] to [Kalman filter myKF  measurement: rawColor  Q: 0.01  R: 10]
set [onLine] to [Hysteresis myHyst  value: filtColor  low: 400  high: 600]
-- onLine is now a stable 0 or 1 based on a noise-free color reading
```

The Kalman removes the high-frequency noise; the hysteresis prevents remaining near-threshold
oscillations.

### Pattern 2: Jittery PID output → smooth motor command

PID compute → Rate limiter → motor

```
set [pidOut] to [PID compute  myPID  ...]
set [motorCmd] to [Rate limit  myRL  target: pidOut  max Δ: 5]
set motor speed PORT1: motorCmd
```

The rate limiter ensures the motor always ramps smoothly, even when the PID output jumps.

### Pattern 3: Button → debounce → state machine step

```
set [rawBtn] to [lego get mode dataset PORT2 0]
set [pressed] to [Debounce  myDB  signal: rawBtn  stable: 20 ms]
if [pressed = 1] then
    -- exactly one transition per real button press
end
```

### Pattern 4: Full sensor pipeline

```
set [rawDist] to [lego get mode dataset PORT1 0]
set [filtDist] to [Moving average  myMA  value: rawDist  window: 5]
set [mappedDist] to [Map  value: filtDist  from: 0–2500  to: 0–100]
set [pidOut] to [PID compute  myPID  setpoint: 50  pv: mappedDist  ...]
set [rampedOut] to [Rate limit  myRL  target: pidOut  max Δ: 8]
set motor speed PORT1: rampedOut
```

Raw sensor → moving average → scale → PID → rate limiter → motor. Each block solves
exactly one problem; together they create a robust control pipeline.

---

## 7. Quick Reference

### Lua API summary

| Function | Signature | Returns |
|----------|-----------|---------|
| `alg.initHysteresis()` | — | handle string |
| `alg.hysteresis(handle, value, lowThresh, highThresh)` | floats | 0.0 or 1.0 |
| `alg.clearAllHysteresis()` | — | — |
| `alg.initDebounce()` | — | handle string |
| `alg.debounce(handle, signal, stableMs)` | float, number | 0.0 or 1.0 |
| `alg.clearAllDebounce()` | — | — |
| `alg.initRateLimit()` | — | handle string |
| `alg.rateLimit(handle, target, maxDelta)` | floats | float |
| `alg.clearAllRateLimit()` | — | — |
| `alg.initKalman()` | — | handle string |
| `alg.kalman(handle, measurement, Q, R)` | floats | float |
| `alg.clearAllKalman()` | — | — |

All stateful functions use the **explicit handle pattern**: call `init*()` once to create an
instance, store the handle in a variable, pass it to the compute function on every iteration.
Handles are strings like `"hy_0"`, `"db_1"` etc. All states are cleared automatically when a
new program is executed.

### Parameter quick reference

| Filter | Key parameter | Typical range |
|--------|-------------|--------------|
| Hysteresis | dead band width (`highThresh − lowThresh`) | 10% of full sensor range |
| Debounce | `stableMs` | 20–50 ms for buttons, 5–20 ms for sensors |
| Rate limiter | `maxDelta` | (desired_slew/s) / (loop_hz) |
| Kalman | Q (process noise) | 0.001–10 (start at 0.1) |
| Kalman | R (measurement noise) | 0.1–20 (start at 1.0) |

### Common mistakes

| Symptom | Probable cause | Fix |
|---------|---------------|-----|
| Hysteresis output chatters | Dead band too narrow | Widen the gap between low and high thresholds |
| Debounce misses presses | `stableMs` longer than press duration | Reduce `stableMs` |
| Rate limiter too slow | `maxDelta` too small for the loop rate | Increase `maxDelta` or check loop rate |
| Kalman follows noise | Q too large relative to R | Decrease Q or increase R |
| Kalman too slow to react | R too large relative to Q | Decrease R or increase Q |

---

## Further Reading

- **All Blockly blocks:** [BLOCKS.md](BLOCKS.md) — visual reference with screenshots.
- **Dead reckoning guide:** [DEADRECKONING.md](DEADRECKONING.md) — encoder-based position
  tracking; moving average and Kalman are useful pre-filters for its inputs.
- **Lua API reference:** [LUAAPI.md](LUAAPI.md) — `alg.initHysteresis`, `alg.hysteresis`,
  `alg.initDebounce`, `alg.debounce`, `alg.initRateLimit`, `alg.rateLimit`,
  `alg.initKalman`, `alg.kalman`, and their `clearAll*` variants.
