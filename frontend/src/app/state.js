/**
 * Observable application state module.
 *
 * Provides a shared state store that components can subscribe to.
 * When setState() is called, all subscribers for the changed keys
 * are notified synchronously with the new value.
 *
 * Usage in a component:
 *   import { subscribe, getState } from '../../app/state.js';
 *
 *   connectedCallback() {
 *     this._unsubPorts = subscribe('portStatuses', ports => this.render(ports));
 *   }
 *   disconnectedCallback() {
 *     this._unsubPorts?.();
 *   }
 */

/** @type {{ portStatuses: Array, deviceList: Array, isConnected: boolean, activeProject: string|null, projects: Array, autostartProject: string|null }} */
const _state = {
    portStatuses:    [],
    deviceList:      [],
    isConnected:     false,
    activeProject:   null,
    projects:        [],
    autostartProject: null,
};

/** @type {Map<string, Set<function>>} */
const _listeners = new Map();

/**
 * Read a state value by key.
 * @param {string} key
 * @returns {*}
 */
export function getState(key) {
    return _state[key];
}

/**
 * Update one or more state keys and notify subscribers.
 * Only keys present in the patch trigger notifications.
 * @param {Partial<typeof _state>} patch
 */
export function setState(patch) {
    Object.assign(_state, patch);
    for (const key of Object.keys(patch)) {
        _listeners.get(key)?.forEach(fn => fn(_state[key]));
    }
}

/**
 * Subscribe to changes for a single state key.
 * The callback is invoked with the new value whenever setState() updates that key.
 *
 * @param {string} key - State key to watch
 * @param {function(*): void} fn - Callback receiving the new value
 * @returns {function(): void} Unsubscribe function — call it in disconnectedCallback
 */
export function subscribe(key, fn) {
    if (!_listeners.has(key)) _listeners.set(key, new Set());
    _listeners.get(key).add(fn);
    return () => _listeners.get(key).delete(fn);
}

/**
 * Reset all state to initial values and remove all subscribers.
 * Called in test setup (beforeEach) to ensure test isolation.
 */
export function resetState() {
    Object.assign(_state, {
        portStatuses:    [],
        deviceList:      [],
        isConnected:     false,
        activeProject:   null,
        projects:        [],
        autostartProject: null,
    });
    _listeners.clear();
}
