/**
 * Unit tests for the mh_wait Blockly block generator.
 *
 * The generator is a pure function: (block, generatorHelper) => Lua code string.
 * We mock only the minimal Blockly API surface it uses, so no Blockly import needed.
 */
import { definition } from './mh_wait.js';

/**
 * Create a minimal mock Blockly block with the given field values.
 * @param {Record<string, string>} fields
 */
function makeBlock(fields = {}) {
    return {
        getFieldValue: key => fields[key] ?? null,
    };
}

/**
 * Create a minimal mock generator helper.
 * valueToCode returns the provided values map keyed by input name.
 * @param {Record<string, string>} values
 */
function makeGenerator(values = {}) {
    return {
        valueToCode: (_block, name, _order) => values[name] ?? '0',
    };
}

describe('mh_wait block definition', () => {
    it('has category "Control flow"', () => {
        expect(definition.category).toBe('Control flow');
    });

    it('has a blockdefinition with type "mh_wait"', () => {
        expect(definition.blockdefinition.type).toBe('mh_wait');
    });

    it('has a tooltip', () => {
        expect(typeof definition.blockdefinition.tooltip).toBe('string');
        expect(definition.blockdefinition.tooltip.length).toBeGreaterThan(0);
    });
});

describe('mh_wait generator', () => {
    it('generates a wait() call with the VALUE input', () => {
        const block = makeBlock();
        const gen = makeGenerator({ VALUE: '500' });

        const code = definition.generator(block, gen);

        expect(code).toContain('wait(500)');
        expect(code).toMatch(/\n$/); // statement blocks must end with newline
    });

    it('uses the generator value for arbitrary expressions', () => {
        const block = makeBlock();
        const gen = makeGenerator({ VALUE: 'myVar * 2' });

        const code = definition.generator(block, gen);

        expect(code).toContain('wait(myVar * 2)');
    });
});
