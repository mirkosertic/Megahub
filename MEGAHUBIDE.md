# Megahub IDE — Frontend Architecture

This document describes the architecture of the Megahub web IDE. It is aimed at developers who want to understand, extend, or test the frontend. User-facing information is in [README.md](README.md).

---

## Overview

The IDE is a single-page web application built with [Vite](https://vitejs.dev/) and plain ES modules — no framework, no virtual DOM. It runs entirely in the browser and communicates with the Megahub device either via the Web Bluetooth API (BLE mode) or HTTP (WiFi mode).

The UI is composed of [Web Components](https://developer.mozilla.org/en-US/docs/Web/API/Web_components) (Custom Elements with Shadow DOM). Each component is self-contained: it owns its own HTML template and CSS stylesheet, which are imported at build time as raw strings and adopted via `CSSStyleSheet`.

---

## Build Modes

Vite is configured with three named modes, selected at build time:

| Mode | Command | Description |
|------|---------|-------------|
| `dev` | `npm run dev` | Local dev server; uses `localStorage` for persistence, no real device needed |
| `bt` | `npm run build:bt` | Hosted on GitHub Pages; communicates with the device over BLE |
| `web` | `npm run build:web` | Embedded in firmware flash; communicates with the ESP32 HTTP server |

The active mode is exposed to all modules via `import.meta.env.VITE_MODE`. Each module that talks to the device branches on this value to call either BLE or HTTP endpoints.

---

## Source Layout

```
frontend/
├── index.html                  Entry point HTML
├── src/
│   ├── index.js                Application wiring layer (thin)
│   ├── bleclient.js            BLE protocol client
│   ├── theme.css               VS Code dark theme CSS variables
│   ├── styles.css              Global layout and utility styles
│   ├── app/
│   │   ├── state.js            Observable state store
│   │   ├── events.js           CustomEvent name constants
│   │   ├── ble-instance.js     Shared BLEClient singleton
│   │   ├── app.js              Application logic (project management, code execution)
│   │   ├── connection.js       BLE connection lifecycle and reconnection
│   │   └── autosave.js         Periodic auto-save timer
│   └── components/
│       ├── blockly/            Blockly workspace editor + block definitions
│       ├── btdevicelist/       Bluetooth Classic device list and pairing
│       ├── files/              Project list, create/open/delete/autostart
│       ├── logger/             Real-time log output panel
│       ├── luapreview/         Generated Lua code preview panel
│       ├── portstatus/         LEGO port connection status grid
│       ├── sidebar-toggle/     Accordion sidebar controller
│       └── ui/                 Button bar (Execute, Stop, Save, Autosave)
└── test/
    └── setup.js                Vitest global setup (state reset before each test)
```

---

## Architecture Principles

### 1. No global state on `window`

All application state is held in the `state.js` module. No `window.Application` or other globals. Components and modules import only what they need.

### 2. One-way data flow

```
User action
  └─► Component dispatches CustomEvent (bubbles: true, composed: true)
        └─► index.js hears it on document
              └─► Calls App function (app.js)
                    └─► Talks to device (BLE or HTTP)
                          └─► Calls setState()
                                └─► Notifies subscribers
                                      └─► Components re-render
```

Components never import `app.js`. They communicate upward only through events and receive data only through state subscriptions. This keeps components independently testable.

### 3. Thin wiring layer

`index.js` is the only place that knows about both components and the app module. Its job is to:
- Register `document`-level CustomEvent listeners and route them to App functions
- Subscribe to state keys that affect non-component DOM (e.g., the header breadcrumb)
- Wire toolbar buttons (Execute, Stop, Save, Autosave) to App functions
- Set up the initial UI state on page load

---

## State Management (`src/app/state.js`)

A minimal pub/sub store (~80 lines, no dependencies).

### State keys

| Key | Type | Description |
|-----|------|-------------|
| `portStatuses` | `Array` | Current LEGO port connection data from the device |
| `deviceList` | `Array` | Bluetooth Classic device list (for pairing panel) |
| `isConnected` | `boolean` | Whether the BLE connection is active |
| `activeProject` | `string\|null` | Name of the currently open project |
| `projects` | `Array` | Project list shown in the files panel |
| `autostartProject` | `string\|null` | Name of the autostart-marked project |

### API

```js
import { getState, setState, subscribe, resetState } from './app/state.js';

// Read a value
const project = getState('activeProject');

// Write one or more values — notifies subscribers for each changed key
setState({ portStatuses: ports, isConnected: true });

// Subscribe to a single key — returns an unsubscribe function
const unsub = subscribe('projects', projects => this.render(projects));
// Call unsub() in disconnectedCallback to prevent memory leaks

// Reset everything — used in test setup
resetState();
```

---

## Custom Events (`src/app/events.js`)

Components signal user intent by dispatching CustomEvents with `bubbles: true, composed: true` so they cross Shadow DOM boundaries and reach `document`. `index.js` listens at the document level.

| Constant | Event name | `detail` payload | Meaning |
|----------|-----------|-----------------|---------|
| `APP_EVENT_PROJECT_OPEN` | `app:project:open` | `{ id }` | User clicked Open |
| `APP_EVENT_PROJECT_CREATE` | `app:project:create` | `{ id }` | User clicked Create |
| `APP_EVENT_PROJECT_DELETE` | `app:project:delete` | `{ id }` | User clicked Delete |
| `APP_EVENT_AUTOSTART_SET` | `app:autostart:set` | `{ project }` | User clicked autostart star |
| `APP_EVENT_BT_DISCOVER` | `app:bt:discover` | `{}` | User clicked Scan |
| `APP_EVENT_BT_PAIR` | `app:bt:pair` | `{ mac }` | User clicked Pair |
| `APP_EVENT_BT_UNPAIR` | `app:bt:unpair` | `{ mac }` | User clicked Unpair |

---

## Application Module (`src/app/app.js`)

Named exports, no class, no singleton pattern. Each function is independently callable and testable.

| Export | Description |
|--------|-------------|
| `setMode(mode)` | Switch the UI between `btconnect`, `management`, `editor` views |
| `setInitState()` | Reset to the BLE connect screen |
| `jumpToFilesView()` | Load projects + autostart from device, navigate to management view |
| `createProject(id)` | Switch to editor for a new empty project |
| `openProject(id)` | Load project file and switch to editor |
| `deleteProject(id)` | Delete from device and refresh project list |
| `setAutostartProject(project)` | Persist autostart selection to device |
| `requestProjectFile(id, filename)` | Fetch a file from the active project |
| `saveProjectFile(filename, type, content)` | Upload a file to the active project |
| `syntaxCheck(luaCode)` | Run a Lua syntax check on the device |
| `executeCode(luaCode)` | Upload and execute a Lua program |
| `stop()` | Stop the running program |
| `requestPairing(mac)` | Initiate Bluetooth Classic pairing |
| `requestRemovePairing(mac)` | Remove a pairing |
| `startBluetoothDiscovery()` | Start a Bluetooth Classic device scan |

### Mode branching

Every device-facing function contains a `mode` branch:

```js
const mode = import.meta.env.VITE_MODE;  // 'dev' | 'bt' | 'web'

export async function executeCode(luaCode) {
    if (mode === 'dev')  { return true; }
    if (mode === 'bt')   { /* BLE sendRequest */ }
    if (mode === 'web')  { /* fetch('/execute', ...) */ }
}
```

---

## BLE Connection (`src/app/connection.js`)

Manages the full connect / disconnect / reconnect lifecycle.

### Entry point

```js
import { initBLEConnection } from './app/connection.js';

await initBLEConnection({ logger, uiComponents, blocklyEditor });
```

### What it does

1. Shows the 6-step connection progress modal
2. Calls `bleClient.connect()` with a step-update callback
3. On success: updates status bar, wires BLE event listeners, sends `READY_FOR_EVENTS`, then navigates to the project list
4. On failure: shows the error state in the modal (auto-closes after 4 s)
5. Registers a disconnect handler (only once) that triggers reconnection

### Reconnection

Exponential backoff: 2 s → 4 s → 8 s → 16 s → 30 s (cap), up to 10 attempts. A persistent "Connection Lost" notification is shown and dismissed on success. After 10 failed attempts the UI returns to the connect screen.

### BLE event → state mapping

| BLE event | Action |
|-----------|--------|
| `APP_EVENT_TYPE_PORTSTATUS` | `setState({ portStatuses: status.ports })` |
| `APP_EVENT_TYPE_BTCLASSICDEVICES` | `setState({ deviceList: data })` |
| `APP_EVENT_TYPE_LOG` | Direct call: `logger.addToLog(text)` |
| `APP_EVENT_TYPE_COMMAND` | Direct call: `blocklyEditor.addProfilingOverlay()` or `uiComponents.processUIEvent()` |

Log and command events are delivered directly to components because they are append-only streams, not replaceable state.

---

## BLE Client (`src/bleclient.js`)

Low-level Web Bluetooth API wrapper. Manages the GATT connection, characteristic notifications, MTU negotiation, request/response framing, and streaming file upload. `ble-instance.js` exports a single shared instance that `app.js` and `connection.js` both import.

---

## Components

Each component lives in its own directory: `component.js`, `component.html`, and `style.css`. The HTML template and stylesheet are inlined at build time.

### Pattern: state subscription

```js
connectedCallback() {
    this._unsub = subscribe('projects', projects => {
        if (projects) this.render(projects);
    });
}
disconnectedCallback() {
    this._unsub?.();
}
```

### Pattern: event dispatch

```js
// User interaction → CustomEvent → index.js → App function
this.dispatchEvent(new CustomEvent(APP_EVENT_PROJECT_OPEN, {
    bubbles: true,
    composed: true,
    detail: { id: projectName }
}));
```

### Component inventory

| Component | State subscriptions | Events dispatched |
|-----------|--------------------|--------------------|
| `custom-files` | `projects`, `autostartProject` | `PROJECT_OPEN`, `PROJECT_CREATE`, `PROJECT_DELETE`, `AUTOSTART_SET` |
| `custom-portstatus` | `portStatuses` | — |
| `custom-btdevicelist` | `deviceList` | `BT_DISCOVER`, `BT_PAIR`, `BT_UNPAIR` |
| `custom-logger` | — | — |
| `custom-blockly` | — | — |
| `custom-luapreview` | — | — |
| `custom-ui` | — | — |
| `custom-sidebar-toggle` | — | — |

---

## Auto-Save (`src/app/autosave.js`)

A 10-second interval timer, toggled by the autosave toolbar button. The save function is injected by `index.js` at startup via `setSaveFunction(fn)`. The timer only fires if `getState('activeProject')` is non-null, so it does nothing while no project is open.

---

## UI Modes and Visibility

The layout has three display modes, controlled by `setMode()`:

| Mode | When | What is visible |
|------|------|----------------|
| `btconnect` | On load (BLE build) | Connect button only |
| `management` | After connect | Project list, logger |
| `editor` | Project open | Blockly editor, toolbar, sidebar panels |

Two visibility mechanisms are in use:

1. **`dynamicvisibility` class** — `setMode()` sets `display: none/block` imperatively for elements with a matching `visible-<mode>` class.
2. **`body[data-mode]` CSS attribute** — used for elements that need `display: flex` or `display: grid` instead of `block` (e.g., the header back button).

---

## Testing

Tests use [Vitest](https://vitest.dev/) with [happy-dom](https://github.com/capricorn86/happy-dom) as the DOM environment. happy-dom is used instead of jsdom because it has proper Shadow DOM support, which is required for testing Web Components.

### Running tests

```bash
cd frontend
npm test                  # single run
npm run test:watch        # watch mode
npm run test:coverage     # coverage report (V8 provider)
```

Tests are co-located with source files (`src/**/*.test.js`) or placed in `test/`.

### State isolation

`test/setup.js` calls `resetState()` in a `beforeEach` hook so each test starts with a clean state and no lingering subscribers.

### What is tested

| Test file | Coverage |
|-----------|---------|
| `src/app/state.test.js` | `getState`, `setState`, `subscribe`, unsubscribe, `resetState`, multi-subscriber, unknown key |
| `src/components/blockly/mh_wait.test.js` | Block definition structure, Lua code generator output |

### What to test next

Priority areas for additional test coverage:

1. **`files` component** — render from state, CustomEvent dispatch on button clicks, autostart star toggle vs. backend notification split
2. **`portstatus` component** — renders connected and disconnected port cards correctly
3. **`btdevicelist` component** — scan, pair, unpair event dispatch; paired badge rendering
4. **`app.js` pure functions** — `validateProjectName` equivalents, mode branching with mocked `fetch`/`bleClient`
5. **`autosave.js`** — enable/disable, timer fires only when project is open (use `vi.useFakeTimers()`)

---

## Adding a New Component

1. Create `src/components/<name>/component.js`, `component.html`, `style.css`.
2. In `component.js`:
   - Import state subscriptions you need from `../../app/state.js`
   - Import event constants from `../../app/events.js`
   - Subscribe in `connectedCallback`, unsubscribe in `disconnectedCallback`
   - Dispatch CustomEvents for user actions — never call `app.js` directly
3. Register the custom element: `customElements.define('custom-<name>', YourClass)`
4. Add the element to `index.html` where needed
5. In `index.js`, add `document.addEventListener(YOUR_EVENT, ...)` handlers if the component introduces new event types
6. Add the new event constants to `events.js`

---

## Adding a New State Key

1. Add the key and its default value to `_state` in `state.js`
2. Add the same key to the `resetState()` call in `state.js`
3. Update the JSDoc type annotation at the top of `state.js`
4. Call `setState({ yourKey: value })` wherever the value is produced (typically in `connection.js` or `app.js`)
5. Subscribe to it in any component that needs it
