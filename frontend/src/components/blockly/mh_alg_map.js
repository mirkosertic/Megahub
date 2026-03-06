import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    inputsForToolbox: {
        VALUE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        IN_MIN: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        IN_MAX: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 1023 },
            },
        },
        OUT_MIN: {
            shadow: {
                type: 'math_number',
                fields: { NUM: -100 },
            },
        },
        OUT_MAX: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 100 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_map',
        message0: 'Map %1 %2 value: %3 \n%4 from: %5 – %6 \n%7 to: %8 – %9',
        args0: [
            {
                type: 'input_dummy',
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
                name: 'IN_MIN',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'IN_MAX',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'OUT_MIN',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'OUT_MAX',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Linearly maps a value from one range to another. ' +
            'Equivalent to Arduino map() but with float support. ' +
            'Output is not clamped — values outside the input range extrapolate linearly.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const value  = generator.valueToCode(block, 'VALUE', 0) || '0';
        const inMin  = generator.valueToCode(block, 'IN_MIN', 0) || '0';
        const inMax  = generator.valueToCode(block, 'IN_MAX', 0) || '1023';
        const outMin = generator.valueToCode(block, 'OUT_MIN', 0) || '-100';
        const outMax = generator.valueToCode(block, 'OUT_MAX', 0) || '100';
        return [`alg.map(${value}, ${inMin}, ${inMax}, ${outMin}, ${outMax})`, 0];
    },
};
// clang-format on
