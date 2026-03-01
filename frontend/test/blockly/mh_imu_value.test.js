import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_imu_value.js';

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

describe('mh_imu_value block definition', () => {
    it('has category "IMU"', () => {
        expect(definition.category).toBe('IMU');
    });

    it('has type "mh_imu_value"', () => {
        expect(definition.blockdefinition.type).toBe('mh_imu_value');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });

    it('has 6 dropdown options defined', () => {
        const valueArg = definition.blockdefinition.args0.find((a) => a.name === 'VALUE');
        expect(valueArg).toBeDefined();
        expect(valueArg.options.length).toBe(6);
    });
});

describe('mh_imu_value generator', () => {
    it('returns ["imu.value(YAW)", 0] for VALUE=YAW', () => {
        const block = makeBlock({ VALUE: 'YAW' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('imu.value(YAW)');
        expect(result[1]).toBe(0);
    });

    it('returns ["imu.value(PITCH)", 0] for VALUE=PITCH', () => {
        const block = makeBlock({ VALUE: 'PITCH' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('imu.value(PITCH)');
        expect(result[1]).toBe(0);
    });

    it('returns ["imu.value(ROLL)", 0] for VALUE=ROLL', () => {
        const block = makeBlock({ VALUE: 'ROLL' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('imu.value(ROLL)');
        expect(result[1]).toBe(0);
    });

    it('returns ["imu.value(ACCELERATION_X)", 0] for VALUE=ACCELERATION_X', () => {
        const block = makeBlock({ VALUE: 'ACCELERATION_X' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('imu.value(ACCELERATION_X)');
        expect(result[1]).toBe(0);
    });
});
