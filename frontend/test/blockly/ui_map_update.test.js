import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/ui_map_update.js';

function makeBlock(extra = {}) {
    return { id: 'test-id', ...extra };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('ui_map_update block definition', () => {
    it('has category "UI"', () => {
        expect(definition.category).toBe('UI');
    });

    it('has type "ui_map_update"', () => {
        expect(definition.blockdefinition.type).toBe('ui_map_update');
    });

    it('is a statement block', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('has toolbox shadows for X, Y, HEADING', () => {
        expect(definition.inputsForToolbox).toHaveProperty('X');
        expect(definition.inputsForToolbox).toHaveProperty('Y');
        expect(definition.inputsForToolbox).toHaveProperty('HEADING');
    });
});

describe('ui_map_update generator', () => {
    it('generates correct ui.mappoint call', () => {
        const block = makeBlock();
        const gen = makeGenerator({ X: '1.2', Y: '3.4', HEADING: '90' });
        const result = definition.generator(block, gen);
        expect(result).toBe('ui.mappoint(1.2, 3.4, 90)\n');
    });

    it('uses defaults when inputs are empty', () => {
        const result = definition.generator(makeBlock(), makeGenerator({}));
        expect(result).toBe('ui.mappoint(0, 0, 0)\n');
    });

    it('returns a string ending with newline', () => {
        const result = definition.generator(makeBlock(), makeGenerator({}));
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
