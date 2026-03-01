import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_gamepad_value.js';

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

describe('mh_gamepad_value block definition', () => {
    it('has category "Gamepad"', () => {
        expect(definition.category).toBe('Gamepad');
    });

    it('has type "mh_gamepad_value"', () => {
        expect(definition.blockdefinition.type).toBe('mh_gamepad_value');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });

    it('has 5 dropdown options for VALUE', () => {
        const valueArg = definition.blockdefinition.args0.find((a) => a.name === 'VALUE');
        expect(valueArg).toBeDefined();
        expect(valueArg.options.length).toBe(5);
    });
});

describe('mh_gamepad_value generator', () => {
    it('generates ["gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X)", 0]', () => {
        const block = makeBlock({ VALUE: 'GAMEPAD_LEFT_X' });
        const gen = makeGenerator({ GAMEPAD: 'GAMEPAD1' });
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X)');
        expect(result[1]).toBe(0);
    });

    it('generates ["gamepad.value(GAMEPAD1,GAMEPAD_DPAD)", 0]', () => {
        const block = makeBlock({ VALUE: 'GAMEPAD_DPAD' });
        const gen = makeGenerator({ GAMEPAD: 'GAMEPAD1' });
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('gamepad.value(GAMEPAD1,GAMEPAD_DPAD)');
        expect(result[1]).toBe(0);
    });

    it('embeds gamepad and value in output', () => {
        const block = makeBlock({ VALUE: 'GAMEPAD_RIGHT_Y' });
        const gen = makeGenerator({ GAMEPAD: 'myGamepad' });
        const result = definition.generator(block, gen);
        expect(result[0]).toContain('myGamepad');
        expect(result[0]).toContain('GAMEPAD_RIGHT_Y');
    });
});
