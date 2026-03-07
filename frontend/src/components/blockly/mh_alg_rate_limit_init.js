import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_rate_limit_init',
        message0: 'Initialize rate limiter',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new rate limiter instance and returns a handle. ' +
            'Store in a variable and pass to the Rate limiter block.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (_block, _generator) => {
        return [`alg.initRateLimit()`, 0];
    },
};
// clang-format on
