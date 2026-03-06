import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_moving_avg_init',
        message0: 'Initialize moving average',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new moving average filter instance and returns a handle. ' +
            'Store this in a variable and pass it to the Moving average block.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        return [`alg.initMovingAvg()`, 0];
    },
};
// clang-format on
