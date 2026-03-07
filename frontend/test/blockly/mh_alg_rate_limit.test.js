import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_rate_limit.js';

describe('mh_alg_rate_limit', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_rate_limit');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.rateLimit with handle, target and maxDelta', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                if (name === 'HANDLE') return 'myRateLimiter';
                if (name === 'TARGET') return '100';
                if (name === 'MAX_DELTA') return '5';
                return '';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.rateLimit(myRateLimiter, 100, 5)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.rateLimit("rl_0", 0, 5)');
    });

    it('has HANDLE input in inputsForToolbox using variables_get', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });

    it('has HANDLE variable named myRateLimiter', () => {
        expect(definition.inputsForToolbox.HANDLE.shadow.fields.VAR.name).toBe('myRateLimiter');
    });

    it('has TARGET shadow default of 0', () => {
        expect(definition.inputsForToolbox.TARGET.shadow.fields.NUM).toBe(0);
    });

    it('has MAX_DELTA shadow default of 5', () => {
        expect(definition.inputsForToolbox.MAX_DELTA.shadow.fields.NUM).toBe(5);
    });
});
