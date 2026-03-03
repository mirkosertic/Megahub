import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_pid_init',
        message0: 'Initialize PID',
        args0: [],
        output: null,
        colour: colorAlgorithms,
        tooltip:
            'Creates a new PID controller instance and returns a handle. ' +
            'Store this in a variable for use with PID compute and reset blocks.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        return [`alg.initPID()`, 0];
    },
};
// clang-format on
