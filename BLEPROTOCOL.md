# Megahub BLE Protocol Reference

This document describes the Bluetooth Low Energy (BLE) protocol used between the Megahub web IDE and the ESP32 firmware. It is useful for anyone building an alternative client (mobile app, Python script, etc.) or for understanding how the web client communicates with the device.

The WiFi/HTTP API is documented separately in [HUBAPI.md](HUBAPI.md).

---

## Table of Contents

- [GATT Profile](#gatt-profile)
- [Protocol Layers](#protocol-layers)
- [Layer 1: Fragment Protocol](#layer-1-fragment-protocol)
- [Layer 2: Control Channel](#layer-2-control-channel)
- [Layer 3: Application — Requests and Responses](#layer-3-application--requests-and-responses)
- [Layer 3: Application — Events](#layer-3-application--events)
- [Layer 3: Application — File Streaming](#layer-3-application--file-streaming)
- [Connection Sequence](#connection-sequence)
- [Request/Response Reference](#requestresponse-reference)
- [Event Reference](#event-reference)

---

## GATT Profile

The ESP32 exposes a single primary GATT service with four characteristics.

### Service

| Field | Value |
|-------|-------|
| UUID | `4fafc201-1fb5-459e-8fcc-c5c9c331914b` |

### Characteristics

| Name | UUID | Properties | Description |
|------|------|-----------|-------------|
| Request | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | Write with response | Client → Device: requests and streaming data |
| Response | `1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e` | Notify / Indicate | Device → Client: request responses |
| Event | `d8de624e-140f-4a22-8594-e2216b84a5f2` | Notify / Indicate | Device → Client: unsolicited events |
| Control | `f78ebbff-c8b7-4107-93de-889a6a06d408` | Notify / Indicate + Write with response | Bidirectional: MTU negotiation, ACK/NACK, stream flow control |

Enable notifications on Response, Event, and Control characteristics before sending any requests.

---

## Protocol Layers

Communication is structured in three layers:

```
┌──────────────────────────────────────────────────────────┐
│  Layer 3: Application  (request types, event types, JSON) │
├──────────────────────────────────────────────────────────┤
│  Layer 2: Control channel  (MTU, ACK/NACK, stream ACK)    │
├──────────────────────────────────────────────────────────┤
│  Layer 1: Fragment protocol  (fragmentation + reassembly) │
└──────────────────────────────────────────────────────────┘
```

---

## Layer 1: Fragment Protocol

BLE has a small ATT MTU (negotiated, typically 247 bytes, minimum 23 bytes). Large messages are split into fragments and reassembled by the receiver.

### Fragment header (5 bytes)

Every BLE write on the Request characteristic and every notification on Response and Event characteristics carries a 5-byte header followed by payload bytes.

```
Byte 0:    Protocol Message Type
Byte 1:    Message ID (0–255, wraps)
Bytes 2–3: Fragment number (big-endian uint16, 0-based)
Byte 4:    Flags
```

### Protocol Message Types

| Value | Name | Direction | Description |
|-------|------|-----------|-------------|
| `0x01` | REQUEST | Client → Device | Application request |
| `0x02` | RESPONSE | Device → Client | Application response |
| `0x03` | EVENT | Device → Client | Unsolicited event |
| `0x04` | CONTROL | Both | Control messages (sent on Control characteristic, no fragment header) |
| `0x05` | STREAM_DATA | Client → Device | Streaming file chunk |
| `0x06` | STREAM_START | Client → Device | Start a streaming upload |
| `0x07` | STREAM_END | Client → Device | End a streaming upload |

### Flags

| Bit | Meaning |
|-----|---------|
| `0x01` | `FLAG_LAST_FRAGMENT` — this is the final fragment of the message |
| `0x02` | `FLAG_ERROR` — error flag (reserved, not currently used) |

### Fragmentation rules

- The client splits outgoing data at `MTU - 5` bytes per fragment.
- The device splits outgoing notifications the same way.
- Each fragment is written with `writeValueWithResponse` (ATT Write Request), so BLE flow control prevents buffer overruns.
- The receiver buffers fragments keyed by `(channel, messageId)` until `FLAG_LAST_FRAGMENT` is set, then reassembles and delivers the complete message.

---

## Layer 2: Control Channel

The Control characteristic is used for out-of-band signalling. Control messages do **not** use the 5-byte fragment header — they are short, fixed-format binary messages.

### Control Message Types

| Value | Name | Direction | Description |
|-------|------|-----------|-------------|
| `0x01` | ACK | Device → Client | Acknowledges a message ID |
| `0x02` | NACK | Device → Client | Rejects a message ID |
| `0x03` | RETRY | Device → Client | Requests client to retry a message |
| `0x04` | BUFFER_FULL | Device → Client | Device buffer full, request rejected |
| `0x05` | RESET | Device → Client | Fragment buffer reset requested |
| `0x06` | MTU_INFO | Device → Client | Reports negotiated MTU to client |
| `0x07` | REQUEST_MTU_INFO | Client → Device | Request current MTU info |
| `0x07` | STREAM_ACK | Device → Client | Acknowledges a stream chunk (same byte value, different direction) |
| `0x09` | STREAM_ERROR | Device → Client | Reports a stream error |

### Control message formats

**ACK / NACK / RETRY / BUFFER_FULL** (Device → Client):
```
Byte 0: Control message type
Byte 1: Message ID being acknowledged/rejected
```

**RESET** (Device → Client):
```
Byte 0: 0x05
```
Client should discard all in-progress fragment buffers on receipt.

**MTU_INFO** (Device → Client):
```
Byte 0: 0x06
Byte 1: reserved (0x00)
Bytes 2–3: MTU value (big-endian uint16)
```
The device sends `MTU_INFO` automatically after the client enables notifications. The client uses this MTU to size its fragments. Usable payload per fragment = MTU − 5 bytes.

**REQUEST_MTU_INFO** (Client → Device):
```
Byte 0: 0x07
Byte 1: 0x00
```
The device responds with another `MTU_INFO` message on the Control characteristic.

**STREAM_ACK** (Device → Client):
```
Byte 0: 0x07
Byte 1: Stream ID
Bytes 2–3: Chunk index (little-endian uint16)
           0xFFFF = STREAM_START acknowledged
           0xFFFE = STREAM_END acknowledged (upload complete)
           Other  = data chunk index acknowledged
```

**STREAM_ERROR** (Device → Client):
```
Byte 0: 0x09
Byte 1: Stream ID
Byte 2: Error code (see stream error codes below)
```

Stream error codes:

| Code | Meaning |
|------|---------|
| `0x01` | Invalid header |
| `0x02` | Invalid metadata |
| `0x03` | Mutex error |
| `0x04` | Too many concurrent streams |
| `0x05` | Cannot create file |
| `0x06` | Unknown stream |
| `0x07` | SD write failed |
| `0x08` | Timeout |

---

## Layer 3: Application — Requests and Responses

All application-level requests are sent on the **Request** characteristic using the fragment protocol. Responses arrive on the **Response** characteristic.

### Request frame format

```
[1 byte: Application Request Type][N bytes: JSON payload (UTF-8)]
```

The JSON payload is UTF-8 encoded. The client encodes it as a string and prepends the 1-byte request type.

### Response frame format

After reassembly, the response data is the raw JSON body (no type byte prefix — the message ID in the fragment header correlates response to request).

### Message ID correlation

Each `sendRequest()` call increments a local message counter (0–255, wrapping). The message ID in the fragment header of the response notification matches the message ID of the original request, allowing the client to match async responses to waiting promises.

---

## Layer 3: Application — Events

Unsolicited events arrive on the **Event** characteristic. After fragment reassembly:

### Event frame format

```
[1 byte: Application Event Type][N bytes: JSON payload (UTF-8)]
```

The first byte identifies the event type. The rest is the JSON body.

---

## Layer 3: Application — File Streaming

Large file uploads (saving project workspace files) use a dedicated streaming protocol over the **Request** characteristic rather than the standard request/response pattern, because a Lua workspace file can easily exceed the fragment reassembly buffer.

### Stream Start (Client → Device)

Written directly on the Request characteristic (no fragment header):

```
Byte 0:    0x06 (STREAM_START)
Byte 1:    Stream ID (0–255)
Bytes 2–5: Total file size (little-endian uint32)
Bytes 6–7: Total chunk count (little-endian uint16)
Byte 8:    Flags: 0x01 = overwrite existing file
Bytes 9…:  Metadata JSON (UTF-8)
           e.g. {"project":"MyProject","filename":"model.xml"}
```

Wait for `STREAM_ACK` with chunk index `0xFFFF` before sending data.

### Stream Data (Client → Device)

Written directly on the Request characteristic (no fragment header), one BLE write per chunk:

```
Byte 0:    0x05 (STREAM_DATA)
Byte 1:    Stream ID
Bytes 2–3: Chunk index (little-endian uint16, 0-based)
Byte 4:    Flags: 0x01 = last chunk, 0x00 = more chunks follow
Bytes 5…:  Chunk payload (up to MTU − 5 bytes)
```

The client uses a sliding window of 8 outstanding chunks. It waits for `STREAM_ACK` for the oldest unacknowledged chunk before advancing the window.

### Stream End (Client → Device)

Written directly on the Request characteristic after all data chunks are acknowledged:

```
Byte 0:    0x07 (STREAM_END)
Byte 1:    Stream ID
Bytes 2–5: Total file size (little-endian uint32, used as checksum)
```

Wait for `STREAM_ACK` with chunk index `0xFFFE` to confirm the upload is complete and the file has been written to the SD card.

---

## Connection Sequence

```
1.  Client calls navigator.bluetooth.requestDevice()  → user selects device
2.  Client connects to GATT server
3.  Client discovers primary service (SERVICE_UUID)
4.  Client gets all 4 characteristics
5.  Client registers characteristicvaluechanged handlers for Response, Event, Control
6.  Client calls startNotifications() on Response, Event, Control
7.  Wait 200 ms for CCCD writes to propagate through BLE stack
8.  Device sends MTU_INFO on Control characteristic
9.  Client records MTU, computes usable payload size (MTU − 5)
10. Client sends REQUEST_MTU_INFO to verify control channel works
11. Client sends READY_FOR_EVENTS request (type 0x0A, body "{}")
12. Device begins streaming log messages, port status, and BT device lists
13. Client navigates to project list view
```

On **reconnect** (after unexpected disconnect), steps 1 is skipped — the existing `BluetoothDevice` object is reused. Steps 2–13 repeat identically.

---

## Request/Response Reference

### Application Request Types

| Value | Name | Description |
|-------|------|-------------|
| `0x01` | STOP_PROGRAM | Stop the running Lua program |
| `0x02` | GET_PROJECT_FILE | Fetch a file from a project |
| `0x03` | PUT_PROJECT_FILE | (reserved — file upload uses streaming protocol) |
| `0x04` | DELETE_PROJECT | Delete a project |
| `0x05` | SYNTAX_CHECK | Check Lua syntax without executing |
| `0x06` | RUN_PROGRAM | Upload and execute a Lua program |
| `0x07` | GET_PROJECTS | List all projects on the SD card |
| `0x08` | GET_AUTOSTART | Get the current autostart project name |
| `0x09` | PUT_AUTOSTART | Set the autostart project |
| `0x0A` | READY_FOR_EVENTS | Signal that the client is ready to receive events |
| `0x0B` | REQUEST_PAIRING | Initiate Bluetooth Classic pairing |
| `0x0C` | REMOVE_PAIRING | Remove a Bluetooth Classic pairing |
| `0x0D` | START_DISCOVERY | Start Bluetooth Classic device discovery |

---

### `0x01` — STOP_PROGRAM

Stop the currently running Lua program and all its threads. LEGO device ports are reinitialized.

**Request body:**
```json
{}
```

**Response body:**
```json
{ "result": true }
```

---

### `0x02` — GET_PROJECT_FILE

Fetch the contents of a file from a project on the SD card.

**Request body:**
```json
{ "project": "MyProject", "filename": "model.xml" }
```

**Response body:** raw file content (not JSON) as UTF-8 bytes

---

### `0x04` — DELETE_PROJECT

Delete a project directory and all its files from the SD card.

**Request body:**
```json
{ "project": "MyProject" }
```

**Response body:**
```json
{}
```

---

### `0x05` — SYNTAX_CHECK

Parse and syntax-check a Lua program without executing it.

**Request body:**
```json
{ "luaScript": "-- lua code here\nprint('hello')" }
```

**Response body (success):**
```json
{ "success": true, "parseTime": 42, "errorMessage": "" }
```

**Response body (failure):**
```json
{ "success": false, "parseTime": 0, "errorMessage": "[string]:2: '=' expected near 'end'" }
```

`parseTime` is in milliseconds.

---

### `0x06` — RUN_PROGRAM

Upload and execute a Lua program. Any currently running program is stopped first.

**Request body:**
```json
{ "luaScript": "print('hello')\nwait(1000)" }
```

**Response body:**
```json
{ "result": true }
```

---

### `0x07` — GET_PROJECTS

List all projects stored on the SD card.

**Request body:**
```json
{}
```

**Response body:**
```json
{
  "projects": [
    { "name": "MyProject" },
    { "name": "TestScript" }
  ]
}
```

---

### `0x08` — GET_AUTOSTART

Get the name of the project currently configured as autostart.

**Request body:**
```json
{}
```

**Response body (project set):**
```json
{ "project": "MyProject" }
```

**Response body (no autostart set):**
```json
{}
```

---

### `0x09` — PUT_AUTOSTART

Set or clear the autostart project. The name is persisted to `/autostart.json` on the SD card.

**Request body:**
```json
{ "project": "MyProject" }
```

**Response body:**
```json
{}
```

---

### `0x0A` — READY_FOR_EVENTS

Signal to the device that the client is ready to receive event notifications. The device starts streaming log messages, port status, and Bluetooth device lists after receiving this.

**Request body:**
```json
{}
```

**Response body:**
```json
{}
```

---

### `0x0B` — REQUEST_PAIRING

Initiate Bluetooth Classic pairing with a device by MAC address.

**Request body:**
```json
{ "mac": "AA:BB:CC:DD:EE:FF" }
```

**Response body:**
```json
{}
```

---

### `0x0C` — REMOVE_PAIRING

Remove a Bluetooth Classic pairing.

**Request body:**
```json
{ "mac": "AA:BB:CC:DD:EE:FF" }
```

**Response body:**
```json
{}
```

---

### `0x0D` — START_DISCOVERY

Start a Bluetooth Classic device discovery scan. Results are delivered via the `BTCLASSICDEVICES` event.

**Request body:**
```json
{}
```

**Response body:**
```json
{}
```

---

## Event Reference

### Application Event Types

| Value | Name | Description |
|-------|------|-------------|
| `0x01` | LOG | Log message from Lua `print()` or firmware |
| `0x02` | PORTSTATUS | LEGO port connection status update |
| `0x03` | COMMAND | UI command from Lua `ui.showvalue()` or profiling data |
| `0x04` | BTCLASSICDEVICES | Bluetooth Classic device list update |

---

### `0x01` — LOG

A log message, typically from Lua `print()` or the firmware's INFO/WARN/DEBUG logging macros.

**Event body (JSON):**
```json
{ "message": "Motor speed set to 80" }
```

---

### `0x02` — PORTSTATUS

Sent whenever the LEGO port connection state changes (device connected, disconnected, or mode negotiation completed).

**Event body (JSON):**
```json
{
  "ports": [
    { "id": 1, "connected": false },
    { "id": 2, "connected": false },
    {
      "id": 3,
      "connected": true,
      "device": {
        "type": "BOOST Color and Distance Sensor",
        "icon": "⚙️",
        "modes": [
          { "id": 0, "name": "COLOR", "units": "IDX", "datasets": 1, "figures": 3, "decimals": 0, "type": "DATA8" },
          { "id": 1, "name": "PROX",  "units": "DIS", "datasets": 1, "figures": 3, "decimals": 0, "type": "DATA8" },
          { "id": 6, "name": "RGB I", "units": "RAW", "datasets": 3, "figures": 5, "decimals": 0, "type": "DATA16" }
        ]
      }
    },
    { "id": 4, "connected": false }
  ]
}
```

`datasets` is the number of separate values returned by `lego.getmodedataset()` for this mode. `type` indicates the underlying data format (`DATA8`, `DATA16`, `DATA32`, `DATAFLOAT`).

---

### `0x03` — COMMAND

Carries either a UI display value (from Lua `ui.showvalue()`) or a thread profiling report.

**`show_value` command:**
```json
{
  "type": "show_value",
  "label": "Motor speed",
  "format": "simple",
  "value": 80
}
```

The `value` field can be a number, boolean, or string. The `format` field is `"simple"` when `FORMAT_SIMPLE` is used.

**`thread_statistics` command** (sent every 10 seconds when profiling is enabled for a thread):
```json
{
  "type": "thread_statistics",
  "blockid": "block_motor_loop",
  "min": 1240,
  "avg": 1850.4,
  "max": 3100
}
```

All timing values are in microseconds. The IDE uses these to render a profiling overlay on the corresponding Blockly block.

---

### `0x04` — BTCLASSICDEVICES

Sent during and after a Bluetooth Classic discovery scan, and whenever the paired device list changes.

**Event body (JSON):** array of device objects

```json
[
  {
    "mac": "AA:BB:CC:DD:EE:FF",
    "name": "8BitDo M30 GamePad",
    "type": "Gamepad",
    "rssi": -62,
    "paired": true
  },
  {
    "mac": "11:22:33:44:55:66",
    "name": "Unknown Device",
    "type": "Unknown",
    "rssi": -78,
    "paired": false
  }
]
```
