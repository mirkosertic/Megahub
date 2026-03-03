import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_pid_reset.js';

function makeBlock(extra = {}) {
    return {
        id: extra.id ?? 'test-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_pid_reset block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_pid_reset"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_pid_reset');
    });

    it('has both previousStatement and nextStatement (statement block)', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });

    it('does not have output (pure statement block)', () => {
        expect(definition.blockdefinition.output).toBeUndefined();
    });

    it('has 1 value input', () => {
        const valueInputs = definition.blockdefinition.args0.filter((a) => a.type === 'input_value');
        expect(valueInputs.length).toBe(1);
    });

    it('has toolbox shadow for HANDLE', () => {
        expect(definition.inputsForToolbox.HANDLE).toBeDefined();
        expect(definition.inputsForToolbox.HANDLE.shadow.type).toBe('variables_get');
    });
});

describe('mh_alg_pid_reset generator', () => {
    it('generates correct alg.resetPID call with handle', () => {
        const block = makeBlock();
        const gen = makeGenerator({ HANDLE: 'myPID' });
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.resetPID(myPID)\n');
    });

    it('uses "pid_0" default when handle is empty', () => {
        const block = makeBlock();
        const gen = makeGenerator({});
        const result = definition.generator(block, gen);
        expect(result).toBe('alg.resetPID("pid_0")\n');
    });

    it('always returns a string (statement form only)', () => {
        const block = makeBlock();
        const result = definition.generator(block, makeGenerator({ HANDLE: 'h' }));
        expect(typeof result).toBe('string');
        expect(result.endsWith('\n')).toBe(true);
    });
});
