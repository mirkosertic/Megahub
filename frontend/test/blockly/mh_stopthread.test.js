/**
 * Unit tests for the mh_stopthread Blockly block generator.
 *
 * The generator is a pure function: (block, generator) => Lua code string.
 * No Blockly import needed — we mock the minimal API surface.
 */
import { definition } from '../../src/components/blockly/mh_stopthread.js';

function makeBlock(fields = {}) {
    return {
        getFieldValue: key => fields[key] ?? null,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (_block, name, _order) => values[name] ?? '0',
    };
}

describe('mh_stopthread block definition', () => {
    it('has category "Control flow"', () => {
        expect(definition.category).toBe('Control flow');
    });

    it('has type "mh_stopthread"', () => {
        expect(definition.blockdefinition.type).toBe('mh_stopthread');
    });

    it('has a tooltip', () => {
        expect(typeof definition.blockdefinition.tooltip).toBe('string');
        expect(definition.blockdefinition.tooltip.length).toBeGreaterThan(0);
    });

    it('is a statement block (previousStatement and nextStatement)', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });
});

describe('mh_stopthread generator', () => {
    it('generates hub.stopthread(handle) call', () => {
        const block = makeBlock();
        const gen   = makeGenerator({ HANDLE: 'myHandle' });

        const code = definition.generator(block, gen);

        expect(code).toContain('hub.stopthread(');
        expect(code).toContain('myHandle');
    });

    it('produces a statement ending with newline', () => {
        const block = makeBlock();
        const gen   = makeGenerator({ HANDLE: 'h' });

        const code = definition.generator(block, gen);

        expect(code).toMatch(/\n$/);
    });

    it('uses 0 as default handle when input is missing', () => {
        const block = makeBlock();
        const gen   = makeGenerator({});  // no HANDLE

        const code = definition.generator(block, gen);

        expect(code).toBe('hub.stopthread(0)\n');
    });

    it('produces exactly hub.stopthread(handle)\\n', () => {
        const block = makeBlock();
        const gen   = makeGenerator({ HANDLE: 'h' });

        const code = definition.generator(block, gen);

        expect(code).toBe('hub.stopthread(h)\n');
    });
});
