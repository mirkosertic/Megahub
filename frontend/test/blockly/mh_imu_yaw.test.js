import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_imu_yaw.js';

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

describe('mh_imu_yaw block definition', () => {
    it('has category "IMU"', () => {
        expect(definition.category).toBe('IMU');
    });

    it('has type "mh_imu_yaw"', () => {
        expect(definition.blockdefinition.type).toBe('mh_imu_yaw');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });
});

describe('mh_imu_yaw generator', () => {
    it('returns array ["imu.yaw()", 0]', () => {
        const block = makeBlock();
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(Array.isArray(result)).toBe(true);
        expect(result[0]).toBe('imu.yaw()');
        expect(result[1]).toBe(0);
    });
});
