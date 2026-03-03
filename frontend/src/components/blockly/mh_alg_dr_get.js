import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_dr_get',
        message0: 'DR pose %1 of %2',
        args0: [
            {
                type: 'field_dropdown',
                name: 'FIELD',
                options: [
                    ['X', 'x'],
                    ['Y', 'y'],
                    ['HEADING', 'heading'],
                ],
            },
            {
                type: 'input_value',
                name: 'HANDLE',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Gets a pose component (X/Y in meters, HEADING in degrees) from a dead reckoning instance. ' +
            'Connect the handle from a DR update block variable. See DEADRECKONING.md for details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        const field = block.getFieldValue('FIELD');
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '""';
        return [`alg.drGet(${handle}, "${field}")`, 0];
    },
};
// clang-format on
