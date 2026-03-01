import { describe, it, expect } from 'vitest';
import { definition } from '../../src/components/blockly/mh_fastled_addleds.js';

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

describe('mh_fastled_addleds block definition', () => {
    it('has category "FastLED"', () => {
        expect(definition.category).toBe('FastLED');
    });

    it('has type "mh_fastled_addleds"', () => {
        expect(definition.blockdefinition.type).toBe('mh_fastled_addleds');
    });

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true);
        expect(definition.blockdefinition.nextStatement).toBe(true);
    });
});

describe('mh_fastled_addleds generator', () => {
    it('generates correct code with NEOPIXEL, pin 4, 100 LEDs', () => {
        const block = makeBlock({ TYPE: 'NEOPIXEL', PIN: '4' });
        const gen = makeGenerator({ NUM_LEDS: '100' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.addleds(NEOPIXEL,4,100)\n');
    });

    it('works with a different pin', () => {
        const block = makeBlock({ TYPE: 'NEOPIXEL', PIN: '13' });
        const gen = makeGenerator({ NUM_LEDS: '60' });
        const code = definition.generator(block, gen);
        expect(code).toBe('fastled.addleds(NEOPIXEL,13,60)\n');
    });

    it('embeds type, pin and num_leds in output', () => {
        const block = makeBlock({ TYPE: 'NEOPIXEL', PIN: 'GPIO27' });
        const gen = makeGenerator({ NUM_LEDS: '30' });
        const code = definition.generator(block, gen);
        expect(code).toContain('NEOPIXEL');
        expect(code).toContain('GPIO27');
        expect(code).toContain('30');
    });
});
