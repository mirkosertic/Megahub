import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/ui_map_clear.js';

describe('ui_map_clear block definition', () => {
    it('has category "UI"', () => {
        expect(definition.category).toBe('UI');
    });

    it('has type "ui_map_clear"', () => {
        expect(definition.blockdefinition.type).toBe('ui_map_clear');
    });

    it('is a statement block', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('has no inputs', () => {
        expect(definition.blockdefinition.args0).toEqual([]);
    });

    it('has no inputsForToolbox (no shadows needed)', () => {
        expect(definition.inputsForToolbox).toBeUndefined();
    });
});

describe('ui_map_clear generator', () => {
    it('generates ui.mapclear() call', () => {
        const block = { id: 'test-id' };
        const gen = {};
        const result = definition.generator(block, gen);
        expect(result).toBe('ui.mapclear()\n');
    });

    it('returns a string ending with newline', () => {
        const result = definition.generator({ id: 'x' }, {});
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
