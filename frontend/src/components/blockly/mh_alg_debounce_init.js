import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_debounce_init',
        message0: 'Initialize debounce',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new debounce filter instance and returns a handle. ' +
            'Store in a variable and pass to the Debounce block.',
        helpUrl: '',
    },
    generator: (_block, _generator) => {
        return [`alg.initDebounce()`, 0];
    },
};
// clang-format on
