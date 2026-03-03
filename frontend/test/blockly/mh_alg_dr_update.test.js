import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_dr_update.js';

function makeBlock(extra = {}) {
    return {
        id: extra.id ?? 'test-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_dr_update block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_dr_update"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_dr_update');
    });

    it('has both previousStatement and nextStatement (statement block)', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('does not have output (pure statement block)', () => {
        expect(definition.blockdefinition.output).toBeUndefined();
    });

    it('has 7 value inputs', () => {
        const valueInputs = definition.blockdefinition.args0.filter((a) => a.type === 'input_value');
        expect(valueInputs.length).toBe(7);
    });

    it('has toolbox shadows for all 7 inputs', () => {
        const keys = Object.keys(definition.inputsForToolbox);
        expect(keys).toContain('HANDLE');
        expect(keys).toContain('LEFT_TICKS');
        expect(keys).toContain('RIGHT_TICKS');
        expect(keys).toContain('YAW');
        expect(keys).toContain('M_PER_TICK');
        expect(keys).toContain('WHEELBASE');
        expect(keys).toContain('IMU_WEIGHT');
    });
});

describe('mh_alg_dr_update generator', () => {
    it('generates correct alg.updateDR call with handle', () => {
        const block = makeBlock();
        const gen = makeGenerator({
            HANDLE: 'myRobot',
            LEFT_TICKS: '100',
            RIGHT_TICKS: '110',
            YAW: '45',
            WHEELBASE: '0.12',
            M_PER_TICK: '0.0005',
            IMU_WEIGHT: '0.8',
        });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.updateDR(myRobot, 100, 110, 45, 0.12, 0.0005, 0.8)\n');
    });

    it('uses "dr_0" default when handle is empty', () => {
        const block = makeBlock();
        const gen = makeGenerator({
            LEFT_TICKS: '100',
            RIGHT_TICKS: '110',
            YAW: '45',
            WHEELBASE: '0.12',
            M_PER_TICK: '0.0005',
            IMU_WEIGHT: '0.8',
        });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.updateDR("dr_0", 100, 110, 45, 0.12, 0.0005, 0.8)\n');
    });

    it('uses defaults for sensor values when valueToCode returns empty string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ HANDLE: 'robot' });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.updateDR(robot, 0, 0, 0, 0.12, 0.0005, 0.8)\n');
    });

    it('always returns a string (statement form only)', () => {
        const block = makeBlock();
        const result = definition.generator(block, makeGenerator({ HANDLE: 'h' }));
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
