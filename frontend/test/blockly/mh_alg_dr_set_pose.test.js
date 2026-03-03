import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_dr_set_pose.js';

function makeBlock(extra = {}) {
    return { id: 'test-id', ...extra };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_dr_set_pose block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_dr_set_pose"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_dr_set_pose');
    });

    it('is a statement block', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('has toolbox shadows for X, Y, HEADING', () => {
        expect(definition.inputsForToolbox).toHaveProperty('X');
        expect(definition.inputsForToolbox).toHaveProperty('Y');
        expect(definition.inputsForToolbox).toHaveProperty('HEADING');
    });
});

describe('mh_alg_dr_set_pose generator', () => {
    it('generates correct drSetPose call', () => {
        const block = makeBlock();
        const gen = makeGenerator({ HANDLE: 'myRobot', X: '0.30', Y: '0.0', HEADING: '0.0' });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.drSetPose(myRobot, 0.30, 0.0, 0.0)\n');
    });

    it('uses defaults when inputs are empty', () => {
        const result = definition.generator(makeBlock(), makeGenerator({}));
        expect(result).toBe('alg.drSetPose("", 0, 0, 0)\n');
    });

    it('returns a string ending with newline', () => {
        const result = definition.generator(makeBlock(), makeGenerator({}));
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
