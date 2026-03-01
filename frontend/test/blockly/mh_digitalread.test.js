import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_digitalread.js';

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

describe('mh_digitalread block definition', () => {
    it('has category "I/O"', () => {
        expect(definition.category).toBe('I/O');
    });

    it('has type "mh_digitalread"', () => {
        expect(definition.blockdefinition.type).toBe('mh_digitalread');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });
});

describe('mh_digitalread generator', () => {
    it('generates ["hub.digitalRead(4)", 0] for PIN=4', () => {
        const block = makeBlock({ PIN: '4' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('hub.digitalRead(4)');
        expect(result[1]).toBe(0);
    });

    it('embeds pin value in output', () => {
        const block = makeBlock({ PIN: 'GPIO13' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toContain('GPIO13');
    });

    it('returns an array [code, 0]', () => {
        const block = makeBlock({ PIN: '4' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(Array.isArray(result)).toBe(true);
        expect(result[1]).toBe(0);
    });
});
