import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_hysteresis_init.js';

describe('mh_alg_hysteresis_init', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_hysteresis_init');
    });

    it('has output null (value block)', () => {
        expect(definition.blockdefinition.output).toBe(null);
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator returns alg.initHysteresis()', () => {
        const [code, prec] = definition.generator({}, {});
        expect(code).toBe('alg.initHysteresis()');
        expect(prec).toBe(0);
    });
});
