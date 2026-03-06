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
                        name: 'myFilter',
                    },
                },
            },
        },
        VALUE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        WINDOW: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 10 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_moving_avg',
        message0: 'Moving average %1 %2 value: %3 \n%4 window: %5',
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
                name: 'WINDOW',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Smooths a sensor value using a ring-buffer moving average. ' +
            'The handle comes from the "Initialize moving average" block stored in a variable. ' +
            'Window is the number of samples to average (2–50; default 10). ' +
            'On first call the buffer is pre-filled so there is no startup ramp.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"ma_0"';
        const value = generator.valueToCode(block, 'VALUE', 0) || '0';
        const window = generator.valueToCode(block, 'WINDOW', 0) || '10';
        return [`alg.movingAvg(${handle}, ${value}, ${window})`, 0];
    },
};
// clang-format on
