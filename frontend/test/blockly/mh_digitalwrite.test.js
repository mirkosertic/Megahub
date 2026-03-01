import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_digitalwrite.js';

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

describe('mh_digitalwrite block definition', () => {
    it('has category "I/O"', () => {
        expect(definition.category).toBe('I/O');
    });

    it('has type "mh_digitalwrite"', () => {
        expect(definition.blockdefinition.type).toBe('mh_digitalwrite');
    });

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });
});

describe('mh_digitalwrite generator', () => {
    it('generates "hub.digitalWrite(4,1)\\n" for PIN=4, VALUE=1', () => {
        const block = makeBlock({ PIN: '4' });
        const gen = makeGenerator({ VALUE: '1' });
        const code = definition.generator(block, gen);
        expect(code).toBe('hub.digitalWrite(4,1)\n');
    });

    it('embeds pin and value in output', () => {
        const block = makeBlock({ PIN: 'GPIO13' });
        const gen = makeGenerator({ VALUE: '0' });
        const code = definition.generator(block, gen);
        expect(code).toContain('GPIO13');
        expect(code).toContain('0');
    });

    it('ends with newline', () => {
        const block = makeBlock({ PIN: '4' });
        const gen = makeGenerator({ VALUE: '1' });
        const code = definition.generator(block, gen);
        expect(code).toMatch(/\n$/);
    });
});
