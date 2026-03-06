import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_moving_avg.js';

describe('mh_alg_moving_avg', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_moving_avg');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.movingAvg with handle, value and window', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                if (name === 'HANDLE') return 'myFilter';
                if (name === 'VALUE') return '42';
                if (name === 'WINDOW') return '10';
                return '';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.movingAvg(myFilter, 42, 10)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.movingAvg("ma_0", 0, 10)');
    });

    it('has HANDLE input in inputsForToolbox using variables_get', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });

    it('has window shadow default of 10', () => {
        expect(definition.inputsForToolbox.WINDOW.shadow.fields.NUM).toBe(10);
    });
});
