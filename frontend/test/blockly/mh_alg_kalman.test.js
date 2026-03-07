import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_kalman.js';

describe('mh_alg_kalman', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_kalman');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.kalman with handle, measurement, processNoise and measureNoise', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                if (name === 'HANDLE') return 'myKalman';
                if (name === 'MEASUREMENT') return '42';
                if (name === 'PROCESS_NOISE') return '0.1';
                if (name === 'MEASURE_NOISE') return '1';
                return '';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.kalman(myKalman, 42, 0.1, 1)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.kalman("kf_0", 0, 0.1, 1)');
    });

    it('has HANDLE input in inputsForToolbox using variables_get', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });

    it('has HANDLE variable named myKalman', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.fields.VAR.name).toBe('myKalman');
    });

    it('has MEASUREMENT shadow default of 0', () => {
        expect(definition.inputsForToolbox.MEASUREMENT.shadow.fields.NUM).toBe(0);
    });

    it('has PROCESS_NOISE shadow default of 0.1', () => {
        expect(definition.inputsForToolbox.PROCESS_NOISE.shadow.fields.NUM).toBe(0.1);
    });

    it('has MEASURE_NOISE shadow default of 1', () => {
        expect(definition.inputsForToolbox.MEASURE_NOISE.shadow.fields.NUM).toBe(1);
    });
});
