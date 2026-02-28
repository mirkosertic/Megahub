/**
 * Vitest global test setup.
 *
 * Runs before every test file. Resets the shared application state so
 * tests are fully isolated from each other — no test can pollute the
 * state observed by a later test.
 */
import { resetState } from '../src/app/state.js';

beforeEach(() => {
    resetState();
});
