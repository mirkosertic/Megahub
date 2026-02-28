/**
 * Auto-save module.
 *
 * Manages the periodic workspace save timer. The autosave button in the
 * toolbar toggles this on/off. index.js wires up the button click and
 * delegates to start/stop functions here.
 */
import { getState } from './state.js';

let _enabled = false;
let _intervalId = null;

/** @type {function(): Promise<void>} — injected by index.js at startup */
let _saveFn = null;

/**
 * Provide the save function to call on each interval tick.
 * Must be called once during app initialization before the toggle button is used.
 * @param {function(): Promise<void>} fn
 */
export function setSaveFunction(fn) {
    _saveFn = fn;
}

/** @returns {boolean} Whether auto-save is currently active */
export function isEnabled() {
    return _enabled;
}

/**
 * Enable auto-save. Starts a 10-second interval that calls the registered
 * save function whenever a project is open.
 */
export function enable() {
    if (_enabled) return;
    _enabled = true;
    _intervalId = setInterval(() => {
        if (getState('activeProject') && _saveFn) {
            _saveFn();
        }
    }, 10000);
}

/**
 * Disable auto-save and cancel the interval.
 */
export function disable() {
    if (!_enabled) return;
    _enabled = false;
    if (_intervalId) {
        clearInterval(_intervalId);
        _intervalId = null;
    }
}
