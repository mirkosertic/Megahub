// ─── Component registration (side effects only) ───────────────────────────────
import './components/files/component.js';
import './components/logger/component.js';
import './components/sidebar-toggle/component.js';
import './components/ui/component.js';
import './components/map/component.js';
import './components/portstatus/component.js';
import './components/blockly/component.js';
import './components/luapreview/component.js';
import './components/btdevicelist/component.js';

// ─── App modules ──────────────────────────────────────────────────────────────
import * as App from './app/app.js';
import { initBLEConnection, Progress } from './app/connection.js';
import { setState, subscribe, getState } from './app/state.js';
import {
    setSaveFunction,
    enable as enableAutosave,
    disable as disableAutosave,
    isEnabled as isAutosaveEnabled,
} from './app/autosave.js';
import {
    APP_EVENT_PROJECT_OPEN,
    APP_EVENT_PROJECT_CREATE,
    APP_EVENT_PROJECT_DELETE,
    APP_EVENT_AUTOSTART_SET,
    APP_EVENT_BT_DISCOVER,
    APP_EVENT_BT_PAIR,
    APP_EVENT_BT_UNPAIR,
} from './app/events.js';

// Note: APP_EVENT_TYPE_COMMAND is handled by the web EventSource 'command' event name string directly

const mode = import.meta.env.VITE_MODE;

// ─── Component references (set in DOMContentLoaded) ──────────────────────────
let blocklyEditor = null;
let luaPreview = null;
let uiComponents = null;
let mapComponent = null;

// ===== Notification System =====

const NotificationIcons = {
    success: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>`,
    error: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>`,
    warning: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>`,
    info: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>`,
};

/**
 * Show a notification toast
 * @param {string} type - 'success' | 'error' | 'warning' | 'info'
 * @param {string} title - Notification title
 * @param {string} message - Notification message (optional)
 * @param {number} duration - Auto-dismiss duration in ms (0 = no auto-dismiss)
 */
function showNotification(type, title, message = '', duration = 4000) {
    const container = document.getElementById('notificationContainer');
    if (!container) return;

    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.innerHTML = `
		<div class="notification-icon">${NotificationIcons[type] || NotificationIcons.info}</div>
		<div class="notification-content">
			<div class="notification-title">${title}</div>
			${message ? `<div class="notification-message">${message}</div>` : ''}
		</div>
		<button class="notification-close" aria-label="Dismiss">
			<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
		</button>
	`;

    const closeBtn = notification.querySelector('.notification-close');
    const dismiss = () => {
        notification.classList.add('hiding');
        setTimeout(() => notification.remove(), 200);
    };

    closeBtn.addEventListener('click', dismiss);

    container.appendChild(notification);

    if (duration > 0) {
        setTimeout(dismiss, duration);
    }

    return notification;
}

// ===== Confirmation Dialog =====

let confirmDialogResolve = null;

/**
 * Show a confirmation dialog
 * @param {string} title - Dialog title
 * @param {string} message - Dialog message
 * @param {Object} options - Optional settings
 * @param {string} options.confirmText - Confirm button text (default: 'Confirm')
 * @param {string} options.cancelText - Cancel button text (default: 'Cancel')
 * @param {boolean} options.destructive - If true, confirm button is red (default: true)
 * @returns {Promise<boolean>} - Resolves to true if confirmed, false if cancelled
 */
function showConfirmDialog(title, message, options = {}) {
    const { confirmText = 'Confirm', cancelText = 'Cancel', destructive = true } = options;

    return new Promise((resolve) => {
        const overlay = document.getElementById('confirmDialog');
        const titleEl = document.getElementById('confirmDialogTitle');
        const messageEl = document.getElementById('confirmDialogMessage');
        const confirmBtn = document.getElementById('confirmDialogConfirm');
        const cancelBtn = document.getElementById('confirmDialogCancel');

        if (!overlay) {
            resolve(false);
            return;
        }

        titleEl.textContent = title;
        messageEl.textContent = message;
        confirmBtn.textContent = confirmText;
        cancelBtn.textContent = cancelText;

        // Set button style based on destructive flag
        confirmBtn.classList.toggle('primary', !destructive);

        confirmDialogResolve = resolve;
        overlay.setAttribute('aria-hidden', 'false');

        // Focus the cancel button for safety
        setTimeout(() => cancelBtn.focus(), 100);
    });
}

function closeConfirmDialog(result) {
    const overlay = document.getElementById('confirmDialog');
    if (overlay) {
        overlay.setAttribute('aria-hidden', 'true');
    }
    if (confirmDialogResolve) {
        confirmDialogResolve(result);
        confirmDialogResolve = null;
    }
}

// Set up dialog event listeners once DOM is ready
function initConfirmDialog() {
    const overlay = document.getElementById('confirmDialog');
    const confirmBtn = document.getElementById('confirmDialogConfirm');
    const cancelBtn = document.getElementById('confirmDialogCancel');

    if (!overlay || !confirmBtn || !cancelBtn) return;

    confirmBtn.addEventListener('click', () => closeConfirmDialog(true));
    cancelBtn.addEventListener('click', () => closeConfirmDialog(false));

    // Close on backdrop click
    overlay.addEventListener('click', (e) => {
        if (e.target === overlay) {
            closeConfirmDialog(false);
        }
    });

    // Close on Escape key
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && overlay.getAttribute('aria-hidden') === 'false') {
            closeConfirmDialog(false);
        }
    });
}

// Make notification and confirm dialog functions globally available
window.showNotification = showNotification;
window.showConfirmDialog = showConfirmDialog;

// ===== Bluetooth Compatibility Check =====

/**
 * Show a message when Web Bluetooth API is not supported
 */
function showBluetoothUnsupportedMessage() {
    const welcomeDiv = document.querySelector('.layout-content.welcome');
    if (!welcomeDiv) return;

    welcomeDiv.innerHTML = `
		<div class="bt-unsupported">
			<div class="bt-unsupported-icon">
				<svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
					<path d="M6.5 6.5l11 11L12 23V1l5.5 5.5-11 11"/>
					<line x1="2" y1="2" x2="22" y2="22" stroke-width="2"/>
				</svg>
			</div>
			<h2 class="bt-unsupported-title">Bluetooth Not Supported</h2>
			<p class="bt-unsupported-message">
				Your browser does not support the Web Bluetooth API, which is required to connect to Megahub devices.
			</p>
			<div class="bt-unsupported-browsers">
				<h3>Supported Browsers</h3>
				<ul>
					<li><span class="browser-supported">Chrome</span> (Desktop & Android)</li>
					<li><span class="browser-supported">Edge</span> (Desktop)</li>
					<li><span class="browser-supported">Opera</span> (Desktop)</li>
					<li><span class="browser-supported">Samsung Internet</span> (Android)</li>
					<li><span class="browser-unsupported">Firefox</span> - Not supported</li>
					<li><span class="browser-unsupported">Safari</span> - Not supported</li>
				</ul>
			</div>
			<a href="https://caniuse.com/web-bluetooth" target="_blank" rel="noopener noreferrer" class="bt-unsupported-link">
				<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
					<circle cx="12" cy="12" r="10"/>
					<line x1="12" y1="16" x2="12" y2="12"/>
					<line x1="12" y1="8" x2="12.01" y2="8"/>
				</svg>
				View browser compatibility details on caniuse.com
			</a>
		</div>
	`;
}

// ─── Editor helpers ───────────────────────────────────────────────────────────

/**
 * Generate Lua code from the current Blockly workspace and update the preview.
 * @returns {string} Generated Lua code
 */
function generateCode() {
    const code = blocklyEditor.generateLUAPreview();
    luaPreview.highlightCode(code);
    return code;
}

/**
 * Run a syntax check on the current Blockly workspace code.
 */
async function syntaxCheck() {
    const luaCode = generateCode();
    if (mode === 'dev') {
        showNotification('info', 'Syntax Check', 'Running in dev mode - no backend available');
        return;
    }
    try {
        const result = await App.syntaxCheck(luaCode);
        if (result.success) {
            showNotification('success', 'Syntax Check Passed', `Parse time: ${result.parseTime}ms`);
        } else {
            showNotification('error', 'Syntax Check Failed', result.errorMessage || 'Unknown error');
        }
    } catch (error) {
        showNotification('error', 'Syntax Check Error', error.message);
    }
}

/**
 * Execute the current Blockly workspace code on the device.
 */
async function executeCode() {
    const luaCode = generateCode();
    if (mode === 'dev') {
        showNotification('info', 'Execute', 'Running in dev mode - no backend available');
        return;
    }
    try {
        const success = await App.executeCode(luaCode);
        uiComponents.clear();
        if (success) {
            showNotification('success', 'Program Started', 'Code is now running on the device');
        } else {
            showNotification('error', 'Execution Failed', 'Could not start program');
        }
    } catch (error) {
        showNotification('error', 'Execution Error', error.message);
    }
}

/**
 * Stop the currently running program on the device.
 */
async function stopCode() {
    if (mode === 'dev') {
        showNotification('info', 'Stop', 'Running in dev mode - no backend available');
        return;
    }
    try {
        await App.stop();
        showNotification('success', 'Program Stopped', 'Execution has been halted');
    } catch (error) {
        showNotification('error', 'Stop Error', error.message);
    }
    blocklyEditor.removeAllProfilingOverlays();
}

/**
 * Save the current workspace (XML and Lua) to the device.
 * @returns {Promise<boolean>} True if save was successful
 */
async function saveWorkspace() {
    try {
        Progress.show();
        const xmlText = blocklyEditor.generateXML();
        await App.saveProjectFile('model.xml', 'application/xml; charset=UTF-8', xmlText);
        const luaCode = generateCode();
        await App.saveProjectFile('program.lua', 'text/x-lua; charset=UTF-8', luaCode);
        console.log('Workspace saved!');
        return true;
    } catch (error) {
        console.error('Error while saving project:', error);
        return false;
    } finally {
        Progress.hide();
    }
}

/**
 * Initialize sidebar accordion behavior.
 * Only one panel can be expanded at a time.
 */
function initSidebarAccordion() {
    const accordion = document.getElementById('sidebarAccordion');
    if (!accordion) return;

    const panels = accordion.querySelectorAll('[data-accordion-panel]');

    // Listen for panel expand events
    panels.forEach((panel) => {
        panel.addEventListener('accordion-expand', () => {
            // Collapse all other panels
            panels.forEach((otherPanel) => {
                if (otherPanel !== panel && otherPanel.collapse) {
                    otherPanel.collapse();
                }
                otherPanel.classList.remove('accordion-expanded');
            });

            // Mark this panel as expanded
            panel.classList.add('accordion-expanded');
        });

        panel.addEventListener('accordion-collapse', () => {
            panel.classList.remove('accordion-expanded');
        });
    });
}

// ─── Application-level CustomEvent routing ────────────────────────────────────
// Components dispatch these events (bubbles: true, composed: true) when they
// need the app to take action. We route them here to App functions.

document.addEventListener(APP_EVENT_PROJECT_OPEN, async (e) => {
    const projectId = e.detail.id;
    try {
        Progress.show();
        blocklyEditor.setLoading(true);
        const xmlContent = await App.openProject(projectId);
        if (xmlContent && !blocklyEditor.loadXML(xmlContent)) {
            console.log('Error loading workspace! Blockly failed to parse?');
        }
    } catch (error) {
        showNotification('error', 'Open Failed', error.message);
    } finally {
        blocklyEditor.setLoading(false);
        Progress.hide();
    }
});

document.addEventListener(APP_EVENT_PROJECT_CREATE, (e) => {
    App.createProject(e.detail.id);
});

document.addEventListener(APP_EVENT_PROJECT_DELETE, async (e) => {
    try {
        Progress.show();
        await App.deleteProject(e.detail.id);
    } catch (error) {
        showNotification('error', 'Delete Failed', error.message);
    } finally {
        Progress.hide();
    }
});

document.addEventListener(APP_EVENT_AUTOSTART_SET, async (e) => {
    try {
        await App.setAutostartProject(e.detail.project);
    } catch (error) {
        showNotification('error', 'Autostart Error', error.message);
    }
});

document.addEventListener(APP_EVENT_BT_DISCOVER, async () => {
    try {
        await App.startBluetoothDiscovery();
    } catch (error) {
        showNotification('error', 'Discovery Error', error.message);
    }
});

document.addEventListener(APP_EVENT_BT_PAIR, async (e) => {
    try {
        await App.requestPairing(e.detail.mac);
    } catch (error) {
        showNotification('error', 'Pairing Error', error.message);
    }
});

document.addEventListener(APP_EVENT_BT_UNPAIR, async (e) => {
    try {
        await App.requestRemovePairing(e.detail.mac);
    } catch (error) {
        showNotification('error', 'Unpair Error', error.message);
    }
});

// ─── DOMContentLoaded ─────────────────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {
    blocklyEditor = document.getElementById('blockly');
    blocklyEditor.addChangeListener(() => {
        generateCode();
    });
    luaPreview = document.getElementById('luapreview');
    uiComponents = document.getElementById('uicomponents');
    mapComponent = document.getElementById('mapcomponent');

    // Wire up the save function to the autosave module
    setSaveFunction(saveWorkspace);

    // Subscribe to activeProject state to update the header breadcrumb label
    subscribe('activeProject', (projectId) => {
        const nameEl = document.getElementById('headerProjectName');
        if (nameEl) nameEl.textContent = projectId || '';
    });

    // Initialize accordion behavior
    initSidebarAccordion();

    // ===== Drag-and-Drop File Support =====
    let dragCounter = 0; // Track nested drag events

    blocklyEditor.addEventListener('dragenter', (e) => {
        e.preventDefault();
        dragCounter++;
        if (dragCounter === 1) {
            blocklyEditor.classList.add('drag-over');
        }
    });

    blocklyEditor.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
    });

    blocklyEditor.addEventListener('dragleave', (e) => {
        e.preventDefault();
        dragCounter--;
        if (dragCounter === 0) {
            blocklyEditor.classList.remove('drag-over');
        }
    });

    blocklyEditor.addEventListener('drop', async (e) => {
        e.preventDefault();
        dragCounter = 0;
        blocklyEditor.classList.remove('drag-over');

        const files = e.dataTransfer.files;
        if (files.length === 0) {
            showNotification('warning', 'No File', 'No file was dropped');
            return;
        }

        const file = files[0];

        // Validate file extension
        if (!file.name.toLowerCase().endsWith('.xml')) {
            showNotification('error', 'Invalid File Type', 'Only XML files are supported. Please drop a .xml file.');
            return;
        }

        try {
            const fileContent = await file.text();

            // Validate XML content (same as paste handler)
            if (!fileContent.trim().startsWith('<xml') && !fileContent.trim().startsWith('<?xml')) {
                showNotification('error', 'Invalid XML', 'File does not contain valid Blockly XML');
                return;
            }

            // Confirm before loading
            const confirmed = await showConfirmDialog(
                'Load Workspace',
                `This will replace your current workspace with ${file.name}. Continue?`,
                { confirmText: 'Load', cancelText: 'Cancel', destructive: true }
            );

            if (confirmed) {
                const success = blocklyEditor.loadXML(fileContent);
                if (success) {
                    showNotification('success', 'Workspace Loaded', `${file.name} has been loaded successfully`);
                } else {
                    showNotification('error', 'Load Failed', 'Could not parse the Blockly XML from file');
                }
            }
        } catch (error) {
            console.error('Failed to read dropped file:', error);
            showNotification('error', 'Read Failed', 'Could not read the dropped file');
        }
    });

    if (mode === 'bt') {
        App.setInitState();

        // Check for Web Bluetooth API support
        if (!navigator.bluetooth) {
            // Hide the connect button and show unsupported message
            const btconnectBtn = document.getElementById('btconnect');
            btconnectBtn.style.display = 'none';

            // Show the Bluetooth unsupported view
            showBluetoothUnsupportedMessage();
        } else {
            document.getElementById('btconnect').addEventListener('click', () => {
                const logger = document.getElementById('logger');
                const uiEl = document.getElementById('uicomponents');
                const blocklyEl = document.getElementById('blockly');
                initBLEConnection({ logger, uiComponents: uiEl, mapComponent, blocklyEditor: blocklyEl });
            });
        }
    } else {
        // web / dev modes: jump directly to project management
        Progress.show();
        App.jumpToFilesView().finally(() => Progress.hide());
    }

    if (mode === 'web') {
        const logger = document.getElementById('logger');
        const eventSource = new EventSource('/events');

        eventSource.onerror = (event) => {
            console.error('EventSource failed:', event);
        };
        eventSource.addEventListener('log', (event) => {
            logger.addToLog(JSON.parse(event.data).message);
        });
        eventSource.addEventListener('command', (event) => {
            const cmd = JSON.parse(event.data);
            if (cmd.type === 'map_points' || cmd.type === 'map_clear') {
                mapComponent.processUIEvent(cmd);
            } else {
                uiComponents.processUIEvent(cmd);
            }
        });
        eventSource.addEventListener('portstatus', (event) => {
            // Update via state so portstatus component re-renders automatically
            const data = JSON.parse(event.data);
            setState({ portStatuses: data.ports !== undefined ? data.ports : data });
        });
    }

    if (mode === 'dev') {
        // Initialize BT Device List with dummy data via state
        setState({
            deviceList: {
                devices: [
                    {
                        mac: 'AA:BB:CC:11:22:33',
                        name: '8BitDo M30 gamepad',
                        type: 0x09, // GAMEPAD
                        paired: true,
                        rssi: -45,
                        cod: 0x002508,
                    },
                    {
                        mac: 'DD:EE:FF:44:55:66',
                        name: 'Logitech MX Master 3',
                        type: 0x0b, // MOUSE
                        paired: false,
                        rssi: -67,
                        cod: 0x002580,
                    },
                    {
                        mac: '11:22:33:AA:BB:CC',
                        name: 'Apple Magic Keyboard',
                        type: 0x0a, // KEYBOARD
                        paired: true,
                        rssi: -52,
                        cod: 0x002540,
                    },
                    {
                        mac: '77:88:99:DD:EE:FF',
                        name: 'Unknown Device',
                        type: 0x00, // UNKNOWN
                        paired: false,
                        rssi: -78,
                        cod: 0x000000,
                    },
                    {
                        mac: '44:55:66:11:22:33',
                        name: 'Sony WH-1000XM4',
                        type: 0x03, // AUDIO
                        paired: true,
                        rssi: -58,
                        cod: 0x240404,
                    },
                ],
                count: 5,
                discoveryActive: true,
            },
        });

        // Initialize port status with dummy data via state
        setState({
            portStatuses: [
                {
                    id: 1,
                    connected: false,
                },
                {
                    id: 2,
                    connected: false,
                },
                {
                    id: 3,
                    connected: true,
                    device: {
                        type: 'BOOST Color and Distance Sensor',
                        icon: '⚙️',
                        modes: [
                            {
                                id: '0',
                                name: 'COLOR',
                            },
                        ],
                    },
                },
                {
                    id: 4,
                    connected: false,
                },
            ],
        });
    }

    // Initialize confirmation dialog
    initConfirmDialog();

    document.getElementById('backToProjects').addEventListener('click', () => {
        Progress.show();
        App.jumpToFilesView().finally(() => Progress.hide());
    });

    document.getElementById('reset').addEventListener('click', async () => {
        const confirmed = await showConfirmDialog(
            'Reset Workspace',
            'Are you sure you want to reset the workspace? All unsaved changes to your Blockly program will be lost.',
            { confirmText: 'Reset', cancelText: 'Cancel', destructive: true }
        );
        if (confirmed) {
            blocklyEditor.clearWorkspace();
            showNotification('success', 'Workspace Reset', 'All blocks have been cleared');
        }
    });

    document.getElementById('syntaxcheck').addEventListener('click', () => {
        syntaxCheck();
    });

    document.getElementById('execute').addEventListener('click', () => {
        executeCode();
    });

    document.getElementById('stop').addEventListener('click', () => {
        stopCode();
    });

    // Manual save button
    document.getElementById('save').addEventListener('click', async () => {
        if (getState('activeProject')) {
            const btn = document.getElementById('save');
            btn.disabled = true;
            btn.classList.add('btn-loading');
            try {
                const success = await saveWorkspace();
                if (success) {
                    showNotification('success', 'Saved', 'Workspace saved successfully');
                } else {
                    showNotification('error', 'Save Failed', 'Could not save workspace');
                }
            } finally {
                btn.disabled = false;
                btn.classList.remove('btn-loading');
            }
        } else {
            showNotification('warning', 'No Project', 'Open or create a project first');
        }
    });

    // Copy project to clipboard button
    document.getElementById('copyProject').addEventListener('click', async () => {
        try {
            const xmlText = blocklyEditor.generateXML();
            await navigator.clipboard.writeText(xmlText);
            showNotification('success', 'Copied to Clipboard', 'Blockly project has been copied to clipboard');
        } catch (error) {
            console.error('Failed to copy to clipboard:', error);
            showNotification('error', 'Copy Failed', 'Could not copy project to clipboard');
        }
    });

    // Paste project from clipboard button
    document.getElementById('pasteProject').addEventListener('click', async () => {
        try {
            const clipboardText = await navigator.clipboard.readText();

            // Check if it looks like valid Blockly XML
            if (!clipboardText.trim().startsWith('<xml') && !clipboardText.trim().startsWith('<?xml')) {
                showNotification('error', 'Invalid Data', 'Clipboard does not contain valid Blockly XML');
                return;
            }

            const confirmed = await showConfirmDialog(
                'Paste Project',
                'This will replace your current workspace with the project from clipboard. Continue?',
                { confirmText: 'Paste', cancelText: 'Cancel', destructive: true }
            );

            if (confirmed) {
                const success = blocklyEditor.loadXML(clipboardText);
                if (success) {
                    showNotification('success', 'Project Loaded', 'Workspace has been loaded from clipboard');
                } else {
                    showNotification('error', 'Load Failed', 'Could not parse the Blockly XML from clipboard');
                }
            }
        } catch (error) {
            console.error('Failed to read from clipboard:', error);
            showNotification(
                'error',
                'Paste Failed',
                'Could not read from clipboard. Make sure you have granted clipboard permissions.'
            );
        }
    });

    // Auto-save toggle button
    const autosaveBtn = document.getElementById('autosave');
    autosaveBtn.addEventListener('click', () => {
        if (isAutosaveEnabled()) {
            disableAutosave();
            autosaveBtn.setAttribute('aria-pressed', 'false');
            autosaveBtn.setAttribute('title', 'Auto-save (off)');
            showNotification('info', 'Auto-save Disabled', 'Automatic saving has been turned off');
        } else {
            enableAutosave();
            autosaveBtn.setAttribute('aria-pressed', 'true');
            autosaveBtn.setAttribute('title', 'Auto-save (on)');
            showNotification('success', 'Auto-save Enabled', 'Workspace will be saved every 10 seconds');
        }
    });
});
