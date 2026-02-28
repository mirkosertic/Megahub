/**
 * Tests for the observable state module.
 * These are pure unit tests — no DOM required.
 */
import { getState, setState, subscribe, resetState } from './state.js';

describe('getState', () => {
    it('returns initial value for a key', () => {
        expect(getState('portStatuses')).toEqual([]);
        expect(getState('isConnected')).toBe(false);
        expect(getState('activeProject')).toBeNull();
    });
});

describe('setState', () => {
    it('updates a single key', () => {
        setState({ isConnected: true });
        expect(getState('isConnected')).toBe(true);
    });

    it('updates multiple keys at once', () => {
        setState({ isConnected: true, activeProject: 'my-project' });
        expect(getState('isConnected')).toBe(true);
        expect(getState('activeProject')).toBe('my-project');
    });

    it('does not affect keys not in the patch', () => {
        setState({ activeProject: 'foo' });
        // portStatuses should still be at its initial/reset value
        expect(getState('portStatuses')).toEqual([]);
    });
});

describe('subscribe', () => {
    it('calls the subscriber when the watched key changes', () => {
        const fn = vi.fn();
        subscribe('isConnected', fn);
        setState({ isConnected: true });
        expect(fn).toHaveBeenCalledOnce();
        expect(fn).toHaveBeenCalledWith(true);
    });

    it('does not call the subscriber for other key changes', () => {
        const fn = vi.fn();
        subscribe('portStatuses', fn);
        setState({ isConnected: true });
        expect(fn).not.toHaveBeenCalled();
    });

    it('calls subscriber with each successive value', () => {
        const received = [];
        subscribe('activeProject', v => received.push(v));

        setState({ activeProject: 'project-a' });
        setState({ activeProject: 'project-b' });

        expect(received).toEqual(['project-a', 'project-b']);
    });

    it('returns an unsubscribe function that stops future notifications', () => {
        const fn = vi.fn();
        const unsub = subscribe('activeProject', fn);

        setState({ activeProject: 'first' });
        expect(fn).toHaveBeenCalledOnce();

        unsub();

        setState({ activeProject: 'second' });
        // Still only called once — unsubscribed before the second change
        expect(fn).toHaveBeenCalledOnce();
    });

    it('allows multiple independent subscribers on the same key', () => {
        const fn1 = vi.fn();
        const fn2 = vi.fn();
        subscribe('isConnected', fn1);
        subscribe('isConnected', fn2);

        setState({ isConnected: true });

        expect(fn1).toHaveBeenCalledOnce();
        expect(fn2).toHaveBeenCalledOnce();
    });
});

describe('resetState', () => {
    it('resets all keys to their initial values', () => {
        setState({ isConnected: true, activeProject: 'test', portStatuses: [{ id: 1 }] });
        resetState();

        expect(getState('isConnected')).toBe(false);
        expect(getState('activeProject')).toBeNull();
        expect(getState('portStatuses')).toEqual([]);
    });

    it('clears all subscribers so they are not called after reset', () => {
        const fn = vi.fn();
        subscribe('isConnected', fn);

        resetState();

        setState({ isConnected: true });
        // Subscriber was cleared by resetState, should not be called
        expect(fn).not.toHaveBeenCalled();
    });
});
