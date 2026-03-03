import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_alg_pid_compute.js';

function makeBlock(extra = {}) {
    return {
        id: extra.id ?? 'test-id',
        outputConnection: extra.outputConnection ?? {},
        ...extra,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (block, name) => values[name] ?? '',
    };
}

describe('mh_alg_pid_compute block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms');
    });

    it('has type "mh_alg_pid_compute"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_pid_compute');
    });

    it('has output "Number" (value block)', () => {
        expect(definition.blockdefinition.output).toBe('Number');
    });

    it('has 8 value inputs', () => {
        const valueInputs = definition.blockdefinition.args0.filter((a) => a.type === 'input_value');
        expect(valueInputs.length).toBe(8);
    });

    it('has toolbox shadows for all 8 inputs', () => {
        const keys = Object.keys(definition.inputsForToolbox);
        expect(keys).toContain('HANDLE');
        expect(keys).toContain('SETPOINT');
        expect(keys).toContain('PV');
        expect(keys).toContain('KP');
        expect(keys).toContain('KI');
        expect(keys).toContain('KD');
        expect(keys).toContain('OUT_MIN');
        expect(keys).toContain('OUT_MAX');
    });
});

describe('mh_alg_pid_compute generator', () => {
    it('generates correct alg.computePID call with all parameters', () => {
        const block = makeBlock();
        const gen = makeGenerator({
            HANDLE: 'myPID',
            SETPOINT: '100',
            PV: '50',
            KP: '1.0',
            KI: '0.1',
            KD: '0.01',
            OUT_MIN: '-127',
            OUT_MAX: '128',
        });
        const [code, prec] = definition.generator(block, gen);
        expect(code).toBe('alg.computePID(myPID, 100, 50, 1.0, 0.1, 0.01, -127, 128)');
        expect(prec).toBe(0);
    });

    it('uses "pid_0" default when handle is empty', () => {
        const block = makeBlock();
        const gen = makeGenerator({
            SETPOINT: '100',
            PV: '50',
            KP: '1.0',
            KI: '0.1',
            KD: '0.01',
            OUT_MIN: '-127',
            OUT_MAX: '128',
        });
        const [code] = definition.generator(block, gen);
        expect(code).toBe('alg.computePID("pid_0", 100, 50, 1.0, 0.1, 0.01, -127, 128)');
    });

    it('uses defaults for parameters when valueToCode returns empty string', () => {
        const block = makeBlock();
        const gen = makeGenerator({ HANDLE: 'controller' });
        const [code] = definition.generator(block, gen);
        expect(code).toBe('alg.computePID(controller, 0, 0, 1.0, 0.0, 0.0, -100, 100)');
    });

    it('returns array [code, 0]', () => {
        const block = makeBlock();
        const result = definition.generator(block, makeGenerator({ HANDLE: 'h' }));
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[1]).toBe(0);
    });
});
