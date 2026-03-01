import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_pinmode.js';

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

describe('mh_pinmode block definition', () => {
    it('has category "I/O"', () => {
        expect(definition.category).toBe('I/O');
    });

    it('has type "mh_pinmode"', () => {
        expect(definition.blockdefinition.type).toBe('mh_pinmode');
    });

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('has 4 MODE options defined', () => {
        const modeArg = definition.blockdefinition.args0.find((a) => a.name === 'MODE');
        expect(modeArg).toBeDefined();
        expect(modeArg.options.length).toBe(4);
    });
});

describe('mh_pinmode generator', () => {
    it('generates "hub.pinMode(4,PINMODE_OUTPUT)\\n"', () => {
        const block = makeBlock({ PIN: '4', MODE: 'PINMODE_OUTPUT' });
        const gen = makeGenerator();
        const code = definition.generator(block, gen);
        expect(code).toBe('hub.pinMode(4,PINMODE_OUTPUT)\n');
    });

    it('includes PINMODE_INPUT_PULLUP when selected', () => {
        const block = makeBlock({ PIN: 'GPIO13', MODE: 'PINMODE_INPUT_PULLUP' });
        const gen = makeGenerator();
        const code = definition.generator(block, gen);
        expect(code).toContain('PINMODE_INPUT_PULLUP');
    });

    it('ends with newline', () => {
        const block = makeBlock({ PIN: '4', MODE: 'PINMODE_OUTPUT' });
        const gen = makeGenerator();
        const code = definition.generator(block, gen);
        expect(code).toMatch(/\n$/);
    });
});
