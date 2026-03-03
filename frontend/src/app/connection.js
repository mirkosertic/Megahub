/**
 * BLE connection lifecycle management.
 *
 * Handles the full connect / reconnect / disconnect flow including:
 * - Connection progress modal UI
 * - Status bar updates
 * - Exponential-backoff reconnection
 * - Registering BLE event listeners after connection
 */
import { bleClient } from './ble-instance.js';
import { setState } from './state.js';
import {
    APP_EVENT_TYPE_LOG,
    APP_EVENT_TYPE_PORTSTATUS,
    APP_EVENT_TYPE_COMMAND,
    APP_EVENT_TYPE_BTCLASSICDEVICES,
    APP_REQUEST_TYPE_READY_FOR_EVENTS,
} from '../bleclient.js';
import { jumpToFilesView, setInitState } from './app.js';

// Reconnection state — module-level so it persists across reconnect attempts
let _isReconnecting = false;
let _reconnectAttempts = 0;
const MAX_RECONNECT_ATTEMPTS = 10;

/** Reference to the "Connection Lost" notification so it can be dismissed on reconnect */
let _connectionLostNotification = null;

/** Guard against registering the disconnect handler multiple times */
let _disconnectHandlerRegistered = false;

// ─── Connection progress modal ───────────────────────────────────────────────

const ConnectionStepDefs = [
    { id: 'requesting', label: 'Requesting device...' },
    { id: 'connecting', label: 'Connecting to GATT server...' },
    { id: 'services', label: 'Discovering services...' },
    { id: 'notifications', label: 'Enabling notifications...' },
    { id: 'mtu', label: 'Negotiating MTU...' },
    { id: 'ready', label: 'Connection established!' },
];

const ConnectionModal = {
    _rendered: false,

    _render() {
        if (this._rendered) return;
        this._rendered = true;
        const stepsEl = document.getElementById('connectionSteps');
        if (!stepsEl) return;
        stepsEl.innerHTML = ConnectionStepDefs.map(
            (s) => `
            <div class="connection-step pending" id="cstep-${s.id}">
                <div class="connection-step-indicator">
                    <div class="connection-step-spinner"></div>
                    <svg class="connection-step-check" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>
                    <svg class="connection-step-error-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                </div>
                <span class="connection-step-label">${s.label}</span>
            </div>
        `
        ).join('');
    },

    show() {
        this._render();
        ConnectionStepDefs.forEach((s) => {
            const el = document.getElementById('cstep-' + s.id);
            if (el) el.className = 'connection-step pending';
        });
        const errEl = document.getElementById('connectionModalError');
        if (errEl) {
            errEl.textContent = '';
            errEl.classList.remove('visible');
        }
        const overlay = document.getElementById('connectionModal');
        if (overlay) overlay.setAttribute('aria-hidden', 'false');
    },

    hide() {
        const overlay = document.getElementById('connectionModal');
        if (overlay) overlay.setAttribute('aria-hidden', 'true');
    },

    setStep(stepId) {
        // Mark all previous steps as done, current as active, rest as pending
        let found = false;
        ConnectionStepDefs.forEach((s) => {
            const el = document.getElementById('cstep-' + s.id);
            if (!el) return;
            if (s.id === stepId) {
                el.className = 'connection-step active';
                found = true;
            } else if (!found) {
                el.className = 'connection-step done';
            } else {
                el.className = 'connection-step pending';
            }
        });
    },

    setAllDone() {
        ConnectionStepDefs.forEach((s) => {
            const el = document.getElementById('cstep-' + s.id);
            if (el) el.className = 'connection-step done';
        });
    },

    setError(stepId, message) {
        // Mark current step as error
        const el = document.getElementById('cstep-' + stepId);
        if (el) el.className = 'connection-step error';
        const errEl = document.getElementById('connectionModalError');
        if (errEl) {
            errEl.textContent = message;
            errEl.classList.add('visible');
        }
    },
};

// ─── Status bar ──────────────────────────────────────────────────────────────

const StatusBar = {
    setConnecting() {
        const dot = document.getElementById('statusbarDot');
        const label = document.getElementById('statusbarConnectionLabel');
        if (dot) dot.className = 'statusbar-dot connecting';
        if (label) label.textContent = 'Connecting...';
    },
    setConnected(deviceName) {
        const dot = document.getElementById('statusbarDot');
        const label = document.getElementById('statusbarConnectionLabel');
        if (dot) dot.className = 'statusbar-dot connected';
        if (label) label.textContent = deviceName ? `Connected to ${deviceName}` : 'Connected';
    },
    setDisconnected() {
        const dot = document.getElementById('statusbarDot');
        const label = document.getElementById('statusbarConnectionLabel');
        if (dot) dot.className = 'statusbar-dot disconnected';
        if (label) label.textContent = 'Not connected';
        this.clearMessage();
    },
    setMessage(text) {
        const el = document.getElementById('statusbarMessage');
        if (el) el.textContent = text;
    },
    clearMessage() {
        const el = document.getElementById('statusbarMessage');
        if (el) el.textContent = '';
    },
};

// ─── Progress bar ─────────────────────────────────────────────────────────────

export const Progress = {
    show() {
        const bar = document.getElementById('progressBar');
        if (bar) bar.classList.add('active');
    },
    hide() {
        const bar = document.getElementById('progressBar');
        if (bar) bar.classList.remove('active');
    },
};

// ─── BLE event wiring ─────────────────────────────────────────────────────────

/**
 * Register BLE event listeners.
 * Translates BLE events into state updates or direct component calls.
 * logger, uiComponents, mapComponent, and blocklyEditor are passed in from index.js
 * since they are DOM elements managed at the top level.
 *
 * @param {{ logger: HTMLElement, uiComponents: HTMLElement, mapComponent: HTMLElement, blocklyEditor: HTMLElement }} elements
 */
function setupEventListeners({ logger, uiComponents, mapComponent, blocklyEditor }) {
    bleClient.removeAllEventListeners();

    bleClient.addEventListener(APP_EVENT_TYPE_LOG, (data) => {
        logger.addToLog(new TextDecoder().decode(data));
    });

    bleClient.addEventListener(APP_EVENT_TYPE_PORTSTATUS, (data) => {
        const status = JSON.parse(new TextDecoder().decode(data));
        console.log('Got Portstatus : ' + JSON.stringify(status));
        // Update state — portstatus component subscribes and re-renders automatically
        setState({ portStatuses: status.ports });
    });

    bleClient.addEventListener(APP_EVENT_TYPE_COMMAND, (data) => {
        const command = JSON.parse(new TextDecoder().decode(data));
        if (command.type === 'thread_statistics') {
            blocklyEditor.addProfilingOverlay(command.blockid, command.min, command.avg, command.max);
        } else if (command.type === 'map_points' || command.type === 'map_clear') {
            mapComponent.processUIEvent(command);
        } else {
            uiComponents.processUIEvent(command);
        }
    });

    bleClient.addEventListener(APP_EVENT_TYPE_BTCLASSICDEVICES, (data) => {
        console.debug('Got Bluetooth Device List:', new TextDecoder().decode(data));
        // Update state — btdevicelist component subscribes and re-renders automatically
        setState({ deviceList: JSON.parse(new TextDecoder().decode(data)) });
    });
}

// ─── Connect / reconnect ─────────────────────────────────────────────────────

/**
 * Handle unexpected BLE disconnection.
 * Updates the UI and starts the exponential-backoff reconnect loop.
 */
async function handleDisconnect() {
    console.log('Connection terminated!');
    setState({ isConnected: false });
    StatusBar.setDisconnected();
    _connectionLostNotification = window.showNotification(
        'warning',
        'Connection Lost',
        'Attempting to reconnect...',
        0
    );
    attemptReconnection();
}

/**
 * Attempt to reconnect with exponential backoff.
 * Re-uses _doConnect() in reconnect mode (no modal, no navigation).
 */
async function attemptReconnection() {
    if (_isReconnecting) return;

    _isReconnecting = true;
    _reconnectAttempts = 0;

    while (_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        _reconnectAttempts++;
        // Delay: 2s, 4s, 8s, 16s, 30s max
        const delayMs = Math.min(2000 * Math.pow(2, _reconnectAttempts - 1), 30000);

        console.log(`Reconnection attempt ${_reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS} in ${delayMs / 1000}s...`);
        // Notification duration proportional to delay time (90% of delay)
        window.showNotification(
            'info',
            'Reconnecting',
            `Attempt ${_reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS} in ${delayMs / 1000}s...`,
            Math.floor(delayMs * 0.9)
        );

        await new Promise((resolve) => setTimeout(resolve, delayMs));

        try {
            // _elements reference is stashed in module scope by initBLEConnection
            await _doConnect(true);

            // Dismiss the persistent "Connection Lost" notification
            if (_connectionLostNotification) {
                _connectionLostNotification.querySelector('.notification-close')?.click();
                _connectionLostNotification = null;
            }
            window.showNotification('success', 'Reconnected', 'Connection restored successfully');
            _isReconnecting = false;
            _reconnectAttempts = 0;
            return;
        } catch (err) {
            console.warn(`Reconnection attempt ${_reconnectAttempts} failed:`, err.message);
        }
    }

    _isReconnecting = false;
    window.showNotification(
        'error',
        'Reconnection Failed',
        'Could not reconnect. Please refresh and connect manually.',
        0
    );
    // Return to connect screen
    setInitState();
}

/** Stash of DOM element references needed by setupEventListeners during reconnect */
let _elements = null;

/**
 * Internal connect implementation, used for both initial connect and reconnect.
 * @param {boolean} reconnecting
 */
async function _doConnect(reconnecting) {
    if (reconnecting) {
        await bleClient.reconnect();
    } else {
        await bleClient.connect((stepId) => ConnectionModal.setStep(stepId));
    }

    setState({ isConnected: true });
    StatusBar.setConnected(bleClient.device?.name);

    // Reset component state so stale data is cleared
    setState({ portStatuses: [], deviceList: [] });

    // Wire up BLE events
    setupEventListeners(_elements);

    // Tell device we are ready to receive events
    console.log('Notifying, I am ready!');
    await bleClient.sendRequest(APP_REQUEST_TYPE_READY_FOR_EVENTS, JSON.stringify({}));
    console.log('Now I should get log messages');
}

/**
 * Initiate BLE connection (initial user-triggered connect).
 * Shows the connection modal, updates the status bar, and navigates to
 * the project management view on success.
 *
 * @param {{ logger: HTMLElement, uiComponents: HTMLElement, blocklyEditor: HTMLElement }} elements
 *   DOM element references needed to wire up BLE event listeners.
 */
export async function initBLEConnection(elements) {
    _elements = elements;

    try {
        ConnectionModal.show();
        ConnectionModal.setStep('requesting');
        StatusBar.setConnecting();

        await _doConnect(false);

        ConnectionModal.setAllDone();
        // Brief pause so user sees the completed state, then close
        setTimeout(() => ConnectionModal.hide(), 600);

        // Register disconnect handler only once across multiple (re)connections
        if (!_disconnectHandlerRegistered) {
            bleClient.onDisconnect(() => handleDisconnect());
            _disconnectHandlerRegistered = true;
        }

        // Navigate to project list on initial connect
        if (!_isReconnecting) {
            await jumpToFilesView();
        }
    } catch (error) {
        console.error('BLE connection error:', error);
        const activeStep = document.querySelector('.connection-step.active');
        const stepId = activeStep ? activeStep.id.replace('cstep-', '') : 'connecting';
        ConnectionModal.setError(stepId, error.message || 'Connection failed');
        // Auto-close error modal after 4 seconds
        setTimeout(() => ConnectionModal.hide(), 4000);
        throw error;
    }
}
