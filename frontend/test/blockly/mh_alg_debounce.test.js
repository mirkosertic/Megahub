import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_debounce.js';

describe('mh_alg_debounce', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_debounce');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.debounce with handle, signal and stableMs', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                if (name === 'HANDLE') return 'myDebounce';
                if (name === 'SIGNAL') return '1';
                if (name === 'STABLE_MS') return '20';
                return '';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.debounce(myDebounce, 1, 20)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.debounce("db_0", 0, 20)');
    });

    it('has HANDLE input in inputsForToolbox using variables_get', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });

    it('has HANDLE variable named myDebounce', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.fields.VAR.name).toBe('myDebounce');
    });

    it('has SIGNAL shadow default of 0', () => {
        expect(definition.inputsForToolbox.SIGNAL.shadow.fields.NUM).toBe(0);
    });

    it('has STABLE_MS shadow default of 20', () => {
        expect(definition.inputsForToolbox.STABLE_MS.shadow.fields.NUM).toBe(20);
    });
});
