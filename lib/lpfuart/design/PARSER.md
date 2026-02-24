# LUMP Protocol Parser — Design and Implementation Plan

## 1. Context and Motivation

The current implementation uses a polymorphic state machine (`WaitingState`, `ParseCommandState`,
`ParseInfoState`, `ParseDataState`) to process the incoming LEGO UART Message Protocol (LUMP) byte
stream. While conceptually clean, this design has a fundamental resilience problem: if the byte stream
becomes misaligned — even by a single byte — the machine interprets payload bytes as header bytes and
enters a cascading failure mode. The only recovery mechanism is a 200 ms silence timeout that forces a
full device re-handshake.

This document specifies a **streaming sliding-window parser** to replace the state machine. The new
parser is self-synchronising, has no heap allocation in the critical path, and exposes explicit
observability counters for error diagnosis.

---

## 2. LUMP Protocol Reference

Derived from `PYBRICKS-Lego Powered Up UART Protocol.pdf` and `PHILOHOME-Lego Powered Up Serial Link
Protocol.pdf`. The sections below are the minimum required for correct parsing.

### 2.1 Frame Format

Every multi-byte message has the structure:

```
[ HEADER (1 byte) ] [ PAYLOAD (n bytes) ] [ CHECKSUM (1 byte) ]
```

**Header byte bit fields:**

| Bits | Field        | Values                                                   |
|------|--------------|----------------------------------------------------------|
| 7–6  | Message type | 00=SYS, 01=CMD, 10=INFO, 11=DATA                         |
| 5–3  | Payload size | 000→1, 001→2, 010→4, 011→8, 100→16, 101→32 bytes        |
| 2–0  | Command/mode | Meaning depends on message type                          |

**Checksum:** XOR of `0xFF` with every byte in the frame excluding the checksum itself:

```
checksum = 0xFF ^ header ^ payload[0] ^ payload[1] ^ ... ^ payload[n-1]
```

### 2.2 System Messages

System messages are **single-byte only** and carry **no checksum**. They are identified by bits 7–6
being `00` (i.e., byte value < `0x40`):

| Byte   | Name       | Value | Direction      | Meaning                                         |
|--------|------------|-------|----------------|-------------------------------------------------|
| `0x00` | BYTE_SYNC  | 0x00  | device → hub   | Synchronisation marker (sent with BYTE_ACK)     |
| `0x02` | BYTE_NACK  | 0x02  | hub → device   | Keep-alive (send every ≤ 100 ms in data mode)   |
| `0x04` | BYTE_ACK   | 0x04  | both           | Acknowledge (end of handshake / fast-baud reply)|

**Important:** All three system bytes have bits 7–6 = `00`. Any other byte with bits 7–6 = `00`
is unknown and should be silently discarded by the parser. No other defined message type produces a
header byte with bits 7–6 = `00`.

### 2.3 Command Messages (CMD, type = 0x40)

Sent device → hub during handshake. Exceptions: `CMD_SELECT` and `CMD_WRITE` are sent hub → device.

| bits 2–0 | Name         | Payload description                                         |
|----------|--------------|-------------------------------------------------------------|
| 0x00     | CMD_TYPE     | 1 byte: device type ID                                      |
| 0x01     | CMD_MODES    | 1, 2, or 4 bytes: mode counts (EV3: 1–2 bytes, LPF2: 4)   |
| 0x02     | CMD_SPEED    | 4 bytes: target baud rate as little-endian uint32           |
| 0x03     | CMD_SELECT   | 1 byte: mode index (hub → device)                           |
| 0x04     | CMD_WRITE    | n bytes: device-specific payload (hub → device)             |
| 0x06     | CMD_EXT_MODE | 1 byte: EXT_MODE_0 (0x00) or EXT_MODE_8 (0x08)             |
| 0x07     | CMD_VERSION  | 8 bytes: fw-version (4 bytes BCD LE) + hw-version (4 bytes) |

### 2.4 Info Messages (INFO, type = 0x80)

Sent device → hub during handshake, once per mode. The **second payload byte** carries the info
type and optionally the `INFO_MODE_PLUS_8` flag (`0x20`):

| payload[0] masked | Name            | Notes                                    |
|-------------------|-----------------|------------------------------------------|
| 0x00              | INFO_NAME       | Null-padded ASCII string, max 11 chars   |
| 0x01              | INFO_RAW        | 8 bytes: float min + float max           |
| 0x02              | INFO_PCT        | 8 bytes: float min + float max           |
| 0x03              | INFO_SI         | 8 bytes: float min + float max           |
| 0x04              | INFO_UNITS      | Null-padded ASCII string, max 4 chars    |
| 0x05              | INFO_MAPPING    | 2 bytes: input flags + output flags      |
| 0x06              | INFO_MODE_COMBOS| Only for mode 0; 16-bit combo bitmasks   |
| 0x80              | INFO_FORMAT     | 4 bytes: datasets, format, figures, dec  |

**Mode extension:** If `payload[0] & 0x20` is set, the actual mode = (header bits 2–0) + 8. This flag
is in the payload, not the header. It does **not** affect payload size calculation.

### 2.5 Data Messages (DATA, type = 0xC0)

Sent device → hub continuously after handshake. Payload size and format are defined by the
previously received `INFO_FORMAT` message for the active mode.

### 2.6 Extended Mode Data (Modes 8–15)

For modes ≥ 8, devices use a two-message sequence to work around the 3-bit mode field limit:

```
[CMD_EXT_MODE frame: header=0x46, payload=0x08, checksum]   ← CMD | LENGTH_1 | CMD_EXT_MODE
[DATA frame:        header=0xC0|size|mode, payload, checksum]   ← mode = actual_mode - 8
```

The `CMD_EXT_MODE` payload byte carries the offset (0x00 for modes 0–7, 0x08 for modes 8–15).
The parser must track this offset and apply it when dispatching the subsequent DATA frame.

This pattern appears for **both incoming data from device** (e.g. Boost Color/Distance Sensor in
mode 8 = SPEC_1) and **outgoing writes from hub** (e.g. setting motor power on mode > 7).

### 2.7 Combi-Mode Data (Technic/SPIKE Devices)

Devices that declare `INFO_MODE_COMBOS` can operate in a mode combination where multiple modes are
active simultaneously. In this state the device sends **multiple DATA frames in rapid succession**,
one per active mode, after each keep-alive NACK. Each DATA frame contains the data for one mode,
identified by its mode field.

For Technic motors with modes ≥ 8 in a combi-set, the `CMD_EXT_MODE` prefix is also emitted per
frame (or once per group — observed behaviour varies). The data handler must be prepared to receive
frames for modes it did not explicitly select, and must not discard them.

---

## 3. Root Cause of Current Synchronisation Failures

The debug log in `parsedatastate.cpp` shows the concrete failure mode. Example sequence:

```
// Motor running backwards: actual frame is D2 E6 FF FF FF 34
// D2 = 0xC0|0x10|0x02 = DATA | SIZE_4 | MODE_2  → 4 payload bytes + checksum

Received correctly:  D2  E6 FF FF FF  34   ← valid
                     ^hdr ^---payload---^  ^chk

After one dropped byte, next header is misread as 0xFF (payload byte):
0xFF = 0b11111111 = DATA | SIZE_32 | MODE_7  → 32 payload bytes expected
Parser now waits for 33 more bytes, consuming an entire subsequent frame as payload.
Checksum fails → returns to WaitingState, but head is now N bytes into stream.
The next read header is another payload byte → cascades.
```

The 200 ms timeout is the only escape: it triggers `reset()`, which drops the baud rate to 2400 and
re-initiates the full handshake. This is slow and causes observable data gaps.

An additional bug in the current implementation: `parsedatastate.cpp` checks
`(messageType & LUMP_INFO_MODE_PLUS_8) > 0` on the DATA header byte to detect mode extension. This
is incorrect — `INFO_MODE_PLUS_8` (0x20) lives in the second **payload** byte of INFO messages, not
in any DATA header field. In a DATA header, bit 5 is part of the size encoding. For a DATA frame
with LENGTH_16 (`0xE0 | mode`), this check fires incorrectly.

---

## 4. Sliding Window Parser Design

### 4.1 Core Concept

Maintain a fixed-size circular ring buffer. On each byte received, push it into the buffer. Then
repeatedly attempt to consume a valid frame from the head of the buffer. If the candidate frame at
the current head fails checksum validation, discard exactly **one byte** and retry. This guarantees
re-alignment within at most `MAX_FRAME_SIZE` (34) bytes after any corruption.

```
Byte stream  →  [  Ring Buffer  ]  →  Frame dispatcher  →  LegoDevice
                      ↑ slide by 1 on bad checksum
```

### 4.2 Frame Validity Test

For a candidate frame starting at position `head`:

```
header   = buf[head]
type     = header & 0xC0
size_enc = (header >> 3) & 0x07
mode     = header & 0x07

// System byte? (type == 0x00)
if (type == 0x00):
    if (header in {0x00, 0x02, 0x04}): dispatch 1-byte system message, advance 1
    else: discard 1 byte (unknown system byte)
    continue

// Payload size
payload_bytes = decode_size(size_enc)   // 1,2,4,8,16,32 or INVALID
if (payload_bytes == INVALID): discard 1 byte, continue

frame_size = 1 + payload_bytes + 1     // header + payload + checksum
if (available < frame_size): wait for more bytes

// Checksum
expected = 0xFF
for i in [0 .. frame_size-2]:
    expected ^= buf[head + i]

if (expected == buf[head + frame_size - 1]):
    dispatch frame, advance head by frame_size    // SUCCESS
else:
    stats.checksumErrors++
    stats.bytesDiscarded++
    advance head by 1                            // SLIDE
```

### 4.3 Ring Buffer Specification

| Parameter          | Value  | Rationale                                               |
|--------------------|--------|---------------------------------------------------------|
| `RING_BUF_SIZE`    | 128    | ≥ 2 × max frame (34 bytes) with slack for read bursts   |
| Allocation         | Static | Member array in `LumpParser`; zero heap allocation       |
| Overflow policy    | Discard oldest byte, increment `stats.bufferOverflows`  |

128 bytes uses 128 bytes of stack/BSS — negligible on the ESP32. The SC16IS752 UART chip has a 64-byte
RX FIFO, so the ring buffer provides additional buffering if `loop()` is temporarily delayed.

### 4.4 Parser State (Minimal)

The sliding window parser itself is stateless across frames, **except** for one piece of semantic
state: the `extModeOffset` (0 or 8), set by a received `CMD_EXT_MODE` frame and consumed by the
next DATA frame:

```cpp
struct LumpParserState {
    uint8_t  buf[RING_BUF_SIZE];
    uint16_t head;          // read index (mod RING_BUF_SIZE)
    uint16_t tail;          // write index (mod RING_BUF_SIZE)
    uint16_t count;         // bytes currently in buffer
    uint8_t  extModeOffset; // 0 or 8; set by CMD_EXT_MODE, cleared after DATA dispatch
};
```

No heap allocation. No virtual calls in the hot path.

### 4.5 Observability Counters

All counters are `uint32_t` and never reset autonomously (caller can snapshot/diff them):

```cpp
struct LumpParserStats {
    uint32_t framesOk;          // successfully dispatched frames
    uint32_t checksumErrors;    // frames rejected by checksum
    uint32_t bytesDiscarded;    // bytes consumed during re-sync
    uint32_t syncRecoveries;    // transitions: bad streak → first good frame
    uint32_t bufferOverflows;   // bytes dropped because ring buffer was full
    uint32_t unknownSysBytes;   // unrecognised system bytes (type==0, not SYNC/ACK/NACK)
    uint32_t invalidSizeBytes;  // header bytes with reserved size encoding
};
```

`syncRecoveries` is incremented when `framesOk` increases after a non-zero `checksumErrors` run.
This is the most useful single metric for detecting intermittent noise on the UART line.

These counters should be logged periodically (e.g. every 5 s in data mode) via the `INFO` macro to
provide passive telemetry without flooding the log.

---

## 5. Device-Specific Data Handling

### 5.1 Problem

Different devices interpret DATA frame payloads differently. The Boost Color/Distance Sensor in
combi-mode 8 (SPEC_1) returns 8 bytes encoding 5 distinct sensor readings. Control+ motors return
a 10-byte frame with speed, accumulated angle, absolute angle, and padding. Standard mode parsing
through `Mode::processDataPacket()` handles well-behaved single-dataset frames but cannot cover
all device-specific interpretations.

### 5.2 Design

Introduce a virtual method `onDataFrame()` on `LegoDevice`. The default implementation uses the
existing `Mode::processDataPacket()` path. Device-specific subclasses (or device-specific handler
registrations) can override it for non-standard payloads.

```cpp
// In legodevice.h
class LegoDevice {
public:
    // Called by LumpParser for every validated DATA frame.
    // mode = resolved mode index (0-15, after applying extModeOffset).
    // payload / payloadSize = validated payload bytes (not including header or checksum).
    // Default implementation looks up Mode* and calls mode->processDataPacket().
    virtual void onDataFrame(int mode, const uint8_t* payload, int payloadSize);

    // Called by LumpParser for combi-mode DATA bursts.
    // Invoked once per individual DATA frame within a combi group.
    // Default: delegates to onDataFrame().
    virtual void onCombiDataFrame(int mode, const uint8_t* payload, int payloadSize);
};
```

This pattern keeps the parsing logic in `LumpParser` and the semantic interpretation in `LegoDevice`
subclasses, without coupling them through a switch statement. New device support requires only a new
subclass and no changes to the parser.

**Example: Boost Color/Distance Sensor override sketch:**

```cpp
class BoostColorDistanceSensor : public LegoDevice {
protected:
    void onDataFrame(int mode, const uint8_t* payload, int payloadSize) override {
        if (mode == 8) {
            // SPEC_1 combi payload: color(1), proximity(1), count(2), reflected(1), ambient(1), rgb(3)
            parseSpec1(payload, payloadSize);
        } else {
            LegoDevice::onDataFrame(mode, payload, payloadSize);
        }
    }
};
```

---

## 6. Class Design

### 6.1 `LumpParser`

```
lib/lpfuart/include/lumpparser.h
lib/lpfuart/src/lumpparser.cpp
```

**Responsibilities:**
- Accumulate incoming bytes into the ring buffer
- Scan for valid frames using the sliding window algorithm
- Dispatch validated frames to `LegoDevice` via callbacks/methods
- Maintain `LumpParserStats`

**Interface:**

```cpp
class LumpParser {
public:
    static constexpr int RING_BUF_SIZE = 128;

    explicit LumpParser(LegoDevice* device);

    // Feed one byte from the UART into the parser.
    // May dispatch zero or more frames as a side effect.
    void feedByte(uint8_t byte);

    // Feed a buffer of bytes (convenience wrapper).
    void feedBytes(const uint8_t* data, int len);

    // Retrieve current statistics snapshot.
    const LumpParserStats& stats() const;

    // Reset statistics counters (e.g. after logging them).
    void resetStats();

private:
    static int decodePayloadSize(uint8_t header);   // returns -1 if invalid
    static bool isKnownSystemByte(uint8_t b);

    void processBuffer();   // inner loop: try to consume frames from head
    void dispatchFrame(uint8_t header, const uint8_t* payload, int payloadSize);
    void dispatchSystemByte(uint8_t b);

    uint8_t         buf_[RING_BUF_SIZE];
    uint16_t        head_;
    uint16_t        count_;
    uint8_t         extModeOffset_;   // 0 or 8
    LumpParserStats stats_;
    LegoDevice*     device_;
};
```

**Key invariant:** `count_` ≤ `RING_BUF_SIZE`. The `buf_` array is indexed as
`buf_[(head_ + i) % RING_BUF_SIZE]` for offset `i`.

### 6.2 `LegoDevice` changes

- Remove `ProtocolState* protocolState_` member.
- Add `LumpParser parser_` member (value, not pointer — no heap allocation).
- `parseIncomingData()` becomes: `while (serialIO_->available()) { parser_.feedByte(serialIO_->readByte()); }`
- Add `virtual void onDataFrame(int mode, const uint8_t* payload, int payloadSize)`.
- Add `virtual void onCombiDataFrame(int mode, const uint8_t* payload, int payloadSize)`.
- Keep `onCmdFrame()` and `onInfoFrame()` as non-virtual internal methods (CMD and INFO parsing
  is protocol-level, not device-specific).
- Keep `Mode`, `Dataset`, `Format` classes unchanged.

### 6.3 Files to Remove

Once the new parser is in place and validated:

| File                          | Reason                                   |
|-------------------------------|------------------------------------------|
| `include/protocolstate.h`     | Replaced by `lumpparser.h`               |
| `include/waitingstate.h`      | Replaced                                 |
| `src/waitingstate.cpp`        | Replaced                                 |
| `include/parsecommandstate.h` | Replaced                                 |
| `src/parsecommandstate.cpp`   | Replaced                                 |
| `include/parseinfostate.h`    | Replaced                                 |
| `src/parseinfostate.cpp`      | Replaced                                 |
| `include/parsedatastate.h`    | Replaced                                 |
| `src/parsedatastate.cpp`      | Replaced                                 |

Keep: `mode.h/.cpp`, `dataset.h/.cpp`, `format.h/.cpp`, `serialio.h`, `legodevice.h/.cpp`.

---

## 7. Frame Dispatch Logic

`dispatchFrame()` routes validated frames as follows. All routing is based on header bits 7–6:

```
type = header & 0xC0

switch (type):
    case 0x40 (CMD):
        cmd = header & 0x07
        switch (cmd):
            CMD_TYPE     → device_->setDeviceIdAndName(...)
            CMD_MODES    → device_->initNumberOfModes(...)
            CMD_SPEED    → device_->setSerialSpeed(...)
            CMD_VERSION  → device_->setVersions(...)
            CMD_EXT_MODE → extModeOffset_ = payload[0]   // 0x00 or 0x08
            CMD_SELECT   → (hub-sent, ignore if received)
            CMD_WRITE    → (hub-sent, ignore if received)
            default      → WARN(...)

    case 0x80 (INFO):
        mode = (header & 0x07)
        infoType = payload[0] & ~0x20          // strip INFO_MODE_PLUS_8 flag
        if (payload[0] & 0x20): mode += 8
        switch (infoType):
            INFO_NAME        → getMode(mode)->setName(...)
            INFO_RAW         → getMode(mode)->setRaw(...)
            INFO_PCT         → getMode(mode)->setPct(...)
            INFO_SI          → getMode(mode)->setSi(...)
            INFO_UNITS       → getMode(mode)->setUnits(...)
            INFO_MAPPING     → getMode(mode)->setMapping(...)
            INFO_MODE_COMBOS → device_->setModeCombos(...)
            INFO_FORMAT      → getMode(mode)->setFormat(...)
            default          → DEBUG(unknown info type)

    case 0xC0 (DATA):
        mode = (header & 0x07) + extModeOffset_
        extModeOffset_ = 0                     // consume the offset
        device_->onDataFrame(mode, payload, payloadSize)

    case 0x00 (SYS):
        // Handled before dispatchFrame() is called; should not reach here.
```

---

## 8. Handshake and ACK Handling

The ACK byte (`0x04`) is a system message processed in `dispatchSystemByte()`:

```
BYTE_ACK received:
    if (!device_->isHandshakeComplete() && device_->fullyInitialized()):
        device_->markAsHandshakeComplete()
```

This matches the current `WaitingState` behaviour. SYNC (`0x00`) is silently ignored. NACK (`0x02`)
received from a device is logged at DEBUG level (unexpected in normal operation).

---

## 9. ESP32-Specific Constraints

| Constraint        | Mitigation in this design                                               |
|-------------------|-------------------------------------------------------------------------|
| No heap in loop   | Ring buffer is a static array member; `feedByte()` has zero allocations |
| Stack depth       | `processBuffer()` is iterative, not recursive; stack usage < 64 bytes   |
| UART read limit   | Keep the 32-bytes-per-loop limit in `parseIncomingData()` to bound      |
|                   | latency. `RING_BUF_SIZE = 128` absorbs any burst within a loop cycle.   |
| Log volume        | Stats are logged periodically, not per-frame. `WARN` only on first      |
|                   | error in a sync-loss streak (tracked via a `inSyncLoss` flag).         |
| String operations | Frame dispatch uses fixed-size `uint8_t` arrays, no `String` objects    |

---

## 10. Observability Strategy

### 10.1 Per-Frame Error Suppression

To avoid log flooding during a sync-loss event (where many consecutive frames may fail checksum),
introduce a `bool inSyncLoss_` flag:

```
on checksum error:
    if (!inSyncLoss_):
        WARN("LUMP sync lost after %lu good frames", stats_.framesOk)
        inSyncLoss_ = true
    stats_.checksumErrors++

on successful frame:
    if (inSyncLoss_):
        INFO("LUMP sync recovered after discarding %lu bytes", stats_.bytesDiscarded_at_loss_start)
        stats_.syncRecoveries++
        inSyncLoss_ = false
```

### 10.2 Periodic Stats Log

`LegoDevice::loop()` should call a helper every 5 s in data mode:

```cpp
void LegoDevice::logParserStats() {
    const auto& s = parser_.stats();
    INFO("Parser: ok=%lu csErr=%lu discarded=%lu recoveries=%lu overflow=%lu",
         s.framesOk, s.checksumErrors, s.bytesDiscarded,
         s.syncRecoveries, s.bufferOverflows);
}
```

### 10.3 Reset/Recovery Threshold

If `stats_.checksumErrors` increments more than `SYNC_LOSS_RESET_THRESHOLD` (suggested: 50) without
a successful frame, the parser should signal `LegoDevice` to perform `reset()`. This replaces the
200 ms silence timer as the primary recovery mechanism, while keeping the silence timer as a secondary
fallback for physical disconnection:

```
on checksum error:
    if (stats_.consecutiveErrors > SYNC_LOSS_RESET_THRESHOLD):
        WARN("Too many consecutive checksum errors, triggering device reset")
        device_->reset()
        return
```

`consecutiveErrors` is reset to 0 on each successful frame dispatch.

---

## 11. Implementation Plan

### Phase 1 — Scaffolding (no behaviour change)

1. Create `lib/lpfuart/include/lumpparser.h` with the `LumpParserStats` struct and `LumpParser`
   class declaration.
2. Create `lib/lpfuart/src/lumpparser.cpp` with stub implementations that log every call.
3. Add `LumpParser parser_` member to `LegoDevice` (but do not yet use it).
4. Verify the project compiles cleanly.

### Phase 2 — Ring Buffer and System Byte Handling

5. Implement `feedByte()`, `feedBytes()`, ring buffer push with overflow detection.
6. Implement `processBuffer()` for system bytes only (SYNC, ACK, NACK).
7. Wire `parseIncomingData()` to call `parser_.feedByte()` alongside the existing state machine
   (dual-path, for comparison).
8. Verify ACK handling still works during device handshake.

### Phase 3 — Full Frame Scanning

9. Implement `decodePayloadSize()`.
10. Implement the sliding window frame validation loop in `processBuffer()`.
11. Implement `dispatchFrame()` for CMD and INFO messages, porting logic from
    `ParseCommandState` and `ParseInfoState`.
12. Implement `onDataFrame()` default in `LegoDevice` using `Mode::processDataPacket()`.
13. Implement `extModeOffset_` tracking for CMD_EXT_MODE / DATA extended mode sequence.
14. Remove the dual-path: let `parseIncomingData()` use only the new parser.

### Phase 4 — Device-Specific Overrides

15. Add `virtual void onDataFrame(int, const uint8_t*, int)` to `LegoDevice`.
16. Add `virtual void onCombiDataFrame(int, const uint8_t*, int)` to `LegoDevice`.
17. Move the Boost Color/Distance Sensor special-case from `ParseDataState` into an override.
18. Add the Control+ / Spike Prime motor override stub (to be filled with device-specific parsing).

### Phase 5 — Cleanup and Observability

19. Remove the old state machine files (see Section 6.3).
20. Implement `logParserStats()` and wire it into `LegoDevice::loop()`.
21. Implement the `SYNC_LOSS_RESET_THRESHOLD` early-reset path.
22. Remove leftover `Serial.printf` debug calls from `parsedatastate.cpp`.

---

## 12. Test Plan

Tests live in `/test/test_lumpparser/` and use the Unity framework (PlatformIO built-in).
All tests are host-native (no hardware required): a stub `LegoDevice` records dispatched frames.

### 12.1 Unit Tests for `LumpParser`

| Test ID | Description                                                      | Input                            | Expected output                        |
|---------|------------------------------------------------------------------|----------------------------------|----------------------------------------|
| LP-01   | System byte SYNC dispatched                                      | `{0x00}`                         | `onSyncReceived()` called once         |
| LP-02   | System byte ACK dispatched                                       | `{0x04}`                         | `onAckReceived()` called once          |
| LP-03   | System byte NACK dispatched                                      | `{0x02}`                         | `onNackReceived()` called once         |
| LP-04   | Valid CMD_TYPE frame                                             | `{0x40, 0x25, 0x9a}`             | `setDeviceIdAndName(37, ...)` called   |
| LP-05   | Valid CMD_SPEED frame                                            | `{0x52, 0x00, 0xc2, 0x01, 0x00, 0x6e}` | `setSerialSpeed(115200)` called  |
| LP-06   | Valid INFO_FORMAT frame (from spec example)                      | `{0x92, 0x80, 0x01, 0x02, 0x04, 0x00, 0x30}` | `getMode(2)->setFormat(...)` |
| LP-07   | Valid DATA frame mode 0                                          | `{0xC0, 0x00, 0x3f}`             | `onDataFrame(0, {0x00}, 1)` called     |
| LP-08   | Single byte corruption before valid frame                        | `{0xFF, 0x40, 0x25, 0x9a}`       | LP-04 result, 1 byte discarded         |
| LP-09   | Multiple byte corruption before valid frame                      | `{0xFF, 0xAA, 0xBB, 0x40, 0x25, 0x9a}` | LP-04 result, 3 bytes discarded   |
| LP-10   | Two valid frames back-to-back                                    | LP-04 bytes + LP-07 bytes        | Both dispatched in order               |
| LP-11   | Corrupt frame followed immediately by valid frame                | bad-checksum frame + valid frame | bad frame: 0 dispatch + n discarded; valid frame dispatched |
| LP-12   | INFO_MODE_PLUS_8 flag: mode resolved correctly                   | `{0x98, 0x20, ...}` (INFO_NAME, mode_0+8) | `getMode(8)->setName(...)` called |
| LP-13   | CMD_EXT_MODE sets offset; next DATA frame uses offset            | `{0x46, 0x08, chk}` + `{0xC2, data, chk}` | `onDataFrame(10, ...)` called |
| LP-14   | extModeOffset reset after DATA frame                             | EXT_MODE + DATA + DATA           | First DATA uses offset; second DATA uses 0 |
| LP-15   | Ring buffer does not overflow on 128 bytes of garbage            | 128 × `0xFF`                     | `stats.bufferOverflows == 0`; `stats.bytesDiscarded == 128` |
| LP-16   | Stats: framesOk increments on each valid frame                   | 3 valid frames                   | `stats.framesOk == 3`                  |
| LP-17   | Stats: checksumErrors increments on bad frames                   | 2 bad + 1 good frame             | `stats.checksumErrors == 2`; `stats.syncRecoveries == 1` |
| LP-18   | Unknown system byte discarded, not dispatched                    | `{0x06}`                         | No dispatch; `stats.unknownSysBytes == 1` |
| LP-19   | Invalid size encoding byte discarded                             | `{0xC8}` (DATA|SIZE_INVALID)     | `stats.invalidSizeBytes == 1`; 1 byte discarded |

> Note: SIZE encoding `0x30` (bits 5-3 = 110) and `0x38` (bits 5-3 = 111) are reserved/invalid in the
> current spec. Any header byte with these size bits should be treated as an invalid header and discarded.

### 12.2 Integration Tests (requires hardware or replay)

| Test ID | Description                                              |
|---------|----------------------------------------------------------|
| IT-01   | Full handshake with Boost Color/Distance Sensor          |
| IT-02   | Data stream in mode 8 (SPEC_1) with EXT_MODE prefix      |
| IT-03   | Motor position data stream, continuous rotation          |
| IT-04   | Deliberate UART cable disconnect and reconnect           |
| IT-05   | Control+ motor in combi-mode: multiple modes per NACK    |

---

## 13. Protocol Edge Cases and Caveats

| Edge Case                               | Handling                                                          |
|-----------------------------------------|-------------------------------------------------------------------|
| `0xFF` after `BYTE_SYNC` (EV3 IR)       | `0xFF` has `type=0xC0`, `size=0x38` → reserved size → discard.   |
| `INFO_FORMAT` with `0x80` in payload[0] | Identified by type (0x80 INFO), not by confusion with size field. |
| INFO message with unknown info type     | Log at DEBUG, do not dispatch, do not discard frame.              |
| DATA frame for unselected mode          | Pass to `onDataFrame()`; let device decide. Log at DEBUG.         |
| CMD_WRITE received from device          | Not expected; log WARN and ignore payload.                        |
| Baud rate change mid-stream             | `reset()` clears ring buffer; baud change then happens cleanly.   |
| Back-to-back EXT_MODE messages          | Second overrides first; offset set to last value before next DATA.|
| Frame payload of all-0x00              | Valid if checksum matches; not special-cased.                     |

---

## 14. Files to Create / Modify

```
lib/lpfuart/
├── design/
│   └── PARSER.md                  ← this document
├── include/
│   ├── lumpparser.h               ← NEW: LumpParser, LumpParserStats
│   ├── legodevice.h               ← MODIFY: add parser_, virtual callbacks, remove protocolState_
│   ├── mode.h                     ← unchanged
│   ├── dataset.h                  ← unchanged
│   ├── format.h                   ← unchanged
│   └── serialio.h                 ← unchanged
└── src/
    ├── lumpparser.cpp             ← NEW
    ├── legodevice.cpp             ← MODIFY: parseIncomingData(), logParserStats()
    ├── mode.cpp                   ← unchanged
    ├── dataset.cpp                ← unchanged
    └── format.cpp                 ← unchanged

test/
└── test_lumpparser/
    └── test_main.cpp              ← NEW: Unity tests for LP-01 through LP-19
```
