/**
 * Shared BLEClient singleton.
 *
 * Both app.js and connection.js import from here to ensure they operate
 * on the same BLE connection instance.
 */
import { BLEClient } from '../bleclient.js';

export const bleClient = new BLEClient();
