/**
 * Unit tests for the lego_get_device_mode Blockly block generator.
 *
 * This is an EXPRESSION block: the generator returns [code, order], not a string.
 * No Blockly import needed — we mock the minimal API surface.
 */
import { definition } from '../../src/components/blockly/lego_get_device_mode.js';

function makeBlock(fields = {}) {
    return {
        getFieldValue: (key) => fields[key] ?? null,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (_block, name) => values[name] ?? '0',
    };
}

describe('lego_get_device_mode block definition', () => {
    it('has category "LEGO©"', () => {
        expect(definition.category).toBe('LEGO©');
    });

    it('has type "lego_get_device_mode"', () => {
        expect(definition.blockdefinition.type).toBe('lego_get_device_mode');
    });

    it('is an expression block (output is defined)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });

    it('is NOT a statement block (no previousStatement/nextStatement)', () => {
        expect(definition.blockdefinition.previousStatement).toBeUndefined();
        expect(definition.blockdefinition.nextStatement).toBeUndefined();
    });

    it('has PORT input in inputsForToolbox with mh_port shadow', () => {
        expect(definition.inputsForToolbox.PORT).toBeDefined();
        expect(definition.inputsForToolbox.PORT.shadow.type).toBe('mh_port');
    });
});

describe('lego_get_device_mode generator', () => {
    it('returns an array [code, order] — not a string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1' });

        const result = definition.generator(block, gen);

        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
    });

    it('generates lego.getdevicemode(port) expression', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1' });

        const [code] = definition.generator(block, gen);

        expect(code).toContain('lego.getdevicemode(');
        expect(code).toContain('PORT1');
    });

    it('has operator precedence 0 in the returned pair', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT2' });

        const [, order] = definition.generator(block, gen);

        expect(order).toBe(0);
    });

    it('produces exactly lego.getdevicemode(p) as the code string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'p' });

        const [code, order] = definition.generator(block, gen);

        expect(code).toBe('lego.getdevicemode(p)');
        expect(order).toBe(0);
    });
});
