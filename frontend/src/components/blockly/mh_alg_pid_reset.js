import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    inputsForToolbox: {
        HANDLE: {
            shadow: {
                type: 'variables_get',
                fields: {
                    VAR: {
                        name: 'myPID',
                    },
                },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_pid_reset',
        message0: 'Reset PID %1',
        args0: [
            {
                type: 'input_value',
                name: 'HANDLE',
                check: null,
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorAlgorithms,
        tooltip:
            'Resets PID controller state (clears integral accumulator and error history). ' +
            'Use when switching setpoints or restarting control.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"pid_0"';
        return `alg.resetPID(${handle})\n`;
    },
};
// clang-format on
