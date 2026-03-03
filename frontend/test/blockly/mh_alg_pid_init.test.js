import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_pid_init.js';

function makeBlock(extra = {}) {
    return {
        id: extra.id ?? 'test-id',
        ...extra,
    };
}

function makeGenerator() {
    return {};
}

describe('mh_alg_pid_init block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_pid_init"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_pid_init');
    });

    it('has output (value block)', () => {
        expect(definition.blockdefinition.output).toBe(null);
    });

    it('has no inputs', () => {
        expect(definition.blockdefinition.args0).toEqual([]);
    });
});

describe('mh_alg_pid_init generator', () => {
    it('generates correct code', () => {
        const block = makeBlock();
        const gen = makeGenerator();
        const [code, prec] = definition.generator(block, gen);
        expect(code).toBe('alg.initPID()');
        expect(prec).toBe(0);
    });

    it('returns array [code, 0]', () => {
        const block = makeBlock();
        const result = definition.generator(block, makeGenerator());
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[1]).toBe(0);
    });
});
