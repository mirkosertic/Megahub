import { colorUI } from './colors.js';

// clang-format off
export const definition = {
    category: 'UI',
    colour: colorUI,
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
        type: 'ui_map_update',
        message0: 'Map: plot position %1 x: %2 y: %3 heading: %4',
        args0: [
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
        colour: colorUI,
        tooltip:
            'Sends the current robot position to the map visualization panel. ' +
            'x and y in meters, heading in degrees. Internally rate-limited to ~5 Hz. ' +
            'See DEADRECKONING.md for details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        const x = generator.valueToCode(block, 'X', 0) || '0';
        const y = generator.valueToCode(block, 'Y', 0) || '0';
        const heading = generator.valueToCode(block, 'HEADING', 0) || '0';
        return `ui.mappoint(${x}, ${y}, ${heading})\n`;
    },
};
// clang-format on
