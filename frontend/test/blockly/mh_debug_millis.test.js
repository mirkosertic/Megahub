import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_debug_millis.js';

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

describe('mh_debug_millis block definition', () => {
    it('has category "Debug"', () => {
        expect(definition.category).toBe('Debug');
    });

    it('has type "mh_debug_millis"', () => {
        expect(definition.blockdefinition.type).toBe('mh_debug_millis');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });
});

describe('mh_debug_millis generator', () => {
    it('returns ["millis()", 0]', () => {
        const block = makeBlock();
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(Array.isArray(result)).toBe(true);
        expect(result[0]).toBe('millis()');
        expect(result[1]).toBe(0);
    });
});
