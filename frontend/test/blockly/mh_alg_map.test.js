import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_map.js';

describe('mh_alg_map', () => {
    it('has correct block type', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_map');
    });

    it('has output Number', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('is in Algorithms category', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('generator emits alg.map with all five arguments', () => {
        const mockBlock = {};
        const mockGenerator = {
            valueToCode: (_b, name, _prec) => {
                const vals = { VALUE: '512', IN_MIN: '0', IN_MAX: '1023', OUT_MIN: '-100', OUT_MAX: '100' };
                return vals[name] ?? '0';
            },
        };
        const [code, prec] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.map(512, 0, 1023, -100, 100)');
        expect(prec).toBe(0);
    });

    it('generator uses fallback defaults when inputs are empty', () => {
        const mockBlock = {};
        const mockGenerator = { valueToCode: () => '' };
        const [code] = definition.generator(mockBlock, mockGenerator);
        expect(code).toBe('alg.map(0, 0, 1023, -100, 100)');
    });

    it('has correct toolbox shadow defaults', () => {
        expect(definition.inputsForToolbox.IN_MAX.shadow.fields.NUM).toBe(1023);
        expect(definition.inputsForToolbox.OUT_MIN.shadow.fields.NUM).toBe(-100);
        expect(definition.inputsForToolbox.OUT_MAX.shadow.fields.NUM).toBe(100);
    });
});
