import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_dr_get.js';

function makeBlock(fields = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-id',
        ...extra,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_dr_get block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_dr_get"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_dr_get');
    });

    it('has output (expression block)', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('has FIELD dropdown with X, Y, HEADING options', () => {
        const fieldArg = definition.blockdefinition.args0.find((a) => a.name === 'FIELD');
        expect(fieldArg).toBeDefined();
        const optionValues = fieldArg.options.map((o) => o[1]);
        expect(optionValues).toContain('x');
        expect(optionValues).toContain('y');
        expect(optionValues).toContain('heading');
    });
});

describe('mh_alg_dr_get generator', () => {
    it('generates correct code for X field', () => {
        const block = makeBlock({ FIELD: 'x' });
        const gen = makeGenerator({ HANDLE: 'myRobot' });
        const [code, prec] = definition.generator(block, gen);
        expect(code).toBe('alg.drGet(myRobot, "x")');
        expect(prec).toBe(0);
    });

    it('generates correct code for HEADING field', () => {
        const block = makeBlock({ FIELD: 'heading' });
        const gen = makeGenerator({ HANDLE: 'handle' });
        const [code] = definition.generator(block, gen);
        expect(code).toBe('alg.drGet(handle, "heading")');
    });

    it('uses empty string handle as default', () => {
        const block = makeBlock({ FIELD: 'y' });
        const gen = makeGenerator({});
        const [code] = definition.generator(block, gen);
        expect(code).toBe('alg.drGet("", "y")');
    });

    it('returns array [code, 0]', () => {
        const block = makeBlock({ FIELD: 'x' });
        const result = definition.generator(block, makeGenerator({}));
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[1]).toBe(0);
    });
});
