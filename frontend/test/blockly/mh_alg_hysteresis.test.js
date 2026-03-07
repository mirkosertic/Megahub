import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_hysteresis.js';

describe('mh_alg_hysteresis', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_hysteresis');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.hysteresis with handle, value, low and high', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                if (name === 'HANDLE') return 'myHysteresis';
                if (name === 'VALUE') return '500';
                if (name === 'LOW') return '400';
                if (name === 'HIGH') return '600';
                return '';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.hysteresis(myHysteresis, 500, 400, 600)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.hysteresis("hy_0", 500, 400, 600)');
    });

    it('has HANDLE input in inputsForToolbox using variables_get', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });

    it('has HANDLE variable named myHysteresis', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.fields.VAR.name).toBe('myHysteresis');
    });

    it('has VALUE shadow default of 500', () => {
        expect(definition.inputsForToolbox.VALUE.shadow.fields.NUM).toBe(500);
    });

    it('has LOW shadow default of 400', () => {
        expect(definition.inputsForToolbox.LOW.shadow.fields.NUM).toBe(400);
    });

    it('has HIGH shadow default of 600', () => {
        expect(definition.inputsForToolbox.HIGH.shadow.fields.NUM).toBe(600);
    });
});
