# LEGO UART Message Protocol (LUMP)

This document describes the **LEGO UART Message Protocol (LUMP)** — the wire-level protocol used to communicate between a LEGO hub (or a compatible controller like Megahub) and LEGO smart devices: sensors, motors, and other peripherals from the WeDo 2.0, BOOST, SPIKE Prime, and Technic product families.

The documentation is structured from big picture to bit-level detail, so you can read as deep as you need.

---

## Table of Contents

1. [What is LUMP?](#1-what-is-lump)
2. [Physical Layer — UART](#2-physical-layer--uart)
3. [The Handshake Sequence](#3-the-handshake-sequence)
4. [Message Architecture](#4-message-architecture)
5. [The Header Byte — Anatomy](#5-the-header-byte--anatomy)
6. [Payload Size Encoding](#6-payload-size-encoding)
7. [Checksum](#7-checksum)
8. [Message Types in Detail](#8-message-types-in-detail)
   - [8.1 SYS — System Bytes](#81-sys--system-bytes)
   - [8.2 CMD — Command Messages](#82-cmd--command-messages)
   - [8.3 INFO — Mode Information Messages](#83-info--mode-information-messages)
   - [8.4 DATA — Sensor/Motor Data Messages](#84-data--sensormotor-data-messages)
9. [Data Formats](#9-data-formats)
10. [Mode System](#10-mode-system)
11. [Extended Mode — Modes 8–15](#11-extended-mode--modes-815)
12. [Keep-Alive Mechanism](#12-keep-alive-mechanism)
13. [Known Device IDs](#13-known-device-ids)
14. [Full Handshake Example](#14-full-handshake-example)
15. [Implementation: LumpParser Internals](#15-implementation-lumpparser-internals)

---

## 1. What is LUMP?

LUMP is the protocol that LEGO smart devices use to talk to the hub that powers them. When you plug a LEGO color sensor, distance sensor, or motor into a Powered Up port, the device immediately begins a self-describing handshake: it tells the hub its type, its capabilities, and the format of all the data it will send. After that, the device streams measurements continuously while the hub sends commands.

From an electronics perspective, LUMP runs over a **half-duplex single-wire UART** — the same data line is used for both directions, but normally only one side transmits at a time.

### Who uses LUMP?

| Product family | Examples |
|---|---|
| LEGO WeDo 2.0 | Tilt Sensor, Motion Sensor |
| LEGO BOOST | Color & Distance Sensor, Interactive Motor |
| LEGO SPIKE Prime | Color, Ultrasonic, Force sensors; Small/Medium/Large motors |
| LEGO Technic | XL Motor, Medium Motor, Color Light Matrix, Angular Motors |

> **Note:** LEGO MINDSTORMS EV3 sensors (Color, Ultrasonic, Gyro, Infrared) carry LUMP device type IDs (29–33) and can enumerate on LUMP-capable hubs when connected via an adapter. The EV3 hub itself, however, uses the LPF1 protocol — a separate, incompatible communication system.

---

## 2. Physical Layer — UART

| Parameter | Value |
|---|---|
| Default (handshake) baud rate | **2400 baud** |
| Operating baud rate | Negotiated — typically **9600–115200 baud** |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Connector | Single data wire (LPF2 connector pin 5/6) |

The device always starts at **2400 baud**. During the handshake it announces its preferred operating speed via a `CMD_SPEED` message. Once the hub acknowledges the handshake, both sides switch to the higher speed simultaneously.

---

## 3. The Handshake Sequence

Before any sensor data flows, device and hub go through a structured initialization dialogue. Here is the complete flow:

```
Device                          Hub (Megahub)
  |                                 |
  |--- CMD_TYPE ------------------>|   "I am device type 37"
  |--- CMD_MODES ----------------->|   "I have 10 modes"
  |--- CMD_SPEED ----------------->|   "Please switch to 115200 baud"
  |--- CMD_VERSION --------------->|   "FW 1.0.0.0, HW 1.0.0.0"
  |                                 |
  |  (for each mode 0..N-1:)        |
  |--- INFO NAME (mode X) -------->|   "Mode 0 is called COLOR"
  |--- INFO RAW  (mode X) -------->|   "Raw range: 0.0 – 10.0"
  |--- INFO PCT  (mode X) -------->|   "Pct range: 0.0 – 100.0"
  |--- INFO SI   (mode X) -------->|   "SI  range: 0.0 – 10.0"
  |--- INFO UNITS(mode X) -------->|   "Unit symbol: IDX"
  |--- INFO MAP  (mode X) -------->|   "Supports ABS input"
  |--- INFO FORMAT(mode X)------->|   "1 dataset × DATA8, 3 sig figs"
  |  (repeat for all modes)         |
  |                                 |
  |--- SYS ACK ------------------->|   "Handshake complete"
  |                                 |
  |<-- SYS ACK -------------------|   Hub acknowledges
  |                                 |
  [Both sides switch to high baud]  |
  |                                 |
  |<-- CMD SELECT mode 0 ---------|   Hub selects default mode
  |                                 |
  |--- DATA frame (mode 0)-------->|   Sensor data streaming begins
  |--- DATA frame (mode 0)-------->|
  |        ...                      |
  |<-- SYS NACK (every ~50 ms) ---|   Hub keep-alive
  |--- DATA frame (mode 0)-------->|
```

The hub must acknowledge completion (`SYS ACK`) and then send keep-alive `NACK` bytes every ~50 ms or the device will stop sending data and wait to be re-initialized.

---

## 4. Message Architecture

All LUMP messages share the same outer envelope:

```
 ┌──────────┬──────────────────────────┬───────────┐
 │  Header  │  Payload (0–32 bytes)    │ Checksum  │
 │  1 byte  │  variable length         │  1 byte   │
 └──────────┴──────────────────────────┴───────────┘
```

**Exception — System bytes:** System messages are **single byte only**. No payload, no checksum.

**Exception — INFO messages:** INFO messages insert an extra `INFO type` byte between the header and the data payload:

```
 ┌──────────┬───────────┬──────────────────────┬───────────┐
 │  Header  │ INFO type │  Data payload         │ Checksum  │
 │  1 byte  │  1 byte   │  variable length      │  1 byte   │
 └──────────┴───────────┴──────────────────────┴───────────┘
```

---

## 5. The Header Byte — Anatomy

Every non-system message starts with a header byte. The 8 bits are split into three fields:

```
  Bit:  7   6   5   4   3   2   1   0
        ├───┤   ├───────────┤   ├───┤
        MSG TYPE  SIZE ENC       CMD/MODE
        (2 bits)  (3 bits)      (3 bits)
```

| Bits | Field | Meaning |
|---|---|---|
| 7–6 | **MSG TYPE** | Message category (SYS / CMD / INFO / DATA) |
| 5–3 | **SIZE ENC** | Encoded payload size (see §6) |
| 2–0 | **CMD / MODE** | Command ID (for CMD/INFO) or mode index (for DATA) |

### MSG TYPE values

| Binary | Hex | Type |
|---|---|---|
| `00` | `0x00` | SYS — single-byte system message |
| `01` | `0x40` | CMD — command message |
| `10` | `0x80` | INFO — mode information message |
| `11` | `0xC0` | DATA — sensor/motor data |

### Quick header decode examples

| Header byte | Type | Size enc | CMD/Mode |
|---|---|---|---|
| `0x40` | CMD | 0 (1 byte) | 0 (CMD_TYPE) |
| `0x49` | CMD | 1 (2 bytes) | 1 (CMD_MODES) |
| `0x52` | CMD | 2 (4 bytes) | 2 (CMD_SPEED) |
| `0x5F` | CMD | 3 (8 bytes) | 7 (CMD_VERSION) |
| `0x98` | INFO | 3 (8 bytes) | 0 (mode 0) |
| `0xC0` | DATA | 0 (1 byte) | 0 (mode 0) |
| `0xC8` | DATA | 1 (2 bytes) | 0 (mode 0) |

---

## 6. Payload Size Encoding

Bits 5–3 of the header encode the payload length as a power of two:

| Encoding (bits 5–3) | Payload bytes |
|---|---|
| `000` (0) | 1 |
| `001` (1) | 2 |
| `010` (2) | 4 |
| `011` (3) | 8 |
| `100` (4) | 16 |
| `101` (5) | 32 |
| `110` (6) | Reserved / invalid |
| `111` (7) | Reserved / invalid |

So the total frame size (excluding system bytes) is always:
```
frame_size = 1 (header) + payload_bytes + 1 (checksum)
           [+ 1 (INFO type byte) for INFO messages]
```

---

## 7. Checksum

The checksum byte is the last byte of every multi-byte frame. It is computed as:

```
checksum = 0xFF XOR header XOR payload[0] XOR payload[1] XOR ... XOR payload[N-1]
```

For INFO messages the INFO type byte is included in the XOR sequence before the data payload bytes.

The receiver computes the same XOR over all bytes it has received (excluding the checksum itself) and compares the result to the received checksum byte. A mismatch means the frame is corrupted.

### Why start with 0xFF?

Starting the accumulator at `0xFF` means that a correctly formed single-byte payload where `payload[0] == 0x00` still produces a non-zero checksum. It is also the same convention used by the LEGO firmware itself.

---

## 8. Message Types in Detail

### 8.1 SYS — System Bytes

System messages are **single bytes** — no payload, no checksum. The message type bits (7–6) are `00` and the message itself is fully encoded in the lower 6 bits.

| Byte value | Name | Direction | Meaning |
|---|---|---|---|
| `0x00` | **SYNC** | Device → Hub | Keep-alive heartbeat during handshake |
| `0x02` | **NACK** | Hub → Device | Keep-alive: "I'm alive, keep sending data" |
| `0x04` | **ACK** | Both | Handshake acknowledgment |

**SYNC**: Sent by the device during the handshake phase to signal it is alive. The hub silently ignores SYNC bytes.

**NACK**: Sent by the hub every ~50 ms while the device is streaming data. Despite its name, this is not an error signal — it is simply the hub's keep-alive pulse. If the device stops receiving NACK bytes within ~100 ms it will halt data transmission.

**ACK**: Sent by the device at the end of its information dump to signal "I have sent all my mode descriptions." The hub responds with its own ACK. After that exchange, both sides switch baud rate and data streaming begins.

---

### 8.2 CMD — Command Messages

CMD messages (type bits = `01`, header `0x4x`) are used during initialization. The lower 3 bits of the header identify the command.

#### CMD_TYPE (0x0)

The device announces its type ID.

```
Header: 0x40  (CMD, size=1 byte, cmd=0)
Payload[0]: device type ID (uint8)
```

Example: `0x40 0x25 0x9A` — device type `0x25` = 37 (BOOST Color & Distance Sensor)

#### CMD_MODES (0x1)

The device announces how many operating modes it supports.

```
Header: 0x49 / 0x51 / 0x59  (CMD, size=2/4/8 bytes, cmd=1)
Payload[0]: number of modes - 1  (e.g. 0x09 = 10 modes)
Payload[1]: (if present) number of "view" modes - 1
Payload[2]: (if 4-byte payload) actual mode count - 1
Payload[3]: (if 4-byte payload) actual view count - 1
```

The number of modes available is always `payload[0] + 1`. The 4-byte variant appears on newer devices.

#### CMD_SPEED (0x2)

The device announces the baud rate it wants to use after the handshake.

```
Header: 0x52  (CMD, size=4 bytes, cmd=2)
Payload[0..3]: baud rate as little-endian uint32
               e.g. 0x00 0xC2 0x01 0x00 = 115200 baud
```

#### CMD_SELECT (0x3) — Hub → Device

The hub sends this to select the active mode after the handshake.

```
Header: 0x43  (CMD, size=1 byte, cmd=3)
Payload[0]: mode index to activate (0-15)
```

#### CMD_WRITE (0x4) — Hub → Device

The hub sends data to an output-capable device (e.g., setting motor speed via protocol).

```
Header: 0x46 / 0x4E / 0x56 ...  (CMD, size varies, cmd=4)
Payload: device-specific write data
```

#### CMD_EXT_MODE (0x6)

Selects the "bank" for subsequent mode operations. Modes 0–7 use bank 0, modes 8–15 use bank 8.

```
Header: 0x46  (CMD, size=1 byte, cmd=6)
Payload[0]: 0x00 (modes 0-7) or 0x08 (modes 8-15)
```

When a device sends `CMD_EXT_MODE` with `0x08` before an INFO or DATA frame, the mode index in the next frame's header should be interpreted as `mode + 8`.

#### CMD_VERSION (0x7)

The device announces its firmware and hardware versions, encoded in BCD (Binary-Coded Decimal).

```
Header: 0x5F  (CMD, size=8 bytes, cmd=7)
Payload[0..3]: firmware version (little-endian BCD, nibble-swapped per byte)
Payload[4..7]: hardware version (little-endian BCD, nibble-swapped per byte)
```

Version decoding example for FW version `0x00 0x00 0x00 0x10`:
- Read bytes 3→2→1→0 (big-endian order): `0x10, 0x00, 0x00, 0x00`
- Each byte is nibble-swapped before digit extraction: `0x10` → swap → `0x01` → digits `"01"`
- Final version string: `"01.00.00.00"`

---

### 8.3 INFO — Mode Information Messages

INFO messages (type bits = `10`, header `0x8x–0xBx`) describe the capabilities of a single mode. They are sent once per mode during the handshake.

The lower 3 bits of the header carry the **mode index** (0–7). Modes 8–15 are indicated by the `MODE_PLUS_8` flag (bit 5 = `0x20`) in the INFO type byte.

**Frame structure:**
```
[Header] [INFO type byte] [data payload...] [Checksum]
```

The INFO type byte has the following format:
```
  Bit:  7   6   5   4   3   2   1   0
                └── MODE_PLUS_8 flag   └── INFO type (0x00-0x1F)
```

If bit 5 (`0x20`) is set in the INFO byte, the effective mode index = header mode bits + 8.

#### INFO_NAME (0x00)

The human-readable name for the mode.

```
INFO byte: 0x00 (or 0x20 for modes 8-15)
Data:      null-terminated ASCII string, max 11 characters
```

Example: mode 0 of color sensor is named `"COLOR"`, mode 3 might be `"REF"`

The name must start with an uppercase letter to be accepted.

#### INFO_RAW (0x01)

The raw sensor value range.

```
INFO byte: 0x01
Data[0..3]:  float min (little-endian IEEE 754)
Data[4..7]:  float max (little-endian IEEE 754)
```

#### INFO_PCT (0x02)

The percentage-scaled range corresponding to the raw range.

```
INFO byte: 0x02
Data[0..3]:  float min (typically 0.0)
Data[4..7]:  float max (typically 100.0)
```

#### INFO_SI (0x03)

The SI-unit range (e.g., meters, degrees, Newtons).

```
INFO byte: 0x03
Data[0..3]:  float min (little-endian IEEE 754)
Data[4..7]:  float max (little-endian IEEE 754)
```

#### INFO_UNITS (0x04)

The unit symbol string for this mode.

```
INFO byte: 0x04
Data:      null-terminated ASCII string, max 4 characters
```

Examples: `"IDX"`, `"PCT"`, `"LUX"`, `"mm"`, `"DEG"`, `"N"`

#### INFO_MAPPING (0x05)

Flags describing input and output capabilities of the mode.

```
INFO byte: 0x05
Data[0]: input capability flags
Data[1]: output capability flags
```

Flag bit meanings (both input and output bytes):

| Bit | Mask | Meaning |
|---|---|---|
| 7 | `0x80` | Supports NULL value |
| 6 | `0x40` | Supports functional mapping 2.0 |
| 4 | `0x10` | ABS — absolute (discrete numbered) values |
| 3 | `0x08` | REL — relative values |
| 2 | `0x04` | DIS — discrete/enum values |

#### INFO_MODE_COMBOS (0x06)

A bitmask describing which modes can be active simultaneously (combi-mode).

```
INFO byte: 0x06
Data:      variable-length bitmask (1 bit per mode combination)
```

This is informational; combi-mode support in Megahub is a future extension.

#### INFO_FORMAT (0x80)

The most important INFO message — defines how the DATA frames for this mode will be structured.

```
INFO byte: 0x80
Data[0]:  number of datasets per frame
Data[1]:  data type ID (0x00=DATA8, 0x01=DATA16, 0x02=DATA32, 0x03=DATAFLOAT)
Data[2]:  number of significant figures for display
Data[3]:  number of decimal places for display
```

Example: a color sensor returning one 8-bit integer value with 3 significant figures and 0 decimal places:
```
datasets=1, type=DATA8 (0x00), figures=3, decimals=0
```

---

### 8.4 DATA — Sensor/Motor Data Messages

DATA messages (type bits = `11`, header `0xCx–0xFx`) carry the actual sensor readings during normal operation. They are sent by the device continuously.

```
Header: 0xC0 | (size_enc << 3) | mode_index
Payload: sensor data (layout defined by the mode's FORMAT)
```

The mode index in the header (bits 2–0) identifies which mode is reporting. If the device previously sent a `CMD_EXT_MODE` with value `0x08`, add 8 to this index.

**Example:** BOOST Color & Distance Sensor in mode 8 (SPEC_1):
- First sends `CMD_EXT_MODE` with `0x08`
- Then sends `DATA` frame with mode bits = `0` (header `0xC8`)
- Effective mode = 0 + 8 = 8

The payload bytes are decoded using the format established during handshake (see §10).

---

## 9. Data Formats

The INFO_FORMAT message defines how to interpret the raw bytes in each DATA frame.

| Format ID | Name | Size | Type | Range |
|---|---|---|---|---|
| `0x00` | **DATA8** | 1 byte | int8_t | −128 to +127 |
| `0x01` | **DATA16** | 2 bytes | int16_t LE | −32768 to +32767 |
| `0x02` | **DATA32** | 4 bytes | int32_t LE | −2,147,483,648 to +2,147,483,647 |
| `0x03` | **DATAFLOAT** | 4 bytes | float LE (IEEE 754) | ±3.4 × 10³⁸ |

All multi-byte integers and floats use **little-endian** byte order.

A DATA frame may contain **multiple datasets** — for example, a sensor that reports X, Y, Z simultaneously would have `datasets=3`. In that case the payload contains `datasets × sizeof(type)` bytes, with each value packed consecutively:

```
payload = [dataset_0_byte(s)] [dataset_1_byte(s)] [dataset_2_byte(s)] ...
```

---

## 10. Mode System

A LEGO device can support up to **16 modes** (indices 0–15). Each mode represents a different way to query the device:

- A color sensor might have one mode for raw RGB values, another for detected color index, another for ambient light intensity.
- A motor might have a position mode (degrees), a speed mode (RPM), and a power mode (percent).

**Mode lifecycle:**

1. The device announces all modes during the handshake via INFO messages.
2. The hub selects exactly one active mode using `CMD_SELECT`.
3. The device then streams DATA frames for that mode only.
4. The hub can switch modes at any time by sending another `CMD_SELECT`.

**Mode object (in Megahub implementation):**

Each mode stores:
- Name (string, ≤ 11 chars)
- Raw min/max (float pair)
- Percent min/max (float pair)
- SI min/max (float pair)
- Unit string (string, ≤ 4 chars)
- Input/output capability flags
- Format (datasets, type, figures, decimals)
- Dataset array (allocated on receipt of INFO_FORMAT)

---

## 11. Extended Mode — Modes 8–15

Devices with more than 8 modes use an extended mode mechanism. The lower 3 bits of a header can only represent modes 0–7. To address modes 8–15, the device precedes the relevant INFO or DATA frame with a `CMD_EXT_MODE` command:

```
Sequence for mode 9 INFO_NAME:
  [CMD_EXT_MODE, payload=0x08]   <- "next frame is in bank 8"
  [INFO header, mode bits=1]     <- mode 1 + 8 = mode 9
  [INFO_NAME data]
  [checksum]
```

The `extModeOffset` is consumed by exactly **one** following frame, then automatically reset to 0.

---

## 12. Keep-Alive Mechanism

LEGO devices have a built-in watchdog: if they stop receiving **NACK** bytes from the hub within approximately **100 ms**, they stop sending data and wait to be re-initialized.

Megahub's keep-alive strategy:

1. The hub sends an immediate NACK when entering data mode to start the watchdog clock.
2. Every 50 ms, the hub checks if it is at least 4 ms past the last received byte (to avoid collisions with incoming DATA frames).
3. If the wire is idle (inter-frame gap), the NACK is sent.
4. As a safety valve, NACK is sent unconditionally after 80 ms regardless of wire activity.

```
Timeline (not to scale):

  ←  DATA frame  →  ← gap → NACK  ← DATA frame  → ← gap → NACK
  |______________| 4ms      ↑     |______________|         ↑
                            hub sends NACK               hub sends NACK
```

From the device's perspective, NACK means "I acknowledge your last transmission, send more." This is why NACK, despite its literal meaning, is the normal positive keep-alive signal in this protocol.

---

## 13. Known Device IDs

Device type IDs as reported in the `CMD_TYPE` message payload:

| ID | Device |
|---|---|
| 29 | MINDSTORMS EV3 Color Sensor |
| 30 | MINDSTORMS EV3 Ultrasonic Sensor |
| 32 | MINDSTORMS EV3 Gyro Sensor |
| 33 | MINDSTORMS EV3 Infrared Sensor |
| 34 | WeDo 2.0 Tilt Sensor |
| 35 | WeDo 2.0 Motion Sensor |
| 37 | BOOST Color and Distance Sensor |
| 38 | BOOST Interactive Motor |
| 45 | Technic XL Motor |
| 46 | Technic Medium Motor |
| 48 | SPIKE Medium Motor |
| 49 | SPIKE Large Motor |
| 61 | SPIKE Color Sensor |
| 62 | SPIKE Ultrasonic Sensor |
| 63 | SPIKE Prime Force Sensor |
| 64 | Technic Color Light Matrix |
| 65 | SPIKE Small Motor |
| 75 | Technic Medium Angular Motor |
| 76 | Technic Large Angular Motor |

---

## 14. Full Handshake Example

Below is a complete annotated byte stream for a **WeDo 2.0 Motion Sensor** (device ID 35) connecting to Megahub. All bytes are hex.

### Device → Hub (2400 baud)

```
CMD_TYPE:
  40 23 9C
  │  │  └─ checksum: 0xFF ^ 0x40 ^ 0x23 = 0x9C
  │  └─── payload: device ID 0x23 = 35 (WeDo 2.0 Motion Sensor)
  └────── header: 0x40 = CMD (0x40) | size=1 (0x00) | cmd=TYPE (0x0)

CMD_MODES:
  49 01 B7
  │  │  └─ checksum: 0xFF ^ 0x49 ^ 0x01 = 0xB7
  │  └─── payload: 0x01 = modes-1, so device has 2 modes
  └────── header: 0x49 = CMD (0x40) | size=2 (0x08) | cmd=MODES (0x1)
          (size=2 but only payload[0] is relevant for 2-byte variant)

CMD_SPEED:
  52 00 C2 01 00 8A
  │  └──────────── payload: 0x0001C200 = 115200 baud (little-endian)
  └────────────── header: 0x52 = CMD (0x40) | size=4 (0x10) | cmd=SPEED (0x2)

CMD_VERSION:
  5F 00 00 10 00 00 00 10 00 A6
  │  └──────────────────────── payload: FW 0x00001000, HW 0x00001000
  └────────────────────────── header: 0x5F = CMD (0x40) | size=8 (0x18) | cmd=VERSION (0x7)

--- Mode 0 INFO ---

INFO NAME for mode 0:
  98 00 44 49 53 54 00 00 00 00 00 00 00 2D
  │  │  └────────────────────────────── "DIST\0" padded to 11 bytes
  │  └─ INFO type byte 0x00 = NAME, bit5=0 so mode is 0+0=0
  └─── header: 0x98 = INFO (0x80) | size=8 (0x18) | mode=0

INFO RAW for mode 0:
  99 01 00 00 00 00 00 00 C8 43 ...
  │  │  └─────────────── float min = 0.0, float max = 400.0 (0x43C80000)
  │  └─ INFO type byte 0x01 = RAW
  └─── header: 0x99 = INFO (0x80) | size=8 (0x18) | mode=1 (but INFO type carries mode)
       Actually header lower 3 bits = mode index in header

INFO FORMAT for mode 0 (simplified):
  92 80 01 00 03 04 00 7E
  │  │  └────────────── datasets=1, type=DATA8(0x00), figures=3, decimals=0
  │  └─ INFO type byte 0x80 = FORMAT
  └─── header

--- End of mode descriptions ---

SYS ACK:
  04
  └── single byte: ACK
```

### Hub → Device

```
SYS ACK:
  04    (hub acknowledges handshake completion)

[Both sides now switch to 115200 baud]

CMD_SELECT mode 0:
  43 00 BC
  │  │  └─ checksum: 0xFF ^ 0x43 ^ 0x00 = 0xBC
  │  └─── payload: mode index 0
  └────── header: 0x43 = CMD | size=1 | cmd=SELECT
```

### Device → Hub (115200 baud — ongoing data)

```
DATA frame, mode 0, 1 byte:
  C0 05 3A
  │  │  └─ checksum: 0xFF ^ 0xC0 ^ 0x05 = 0x3A
  │  └─── payload: 0x05 = distance reading 5
  └────── header: 0xC0 = DATA (0xC0) | size=1 (0x00) | mode=0

DATA frame, mode 0, 1 byte:
  C0 07 38
  (distance reading 7)
```

### Hub → Device (keep-alive)

```
SYS NACK:
  02    (sent every ~50 ms)
```

---

## 15. Implementation: LumpParser Internals

The Megahub implementation lives in [lib/lpfuart/](lib/lpfuart/). This section explains how the parser works at the code level.

### Component Map

```
┌─────────────────────────────────────────────────────────────┐
│                        LegoDevice                           │
│  loop() ──► parseIncomingData() ──► parser_.feedByte()      │
│  loop() ──► needsKeepAlive() ──► sendNack()                 │
│  loop() ──► finishHandshake() ──► sendAck() + baud switch   │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                    LumpParser                        │   │
│  │  feedByte() / feedBytes()                            │   │
│  │    └──► processBuffer()   ← sliding window loop      │   │
│  │           ├── SYS byte? → dispatchSystemByte()       │   │
│  │           ├── validate checksum                      │   │
│  │           └── valid frame → dispatchFrame()          │   │
│  │                 ├── CMD → parse CMD fields           │   │
│  │                 ├── INFO → parse INFO fields         │   │
│  │                 └── DATA → device_->onDataFrame()    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                             │
│  Mode[0..15]  ◄── populated by INFO messages                │
│    └── Dataset[0..N]  ◄── populated by DATA messages        │
└─────────────────────────────────────────────────────────────┘
```

### Ring Buffer and Framing

The parser maintains a **128-byte circular ring buffer**. Bytes from the UART are pushed in via `feedByte()` / `feedBytes()`. The inner loop `processBuffer()` runs a **sliding-window** algorithm:

1. **Peek at the head byte** (do not consume yet).
2. Determine the message type from bits 7–6.
3. If SYS (`0x00`): consume 1 byte immediately, dispatch.
4. Otherwise: decode the payload size from bits 5–3.
5. Check if the buffer contains a full frame (`1 + payloadSize + 1` bytes, plus 1 extra for INFO).
6. If not enough bytes yet: `break` and wait for more UART data.
7. Compute the expected checksum over all frame bytes except the last.
8. **If checksum matches**: extract the payload, advance the head past the entire frame, dispatch.
9. **If checksum mismatches**: advance the head by **1 byte only** (re-synchronization) and retry from step 1.

This byte-at-a-time sliding on mismatch allows the parser to **self-synchronize** after line noise or a missed byte without needing a dedicated framing byte or start-of-frame marker.

### Sync Loss and Recovery

If more than 300 consecutive checksum errors occur, the parser calls `device_->reset()`. This resets the LEGO device connection back to 2400 baud and restarts the handshake. The parser logs:

- Number of bytes discarded during the sync-loss episode
- A hex dump of the first 64 discarded bytes (for debugging electrical noise)

### Statistics

The `LumpParserStats` structure is available via `parser_.stats()` and is logged every 5 seconds in data mode:

| Field | Meaning |
|---|---|
| `framesOk` | Total successfully parsed and dispatched frames |
| `checksumErrors` | Total frames rejected by checksum |
| `bytesDiscarded` | Total bytes consumed during re-sync attempts |
| `syncRecoveries` | Number of times sync was lost and re-established |
| `bufferOverflows` | Times the ring buffer was full (oldest byte dropped) |
| `unknownSysBytes` | Unrecognised system byte values |
| `invalidSizeBytes` | Header bytes with reserved size encoding (6 or 7) |

### Key Files

| File | Purpose |
|---|---|
| [lib/lpfuart/src/lumpparser.cpp](lib/lpfuart/src/lumpparser.cpp) | Protocol parser — frame detection, checksum, dispatch |
| [lib/lpfuart/include/lumpparser.h](lib/lpfuart/include/lumpparser.h) | Parser class and stats struct |
| [lib/lpfuart/src/legodevice.cpp](lib/lpfuart/src/legodevice.cpp) | Device state machine, handshake, keep-alive, mode selection |
| [lib/lpfuart/include/legodevice.h](lib/lpfuart/include/legodevice.h) | Device class, device ID constants |
| [lib/lpfuart/src/mode.cpp](lib/lpfuart/src/mode.cpp) | Mode metadata storage, data packet processing |
| [lib/lpfuart/include/mode.h](lib/lpfuart/include/mode.h) | Mode class |
| [lib/lpfuart/src/dataset.cpp](lib/lpfuart/src/dataset.cpp) | Individual value parsing (DATA8/16/32/FLOAT) |
| [lib/lpfuart/include/dataset.h](lib/lpfuart/include/dataset.h) | Dataset class |
| [lib/lpfuart/src/format.cpp](lib/lpfuart/src/format.cpp) | Data format type mapping |
| [lib/lpfuart/include/format.h](lib/lpfuart/include/format.h) | Format class and FormatType enum |
| [lib/lpfuart/include/serialio.h](lib/lpfuart/include/serialio.h) | Abstract UART I/O interface |

---

*This document was generated from the Megahub firmware source code. For the authoritative protocol specification, refer to the [LEGO Powered Up documentation](https://lego.github.io/lego-ble-wireless-protocol-docs/) and community reverse-engineering resources such as [pybricks/micropython-http-client](https://github.com/pybricks).*
