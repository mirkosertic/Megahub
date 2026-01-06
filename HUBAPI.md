# Hub Web Server API Documentation

This document describes the REST API endpoints and Server-Sent Events (SSE) for the LEGO ESP32 Hub Web Server.

## Base URL

The server runs on port 80 and is accessible via:
- `http://<device-ip>/`
- `http://<device-uid>.local/` (via mDNS)

## Table of Contents

- [Static Resources](#static-resources)
- [Project Management](#project-management)
- [Code Execution](#code-execution)
- [Configuration](#configuration)
- [Device Discovery](#device-discovery)
- [Server-Sent Events (SSE)](#server-sent-events-sse)

---

## Static Resources

### GET /

Serves the main application HTML page.

**Response:**
- Content-Type: `text/html`
- Content-Encoding: `gzip`

### GET /index.js

Serves the main application JavaScript file.

**Response:**
- Content-Type: `text/javascript`
- Content-Encoding: `gzip`

### GET /project.js

Serves the project-related JavaScript file.

**Response:**
- Content-Type: `text/javascript`
- Content-Encoding: `gzip`

### GET /component.js

Serves the component JavaScript file.

**Response:**
- Content-Type: `text/javascript`
- Content-Encoding: `gzip`

### GET /style.css

Serves the application stylesheet.

**Response:**
- Content-Type: `text/css`
- Content-Encoding: `gzip`

---

## Project Management

### GET /projects

Retrieves a list of all available projects.

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```json
{
  "projects": [
    {
      "name": "project1"
    },
    {
      "name": "project2"
    }
  ]
}
```

### GET /project/{projectId}

Serves the project HTML page.

**URL Parameters:**
- `projectId` - URL-encoded project identifier

**Response:**
- Status: `200 OK`
- Content-Type: `text/html`
- Content-Encoding: `gzip`
- Cache-Control: `no-cache, must-revalidate`

### GET /project/{projectId}/model.xml

Downloads the `model.xml` file for a specific project.

**URL Parameters:**
- `projectId` - URL-encoded project identifier

**Response:**
- Status: `200 OK`
- Content-Type: `application/octet-stream`
- Cache-Control: `no-cache, must-revalidate`

### PUT /project/{projectId}/{filename}

Uploads a file to a project using the body of the PUT operation.

**URL Parameters:**
- `projectId` - URL-encoded project identifier
- `filename` - Name of the file to upload

**Request:**
- Body: the content of the file

**Response:**
- Status: `201 Created`
- Content-Length: `0`
- Cache-Control: `no-cache, must-revalidate`

### DELETE /project/{projectId}

Deletes a project and all its associated files.

**URL Parameters:**
- `projectId` - URL-encoded project identifier

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```json
{}
```

---

## Code Execution

### PUT /stop

Stops the currently running Lua code.

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```json
{
  "success": true
}
```

### PUT /syntaxcheck

Checks the syntax of uploaded Lua code without executing it.

**Request:**
- Body: Lua code to check

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body (Success):**
```json
{
  "success": true,
  "parseTime": 123,
  "errorMessage": ""
}
```

**Response Body (Failure):**
```json
{
  "success": false,
  "parseTime": 0,
  "errorMessage": "syntax error description"
}
```

### PUT /execute

Uloadeds and executes Lua code on the device.

**Request:**
- Body: Lua code to execute

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```json
{
  "success": true
}
```

---

## Configuration

### GET /autostart

Retrieves the project configured to start automatically on device boot.

**Response:**
- Status: `200 OK`
- Content-Type: `application/json`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```json
{
  "project": "project_name"
}
```

**Note:** If no autostart project is configured, the `project` field will be omitted.

### PUT /autostart

Sets the project to start automatically on device boot.

**Request:**
- Content-Type: `application/json`

**Request Body:**
```json
{
  "project": "project_name"
}
```

**Response:**
- Status: `201 Created` (success) or `501 Not Implemented` (failure)
- Content-Length: `0`
- Cache-Control: `no-cache, must-revalidate`

---

## Device Discovery

### GET /description.xml

Returns the SSDP (Simple Service Discovery Protocol) device description XML.

**Response:**
- Status: `200 OK`
- Content-Type: `application/xml`
- Cache-Control: `no-cache, must-revalidate`

**Response Body:**
```xml
<?xml version='1.0'?>
<root xmlns='urn:schemas-upnp-org:device-1-0'>
  <specVersion>
    <major>1</major>
    <minor>0</minor>
  </specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
    <friendlyName>Device Name</friendlyName>
    <manufacturer>Manufacturer Name</manufacturer>
    <modelName>Model Name</modelName>
    <modelNumber>Version</modelNumber>
    <serialNumber>Serial Number</serialNumber>
    <UDN>uuid:device-uid</UDN>
    <presentationURL>http://device-uid.local:80/index.html</presentationURL>
  </device>
</root>
```

---

## Server-Sent Events (SSE)

### GET /events

Establishes a Server-Sent Events connection for real-time updates.

**Response:**
- Content-Type: `text/event-stream`
- Connection: `keep-alive`

### Event Types

#### `log`

Emitted when a log message is generated by the device.

**Event Data:**
```json
{
  "message": "Log message text"
}
```

#### `command`

Commands are issued by the Hub and sent to the Client to perform certain tasks like UI / Debug output etc.

**Event Data:**
```json
{
  "type": "show_value",
  "label": "The label to be shown before the value",
  "format": "FORMAT_SIMPLE",  
  "value": "The value to be show"
}
```

#### `portstatus`

Emitted when the status of a device port changes.

**Event Data:**
```json
{
  "ports": [
    {
      "id": 1,
      "connected": false
    },
    {
      "id": 2,
      "connected": false
    },
    {
      "id": 3,
      "connected": true,
      "device": {
        "type": "BOOST Color and Distance Sensor",
        "icon": "⚙️",
        "modes": [
          "COLOR",
          "PROX",
          "COUNT",
          "REFLT",
          "AMBI",
          "COL O",
          "RGB I",
          "IR Tx"
        ]
      }
    },
    {
      "id": 4,
      "connected": false
    }
  ]
}
```

---

## Error Handling

All API endpoints use standard HTTP status codes:

- `200 OK` - Request succeeded
- `201 Created` - Resource created successfully
- `501 Not Implemented` - Operation failed

Most endpoints include a `Cache-Control: no-cache, must-revalidate` header to prevent caching of dynamic content.

---

## Notes

- All project identifiers in URLs should be URL-encoded
- The server supports SSDP discovery for automatic device detection on the network
- mDNS is enabled for accessing the device via `<device-uid>.local` hostname
- SSE connections remain open for real-time updates and should be handled appropriately by clients
