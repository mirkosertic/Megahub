import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_dr_reset.js';

function makeBlock(extra = {}) {
    return { id: 'test-id', ...extra };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_dr_reset block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_dr_reset"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_dr_reset');
    });

    it('is a statement block (previousStatement + nextStatement)', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('has no output', () => {
        expect(definition.blockdefinition.output).toBeUndefined();
    });
});

describe('mh_alg_dr_reset generator', () => {
    it('generates alg.drReset with provided handle', () => {
        const block = makeBlock();
        const gen = makeGenerator({ HANDLE: 'myRobot' });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.drReset(myRobot)\n');
    });

    it('falls back to empty string handle', () => {
        const block = makeBlock();
        const gen = makeGenerator({});
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.drReset("")\n');
    });

    it('returns a string ending with newline', () => {
        const result = definition.generator(makeBlock(), makeGenerator({}));
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
