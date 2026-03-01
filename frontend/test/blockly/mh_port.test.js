import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_port.js';

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

describe('mh_port block definition', () => {
    it('has category "I/O"', () => {
        expect(definition.category).toBe('I/O');
    });

    it('has type "mh_port"', () => {
        expect(definition.blockdefinition.type).toBe('mh_port');
    });

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true);
    });

    it('has 4 dropdown options for PORT', () => {
        const portArg = definition.blockdefinition.args0.find((a) => a.name === 'PORT');
        expect(portArg).toBeDefined();
        expect(portArg.options.length).toBe(4);
    });
});

describe('mh_port generator', () => {
    it('returns ["PORT1", 0] for PORT=PORT1', () => {
        const block = makeBlock({ PORT: 'PORT1' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('PORT1');
        expect(result[1]).toBe(0);
    });

    it('returns ["PORT2", 0] for PORT=PORT2', () => {
        const block = makeBlock({ PORT: 'PORT2' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('PORT2');
        expect(result[1]).toBe(0);
    });

    it('returns ["PORT3", 0] for PORT=PORT3', () => {
        const block = makeBlock({ PORT: 'PORT3' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('PORT3');
        expect(result[1]).toBe(0);
    });

    it('returns ["PORT4", 0] for PORT=PORT4', () => {
        const block = makeBlock({ PORT: 'PORT4' });
        const gen = makeGenerator();
        const result = definition.generator(block, gen);
        expect(result[0]).toBe('PORT4');
        expect(result[1]).toBe(0);
    });
});
