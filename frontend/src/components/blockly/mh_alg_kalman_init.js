import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_kalman_init',
        message0: 'Initialize Kalman filter',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new 1D Kalman filter instance and returns a handle. ' +
            'Store in a variable and pass to the Kalman filter block.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (_block, _generator) => {
        return [`alg.initKalman()`, 0];
    },
};
// clang-format on
