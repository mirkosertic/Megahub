import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_gamepad_gamepad.js';

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

describe('mh_gamepad_gamepad block definition', () => {
    it('has category "Gamepad"', () => {
        expect(definition.category).toBe('Gamepad');
    });

    it('has type "mh_gamepad_gamepad"', () => {
        expect(definition.blockdefinition.type).toBe('mh_gamepad_gamepad');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });
});

describe('mh_gamepad_gamepad generator', () => {
    it('returns ["GAMEPAD1", 0] for GAMEPAD=GAMEPAD1', () => {
        const block = makeBlock({ GAMEPAD: 'GAMEPAD1' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('GAMEPAD1');
        expect(result[1]).toBe(0);
    });

    it('returns the field value as the code', () => {
        const block = makeBlock({ GAMEPAD: 'GAMEPAD2' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('GAMEPAD2');
        expect(result[1]).toBe(0);
    });
});
