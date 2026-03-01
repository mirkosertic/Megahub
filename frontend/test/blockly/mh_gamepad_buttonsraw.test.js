import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_gamepad_buttonsraw.js';

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

describe('mh_gamepad_buttonsraw block definition', () => {
    it('has category "Gamepad"', () => {
        expect(definition.category).toBe('Gamepad');
    });

    it('has type "mh_gamepad_buttonsraw"', () => {
        expect(definition.blockdefinition.type).toBe('mh_gamepad_buttonsraw');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });
});

describe('mh_gamepad_buttonsraw generator', () => {
    it('generates ["gamepad.buttonsraw(GAMEPAD1)", 0] for GAMEPAD1', () => {
        const block = makeBlock();
        const gen = makeGenerator({ GAMEPAD: 'GAMEPAD1' });
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('gamepad.buttonsraw(GAMEPAD1)');
        expect(result[1]).toBe(0);
    });

    it('embeds gamepad value in output', () => {
        const block = makeBlock();
        const gen = makeGenerator({ GAMEPAD: 'myGamepad' });
        const result = definition.generator(block, gen);
        expect(result[0]).toContain('myGamepad');
    });
});
