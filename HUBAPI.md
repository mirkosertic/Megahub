# Megahub Web Server API

This document describes the HTTP REST endpoints and Server-Sent Events (SSE) provided by the Megahub web server. The web server is active only in **WiFi mode**. For BLE communication, see [BLEPROTOCOL.md](BLEPROTOCOL.md).

## Base URL

The server listens on port 80 and is reachable at:
- `http://<device-ip>/`
- `http://<device-uid>.local/` (via mDNS — works on macOS, Linux, and Windows 10+)

## Table of Contents

- [Static Resources](#static-resources)
- [Project Management](#project-management)
- [Code Execution](#code-execution)
- [Configuration](#configuration)
- [Device Discovery](#device-discovery)
- [Server-Sent Events (SSE)](#server-sent-events-sse)
- [Error Handling](#error-handling)

---

## Static Resources

All static assets are served gzip-compressed from firmware flash.

### GET /

Serves the main application HTML page.

**Response:**
- `Content-Type: text/html`
- `Content-Encoding: gzip`

### GET /index.js

Serves the main application JavaScript bundle.

**Response:**
- `Content-Type: text/javascript`
- `Content-Encoding: gzip`

### GET /style.css

Serves the application stylesheet.

**Response:**
- `Content-Type: text/css`
- `Content-Encoding: gzip`

---

## Project Management

### GET /projects

List all projects stored on the SD card.

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

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

### PUT /project/{projectId}/{filename}

Upload a file to a project. Creates the project directory if it does not exist. Overwrites any existing file with the same name.

**URL parameters:**
- `projectId` — URL-encoded project name
- `filename` — file name (e.g. `model.xml`)

**Request body:** raw file content (any content type)

**Response:**
- Status: `201 Created`
- `Content-Length: 0`
- `Cache-Control: no-cache, must-revalidate`

**Note:** This is the only file transfer direction supported over HTTP. Reading project files back from the device is not available via HTTP — use BLE (`GET_PROJECT_FILE`, request type `0x02`) instead.

---

### DELETE /project/{projectId}

Remove a project from the device configuration. The project's files on the SD card are **not** deleted — only the configuration entry is removed.

**URL parameters:**
- `projectId` — URL-encoded project name

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

**Response body:**
```json
{}
```

---

## Code Execution

### PUT /syntaxcheck

Check the syntax of a Lua program without executing it. The code is written to a temporary file (`/~syntaxcheck.lua`) on the SD card, then parsed.

**Request body:** Lua source code (plain text)

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

**Response body (success):**
```json
{
  "success": true,
  "parseTime": 42,
  "errorMessage": ""
}
```

**Response body (failure):**
```json
{
  "success": false,
  "parseTime": 0,
  "errorMessage": "[string]:2: '=' expected near 'end'"
}
```

`parseTime` is in milliseconds.

---

### PUT /execute

Upload and execute a Lua program. The code is written to a temporary file (`/~execute.lua`) on the SD card before execution. Any currently running program is stopped first.

**Request body:** Lua source code (plain text)

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

**Response body:**
```json
{ "success": true }
```

`success` is `false` if the temporary file could not be written to the SD card.

---

### PUT /stop

Stop the currently running Lua program and all its threads. LEGO device ports are reinitialized.

**Request body:** (empty)

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

**Response body:**
```json
{ "success": true }
```

---

## Configuration

### GET /autostart

Get the project currently configured as autostart.

**Response:**
- Status: `200 OK`
- `Content-Type: application/json`
- `Cache-Control: no-cache, must-revalidate`

**Response body (project set):**
```json
{ "project": "MyProject" }
```

**Response body (no autostart configured):**
```json
{}
```

---

### PUT /autostart

Set the autostart project. The name is persisted to `/autostart.json` on the SD card.

**Request:**
- `Content-Type: application/json`

**Request body:**
```json
{ "project": "MyProject" }
```

**Response:**
- Status: `201 Created` — saved successfully
- Status: `501 Not Implemented` — save failed (e.g. SD card error)
- `Content-Length: 0`
- `Cache-Control: no-cache, must-revalidate`

---

## Device Discovery

### GET /description.xml

Returns the SSDP/UPnP device description. Advertised via SSDP multicast every 5 seconds so that network tools can discover the device automatically.

**Response:**
- Status: `200 OK`
- `Content-Type: application/xml`
- `Cache-Control: no-cache, must-revalidate`

**Response body:**
```xml
<?xml version='1.0'?>
<root xmlns='urn:schemas-upnp-org:device-1-0'>
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
    <friendlyName>Megahub</friendlyName>
    <manufacturer>Megahub Project</manufacturer>
    <modelName>Megahub</modelName>
    <modelNumber>1.0</modelNumber>
    <serialNumber>AABBCCDDEEFF</serialNumber>
    <UDN>uuid:aabbccddeeff</UDN>
    <presentationURL>http://aabbccddeeff.local:80/index.html</presentationURL>
  </device>
</root>
```

---

## Server-Sent Events (SSE)

### GET /events

Opens a persistent Server-Sent Events connection. The server pushes log messages, UI commands, and port status updates in real time. Keep this connection open while the IDE is active.

**Response:**
- `Content-Type: text/event-stream`
- `Connection: keep-alive`

Each event has a named `event` field and a `data` field containing a JSON object.

---

### Event: `log`

A log or print message from the device. Generated by Lua `print()` or by firmware log macros (INFO, WARN, DEBUG).

**Data:**
```json
{ "message": "Motor speed set to 80" }
```

---

### Event: `command`

A UI command from the running program. Two sub-types exist, identified by the `type` field.

**`show_value`** — from Lua `ui.showvalue()`:
```json
{
  "type": "show_value",
  "label": "Sensor reading",
  "format": "simple",
  "value": 42
}
```

`value` may be a number, boolean, or string. `format` is `"simple"` when `FORMAT_SIMPLE` is used in Lua.

**`thread_statistics`** — sent every 10 s when a thread runs with profiling enabled:
```json
{
  "type": "thread_statistics",
  "blockid": "block_motor_loop",
  "min": 1240,
  "avg": 1850.4,
  "max": 3100
}
```

All timing values are in microseconds.

---

### Event: `portstatus`

Sent whenever the LEGO port connection state changes. Contains the current status of all four ports.

**Data:**
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

When `connected` is `false`, no `device` field is present. `datasets` is the number of separate values returned by `lego.getmodedataset()` for that mode.

---

## Error Handling

| Status | Meaning |
|--------|---------|
| `200 OK` | Request succeeded |
| `201 Created` | Resource created or updated successfully |
| `501 Not Implemented` | Operation failed (typically an SD card write error) |

All dynamic endpoints include `Cache-Control: no-cache, must-revalidate` to prevent stale responses being served from browser caches.

---

## Notes

- Project names in URLs must be URL-encoded (spaces as `%20`, etc.)
- The server uses mDNS (`<device-uid>.local`) for zero-configuration discovery on the local network
- SSDP/UPnP discovery is broadcast every 5 seconds so tools like Windows Network Explorer can find the device
- The `/syntaxcheck` and `/execute` endpoints write code to temporary files on the SD card before processing — a working SD card is required
- The SSE `/events` endpoint should remain open for the duration of an active session
