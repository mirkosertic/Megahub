import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    inputsForToolbox: {
        HANDLE: {
            shadow: {
                type: 'variables_get',
                fields: {
                    VAR: {
                        name: 'myHysteresis',
                    },
                },
            },
        },
        VALUE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 500 },
            },
        },
        LOW: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 400 },
            },
        },
        HIGH: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 600 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_hysteresis',
        message0: 'Hysteresis %1 %2 value: %3 \n%4 low: %5 high: %6',
        args0: [
            {
                type: 'input_value',
                name: 'HANDLE',
                check: null,
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'VALUE',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'LOW',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'HIGH',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Hysteresis (Schmitt trigger) filter. Output switches to 1 when value exceeds high threshold, ' +
            'and back to 0 when value drops below low threshold. Eliminates chatter near a threshold. Returns 0 or 1.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"hy_0"';
        const value = generator.valueToCode(block, 'VALUE', 0) || '500';
        const low = generator.valueToCode(block, 'LOW', 0) || '400';
        const high = generator.valueToCode(block, 'HIGH', 0) || '600';
        return [`alg.hysteresis(${handle}, ${value}, ${low}, ${high})`, 0];
    },
};
// clang-format on
