import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_dr_init',
        message0: 'Initialize DR',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new dead reckoning instance and returns a handle. ' +
            'Store this in a variable for use with other DR blocks.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        return [`alg.initDR()`, 0];
    },
};
// clang-format on
