/**
 * Application module — project management, code execution, and device communication.
 *
 * All methods are exported as named functions. No globals are set here.
 * The build mode (bt / web / dev) is read from import.meta.env.VITE_MODE.
 *
 * Communication flow:
 *   index.js event handler → App.someMethod() → BLE/HTTP → setState()
 *
 * Components must NOT import from this module. They communicate upward
 * using CustomEvents (see events.js), and receive state updates via state.js.
 */
import { bleClient } from './ble-instance.js';
import { setState, getState } from './state.js';
import {
    APP_REQUEST_TYPE_GET_AUTOSTART,
    APP_REQUEST_TYPE_GET_PROJECTS,
    APP_REQUEST_TYPE_GET_PROJECT_FILE,
    APP_REQUEST_TYPE_PUT_AUTOSTART,
    APP_REQUEST_TYPE_SYNTAX_CHECK,
    APP_REQUEST_TYPE_RUN_PROGRAM,
    APP_REQUEST_TYPE_STOP_PROGRAM,
    APP_REQUEST_TYPE_DELETE_PROJECT,
    APP_REQUEST_TYPE_REQUEST_PAIRING,
    APP_REQUEST_TYPE_REMOVE_PAIRING,
    APP_REQUEST_TYPE_START_DISCOVERY,
} from '../bleclient.js';

const mode = import.meta.env.VITE_MODE;

// localStorage key for dev-mode workspace persistence
const STORAGE_KEY = 'blockly_robot_workspace';

/**
 * Switch the UI to the named display mode.
 * Hides/shows elements with class `dynamicvisibility` based on their
 * `visible-<mode>` class.
 * @param {'btconnect'|'management'|'editor'} newMode
 */
export function setMode(newMode) {
    document.body.dataset.mode = newMode;
    for (const item of document.querySelectorAll('.dynamicvisibility')) {
        item.style.display = 'none';
        item.style.visibility = 'hidden';
        if (item.classList.contains('visible-' + newMode)) {
            item.style.display = 'block';
            item.style.visibility = 'visible';
        }
    }
}

/** Switch UI to the initial BLE connect screen. */
export function setInitState() {
    setMode('btconnect');
}

/**
 * Navigate to the project management view.
 * Loads the project list and autostart config, then updates state so
 * the files component re-renders automatically.
 *
 * Callers should show/hide progress indicators around this call.
 */
export async function jumpToFilesView() {
    setMode('management');
    const { projects, autostart } = await _loadProjectsAndAutostart();
    setState({ projects, autostartProject: autostart });
}

/**
 * Create a new empty project and open the editor.
 * @param {string} projectId
 */
export function createProject(projectId) {
    console.log('Creating new empty project');
    setMode('editor');
    setState({ activeProject: projectId });
}

/**
 * Open an existing project and load its Blockly workspace.
 * Returns the XML content so the caller can pass it to the Blockly editor.
 * @param {string} projectId
 * @returns {Promise<string>} XML workspace content
 */
export async function openProject(projectId) {
    console.log('Opening project ' + projectId);
    setMode('editor');
    setState({ activeProject: projectId });
    return await requestProjectFile(projectId, 'model.xml');
}

/**
 * Permanently delete a project and refresh the project list.
 * @param {string} project - Project name
 */
export async function deleteProject(project) {
    if (mode === 'bt') {
        console.log('Deleting project ' + project);
        await bleClient.sendRequest(APP_REQUEST_TYPE_DELETE_PROJECT, JSON.stringify({ project }));
    } else if (mode === 'web') {
        await fetch('/project/' + encodeURIComponent(project), { method: 'DELETE' });
    }
    await jumpToFilesView();
}

/**
 * Persist the autostart project selection to the device.
 * @param {string} project - Project name, or null to clear
 */
export async function setAutostartProject(project) {
    if (mode === 'bt') {
        await bleClient.sendRequest(APP_REQUEST_TYPE_PUT_AUTOSTART, JSON.stringify({ project }));
    } else if (mode === 'web') {
        await fetch('/autostart', {
            method: 'PUT',
            body: JSON.stringify({ project }),
            headers: { 'Content-type': 'application/json; charset=UTF-8' },
        });
    }
}

/**
 * Fetch a single project file from the device.
 * @param {string} projectId
 * @param {string} filename
 * @returns {Promise<string>} File content as text
 */
export async function requestProjectFile(projectId, filename) {
    console.log('Requesting file ' + filename + ' of project ' + projectId);
    if (mode === 'dev') {
        if (filename === 'model.xml') {
            return localStorage.getItem(STORAGE_KEY) ?? '';
        }
        return '';
    } else if (mode === 'bt') {
        const response = await bleClient.sendRequest(
            APP_REQUEST_TYPE_GET_PROJECT_FILE,
            JSON.stringify({ project: projectId, filename })
        );
        return new TextDecoder().decode(response);
    } else if (mode === 'web') {
        const response = await fetch(`/project/${encodeURIComponent(projectId)}/${filename}`);
        return response.text();
    }
    return '';
}

/**
 * Save a file to the active project on the device.
 * @param {string} filename
 * @param {string} contentType - MIME type
 * @param {string} content
 */
export async function saveProjectFile(filename, contentType, content) {
    const projectId = getState('activeProject');
    if (mode === 'dev') {
        if (filename === 'model.xml') localStorage.setItem(STORAGE_KEY, content);
    } else if (mode === 'bt') {
        try {
            // Use streaming protocol for memory-efficient uploads
            await bleClient.uploadFileStreaming(projectId, filename, content);
        } catch (error) {
            console.error('Upload failed:', error.message);
            window.showNotification('error', 'Upload Failed', `Could not save ${filename}: ${error.message}`);
            throw error; // Re-throw to let caller handle it
        }
    } else if (mode === 'web') {
        await fetch(`/project/${encodeURIComponent(projectId)}/${filename}`, {
            method: 'PUT',
            body: content,
            headers: { 'Content-type': contentType },
        });
    }
}

/**
 * Run a Lua syntax check on the device.
 * @param {string} luaCode
 * @returns {Promise<{ success: boolean, parseTime?: number, errorMessage?: string }>}
 */
export async function syntaxCheck(luaCode) {
    if (mode === 'dev') {
        return { success: true, parseTime: 0 };
    } else if (mode === 'bt') {
        const response = await bleClient.sendRequest(APP_REQUEST_TYPE_SYNTAX_CHECK, JSON.stringify({ luaScript: luaCode }));
        return JSON.parse(new TextDecoder().decode(response));
    } else if (mode === 'web') {
        const response = await fetch('/syntaxcheck', {
            method: 'PUT',
            body: luaCode,
            headers: { 'Content-type': 'text/x-lua; charset=UTF-8' },
        });
        return response.json();
    }
    return { success: false };
}

/**
 * Upload and execute a Lua program on the device.
 * @param {string} luaCode
 * @returns {Promise<boolean>} True if the program started successfully
 */
export async function executeCode(luaCode) {
    if (mode === 'dev') {
        return true;
    } else if (mode === 'bt') {
        const response = await bleClient.sendRequest(APP_REQUEST_TYPE_RUN_PROGRAM, JSON.stringify({ luaScript: luaCode }));
        return JSON.parse(new TextDecoder().decode(response)).result;
    } else if (mode === 'web') {
        await fetch('/execute', {
            method: 'PUT',
            body: luaCode,
            headers: { 'Content-type': 'text/x-lua; charset=UTF-8' },
        });
        return true;
    }
    return false;
}

/**
 * Stop the currently running Lua program on the device.
 */
export async function stop() {
    if (mode === 'dev') {
        return;
    } else if (mode === 'bt') {
        await bleClient.sendRequest(APP_REQUEST_TYPE_STOP_PROGRAM, JSON.stringify({}));
    } else if (mode === 'web') {
        await fetch('/stop', {
            method: 'PUT',
            body: '',
            headers: { 'Content-type': 'text/x-lua; charset=UTF-8' },
        });
    }
}

/**
 * Initiate Bluetooth Classic pairing with the given device.
 * @param {string} mac - Device MAC address
 */
export async function requestPairing(mac) {
    if (mode === 'dev') {
        window.showNotification('info', 'Pairing', `Dev mode - simulating pairing with ${mac}`);
    } else if (mode === 'bt') {
        window.showNotification('info', 'Pairing', `Initiating pairing with ${mac}...`);
        await bleClient.sendRequest(APP_REQUEST_TYPE_REQUEST_PAIRING, JSON.stringify({ mac }));
        window.showNotification('success', 'Pairing Initiated', 'Check your device for pairing confirmation');
    }
}

/**
 * Remove an existing Bluetooth Classic pairing.
 * @param {string} mac - Device MAC address
 */
export async function requestRemovePairing(mac) {
    if (mode === 'dev') {
        window.showNotification('info', 'Remove Pairing', `Dev mode - simulating unpairing ${mac}`);
    } else if (mode === 'bt') {
        await bleClient.sendRequest(APP_REQUEST_TYPE_REMOVE_PAIRING, JSON.stringify({ mac }));
        window.showNotification('success', 'Pairing Removed', `Device ${mac} has been unpaired`);
    }
}

/**
 * Start Bluetooth Classic device discovery scan.
 */
export async function startBluetoothDiscovery() {
    if (mode === 'dev') {
        window.showNotification('info', 'Bluetooth Scan', 'Dev mode - no actual scan');
    } else if (mode === 'bt') {
        window.showNotification('info', 'Scanning', 'Searching for Bluetooth devices...');
        await bleClient.sendRequest(APP_REQUEST_TYPE_START_DISCOVERY, JSON.stringify({}));
    }
}

// ─── Internal helpers ────────────────────────────────────────────────────────

/**
 * Fetch both the project list and the current autostart setting from the device.
 * @returns {Promise<{ projects: Array, autostart: string|null }>}
 */
async function _loadProjectsAndAutostart() {
    let projects = [{ name: 'dummy' }, { name: 'testproject' }];
    let autostart = 'testproject';

    if (mode === 'bt') {
        console.log('Requesting Projects...');
        const pRes = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECTS, JSON.stringify({}));
        const pText = new TextDecoder().decode(pRes);
        console.log('Got Response : (' + pText + ')');
        const pJson = JSON.parse(pText);
        console.log('JSON Response was ' + JSON.stringify(pJson));
        projects = pJson.projects;

        const aRes = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_AUTOSTART, JSON.stringify({}));
        autostart = JSON.parse(new TextDecoder().decode(aRes)).project;
    } else if (mode === 'web') {
        const pRes = await fetch('/projects');
        projects = (await pRes.json()).projects;

        const aRes = await fetch('/autostart');
        autostart = (await aRes.json()).project;
    }

    return { projects, autostart };
}
