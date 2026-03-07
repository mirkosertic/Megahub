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
                        name: 'myRateLimiter',
                    },
                },
            },
        },
        TARGET: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        MAX_DELTA: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 5 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_rate_limit',
        message0: 'Rate limit %1 %2 target: %3 \n%4 max \u0394 per call: %5',
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
                name: 'TARGET',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'MAX_DELTA',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Rate limiter (slew rate). Limits how fast the output value can change per call to avoid sudden jumps. ' +
            'On first call returns target immediately. The effective rate in units/second depends on how often the block is called.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"rl_0"';
        const target = generator.valueToCode(block, 'TARGET', 0) || '0';
        const maxDelta = generator.valueToCode(block, 'MAX_DELTA', 0) || '5';
        return [`alg.rateLimit(${handle}, ${target}, ${maxDelta})`, 0];
    },
};
// clang-format on
