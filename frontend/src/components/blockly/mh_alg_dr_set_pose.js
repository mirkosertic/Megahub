import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    inputsForToolbox: {
        X: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        Y: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        HEADING: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_dr_set_pose',
        message0: 'Set DR pose of %1 %2 x: %3 y: %4 heading: %5',
        args0: [
            {
                type: 'input_value',
                name: 'HANDLE',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'X',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'Y',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'HEADING',
                check: 'Number',
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorAlgorithms,
        tooltip:
            'Injects a known absolute pose (x, y in meters; heading in degrees) into the dead ' +
            'reckoning state. Use when a landmark is detected to correct accumulated drift. ' +
            'See DEADRECKONING.md for details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '""';
        const x = generator.valueToCode(block, 'X', 0) || '0';
        const y = generator.valueToCode(block, 'Y', 0) || '0';
        const heading = generator.valueToCode(block, 'HEADING', 0) || '0';
        return `alg.drSetPose(${handle}, ${x}, ${y}, ${heading})\n`;
    },
};
// clang-format on
