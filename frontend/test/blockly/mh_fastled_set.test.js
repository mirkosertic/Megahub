import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_fastled_set.js';

function makeBlock(fields = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-block-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra,
    };
}

function makeGenerator(values = {}, statements = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
        statementToCode: (block, name) => statements[name] ?? '',
    };
}

describe('mh_fastled_set block definition', () => {
    it('has category "FastLED"', () => {
        expect(definition.category).toBe('FastLED');
    });

    it('has type "mh_fastled_set"', () => {
        expect(definition.blockdefinition.type).toBe('mh_fastled_set');
    });

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });
});

describe('mh_fastled_set generator', () => {
    it('generates correct code for green #00ff00', () => {
        const block = makeBlock({ COLOR: '#00ff00' });
        const gen = makeGenerator({ INDEX: '1' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.set(1 ,0, 255,0)\n');
    });

    it('generates correct code for red #ff0000', () => {
        const block = makeBlock({ COLOR: '#ff0000' });
        const gen = makeGenerator({ INDEX: '0' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.set(0 ,255, 0,0)\n');
    });

    it('generates correct code for blue #0000ff', () => {
        const block = makeBlock({ COLOR: '#0000ff' });
        const gen = makeGenerator({ INDEX: '2' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.set(2 ,0, 0,255)\n');
    });

    it('generates correct code for white #ffffff', () => {
        const block = makeBlock({ COLOR: '#ffffff' });
        const gen = makeGenerator({ INDEX: '0' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.set(0 ,255, 255,255)\n');
    });

    it('generates correct code for black #000000', () => {
        const block = makeBlock({ COLOR: '#000000' });
        const gen = makeGenerator({ INDEX: '0' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.set(0 ,0, 0,0)\n');
    });
});
