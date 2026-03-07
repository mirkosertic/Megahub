import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_hysteresis_init',
        message0: 'Initialize hysteresis',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new hysteresis (Schmitt trigger) instance and returns a handle. ' +
            'Store in a variable and pass to the Hysteresis block.',
        helpUrl: '',
    },
    generator: (_block, _generator) => {
        return [`alg.initHysteresis()`, 0];
    },
};
// clang-format on
