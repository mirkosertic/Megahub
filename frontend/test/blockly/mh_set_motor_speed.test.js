/**
 * Unit tests for the mh_set_motor_speed Blockly block generator.
 *
 * The generator is a pure function: (block, generator) => Lua code string.
 * No Blockly import needed — we mock the minimal API surface.
 */
import { definition } from '../../src/components/blockly/mh_set_motor_speed.js';

function makeBlock(fields = {}) {
    return {
        getFieldValue: (key) => fields[key] ?? null,
    };
}

function makeGenerator(values = {}) {
    return {
        valueToCode: (_block, name) => values[name] ?? '0',
    };
}

describe('mh_set_motor_speed block definition', () => {
    it('has category "I/O"', () => {
        expect(definition.category).toBe('I/O');
    });

    it('has type "mh_set_motor_speed"', () => {
        expect(definition.blockdefinition.type).toBe('mh_set_motor_speed');
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

describe('mh_set_motor_speed generator', () => {
    it('generates hub.setmotorspeed(port, value) call', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1', VALUE: '50' });

        const code = definition.generator(block, gen);

        expect(code).toContain('hub.setmotorspeed(');
        expect(code).toContain('PORT1');
        expect(code).toContain('50');
    });

    it('produces a statement ending with newline', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT1', VALUE: '100' });

        const code = definition.generator(block, gen);

        expect(code).toMatch(/\n$/);
    });

    it('uses default 0 for missing VALUE input', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'PORT2' }); // no VALUE

        const code = definition.generator(block, gen);

        expect(code).toContain('0');
        expect(code).toContain('PORT2');
    });

    it('includes both PORT and VALUE in the output', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'myPort', VALUE: 'speed * 2' });

        const code = definition.generator(block, gen);

        expect(code).toContain('myPort');
        expect(code).toContain('speed * 2');
    });

    it('produces exactly hub.setmotorspeed(port,value)\\n', () => {
        const block = makeBlock();
        const gen = makeGenerator({ PORT: 'p', VALUE: 'v' });

        const code = definition.generator(block, gen);

        expect(code).toBe('hub.setmotorspeed(p,v)\n');
    });
});
