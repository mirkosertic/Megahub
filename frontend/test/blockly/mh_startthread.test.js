/**
 * Unit tests for the mh_startthread Blockly block generator.
 *
 * mh_startthread is a dual-mode block: as a statement it returns a string
 * ending in \n; when wired as an expression (outputConnection connected)
 * it returns [code, 0].
 * No Blockly import needed — we mock the minimal API surface.
 */
import { definition } from '../../src/components/blockly/mh_startthread.js';

function makeBlock({ fields = {}, asExpression = false } = {}) {
    return {
        getFieldValue: (key) => fields[key] ?? null,
        id: 'test-block-id',
        outputConnection: asExpression ? { targetConnection: {} } : null,
    };
}

function makeGenerator({ values = {}, stmts = {} } = {}) {
    return {
        valueToCode: (_block, name) => values[name] ?? '0',
        statementToCode: (_block, name) => stmts[name] ?? '',
    };
}

describe('mh_startthread block definition', () => {
    it('has category "Control flow"', () => {
        expect(definition.category).toBe('Control flow');
    });

    it('has type "mh_startthread"', () => {
        expect(definition.blockdefinition.type).toBe('mh_startthread');
    });

    it('has a tooltip', () => {
        expect(typeof definition.blockdefinition.tooltip).toBe('string');
        expect(definition.blockdefinition.tooltip.length).toBeGreaterThan(0);
    });

    it('is a statement block (previousStatement and nextStatement)', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });
});

describe('mh_startthread generator — statement mode', () => {
    it('generates hub.startthread(...) call ending with \\n', () => {
        const block = makeBlock({ fields: { NAME: 'myThread', PROFILING: 'TRUE' } });
        const gen = makeGenerator({ values: { STACKSIZE: '4096' }, stmts: { DO: '  x = 1\n' } });

        const code = definition.generator(block, gen);

        expect(typeof code).toBe('string');
        expect(code).toContain('hub.startthread(');
        expect(code).toContain("'myThread'");
        expect(code).toContain('4096');
        expect(code).toContain('true'); // PROFILING: "TRUE" → true
        expect(code).toContain('function()');
        expect(code).toContain('end)');
        expect(code).toMatch(/\n$/);
    });

    it('sets profiling=false when checkbox is unchecked', () => {
        const block = makeBlock({ fields: { NAME: 'bg', PROFILING: 'FALSE' } });
        const gen = makeGenerator({ values: { STACKSIZE: '2048' } });

        const code = definition.generator(block, gen);

        expect(code).toContain('false');
    });

    it('embeds DO body inside function() ... end)', () => {
        const block = makeBlock({ fields: { NAME: 't', PROFILING: 'TRUE' } });
        const gen = makeGenerator({
            values: { STACKSIZE: '4096' },
            stmts: { DO: '  wait(100)\n' },
        });

        const code = definition.generator(block, gen);

        expect(code).toContain('wait(100)');
        expect(code).toContain('function()');
        expect(code).toContain('end)');
    });

    it('uses block.id in the generated code', () => {
        const block = makeBlock({ fields: { NAME: 'n', PROFILING: 'TRUE' } });
        const gen = makeGenerator({ values: { STACKSIZE: '4096' } });

        const code = definition.generator(block, gen);

        expect(code).toContain('test-block-id');
    });
});

describe('mh_startthread generator — expression mode', () => {
    it('returns [code, 0] when wired as expression', () => {
        const block = makeBlock({
            fields: { NAME: 'expr', PROFILING: 'FALSE' },
            asExpression: true,
        });
        const gen = makeGenerator({ values: { STACKSIZE: '1024' } });

        const result = definition.generator(block, gen);

        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(typeof result[0]).toBe('string');
        expect(result[1]).toBe(0);
        expect(result[0]).toContain('hub.startthread(');
    });
});
