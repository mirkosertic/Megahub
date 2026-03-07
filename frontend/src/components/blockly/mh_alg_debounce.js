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
                        name: 'myDebounce',
                    },
                },
            },
        },
        SIGNAL: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        STABLE_MS: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 20 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_debounce',
        message0: 'Debounce %1 %2 signal: %3 \n%4 stable ms: %5',
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
                name: 'SIGNAL',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'STABLE_MS',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Debounce filter for buttons and digital sensors. Output only changes when the input has been stable ' +
            'for the specified number of milliseconds. Returns 0 or 1. Typical stable time: 20–50 ms.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"db_0"';
        const signal = generator.valueToCode(block, 'SIGNAL', 0) || '0';
        const stableMs = generator.valueToCode(block, 'STABLE_MS', 0) || '20';
        return [`alg.debounce(${handle}, ${signal}, ${stableMs})`, 0];
    },
};
// clang-format on
