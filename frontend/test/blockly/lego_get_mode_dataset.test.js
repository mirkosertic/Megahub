/**
 * Unit tests for the lego_get_mode_dataset Blockly block generator.
 *
 * This is an EXPRESSION block: the generator returns [code, order], not a string.
 * No Blockly import needed — we mock the minimal API surface.
 */
import { definition } from '../../src/components/blockly/lego_get_mode_dataset.js';

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

describe('lego_get_mode_dataset block definition', () => {
    it('has category "LEGO©"', () => {
        expect(definition.category).toBe('LEGO©');
    });

    it('has type "lego_get_mode_dataset"', () => {
        expect(definition.blockdefinition.type).toBe('lego_get_mode_dataset');
    });

    it('is an expression block (output is defined)', () => {
        // output: null means "any type" in Blockly — it's still an expression block
        expect('output' in definition.blockdefinition).toBe(true);
    });

    it('is NOT a statement block (no previousStatement/nextStatement)', () => {
        expect(definition.blockdefinition.previousStatement).toBeUndefined();
        expect(definition.blockdefinition.nextStatement).toBeUndefined();
    });

    it('has MODE input in inputsForToolbox', () => {
        expect(definition.inputsForToolbox.MODE).toBeDefined();
        expect(definition.inputsForToolbox.MODE.shadow.type).toBe('lego_get_device_mode');
    });
});

describe('lego_get_mode_dataset generator', () => {
    it('returns an array [code, order] — not a string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1', MODE: '0', DATASET: '0' });

        const result = definition.generator(block, gen);

        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
    });

    it('generates lego.getmodedataset(port, mode, dataset) expression', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1', MODE: '0', DATASET: '0' });

        const [code] = definition.generator(block, gen);

        expect(code).toContain('lego.getmodedataset(');
        expect(code).toContain('PORT1');
        expect(code).toContain('0');
    });

    it('has operator precedence 0 in the returned pair', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT2', MODE: '8', DATASET: '1' });

        const [, order] = definition.generator(block, gen);

        expect(order).toBe(0);
    });

    it('embeds PORT, MODE and DATASET in the code', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'myPort', MODE: 'myMode', DATASET: 'idx + 1' });

        const [code] = definition.generator(block, gen);

        expect(code).toContain('myPort');
        expect(code).toContain('myMode');
        expect(code).toContain('idx + 1');
    });

    it('produces exactly lego.getmodedataset(p,m,d) as the code string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'p', MODE: 'm', DATASET: 'd' });

        const [code, order] = definition.generator(block, gen);

        expect(code).toBe('lego.getmodedataset(p,m,d)');
        expect(order).toBe(0);
    });
});
