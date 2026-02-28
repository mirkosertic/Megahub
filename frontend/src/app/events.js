/**
 * Application-level CustomEvent names.
 *
 * Components dispatch these events (bubbles: true, composed: true) when they
 * need the application to take action. index.js listens on document and
 * routes them to the appropriate App function.
 *
 * This decouples components from the App module — components never import App.
 */

// Project management
export const APP_EVENT_PROJECT_OPEN   = 'app:project:open';    // detail: { id }
export const APP_EVENT_PROJECT_CREATE = 'app:project:create';  // detail: { id }
export const APP_EVENT_PROJECT_DELETE = 'app:project:delete';  // detail: { id }
export const APP_EVENT_AUTOSTART_SET  = 'app:autostart:set';   // detail: { project }

// Bluetooth Classic device management
export const APP_EVENT_BT_DISCOVER = 'app:bt:discover';  // detail: {}
export const APP_EVENT_BT_PAIR     = 'app:bt:pair';      // detail: { mac }
export const APP_EVENT_BT_UNPAIR   = 'app:bt:unpair';    // detail: { mac }
