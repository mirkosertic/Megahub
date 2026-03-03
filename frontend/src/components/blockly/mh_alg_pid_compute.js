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
        SETPOINT: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 100,
                },
            },
        },
        PV: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 0,
                },
            },
        },
        KP: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 1.0,
                },
            },
        },
        KI: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 0.0,
                },
            },
        },
        KD: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 0.0,
                },
            },
        },
        OUT_MIN: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: -127,
                },
            },
        },
        OUT_MAX: {
            shadow: {
                type: 'math_number',
                fields: {
                    NUM: 128,
                },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_pid_compute',
        message0: 'PID compute %1 %2 setpoint: %3 PV: %4 \n%5 Kp: %6 Ki: %7 Kd: %8 %9 \noutput: %10 to %11',
        args0: [
            {
                type: 'input_value',
                name: 'HANDLE',
                check: null,
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'SETPOINT',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'PV',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'KP',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'KI',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'KD',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'OUT_MIN',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'OUT_MAX',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Computes PID control output. The handle comes from "Initialize PID" block. ' +
            'Returns the control value clamped to output range.',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"pid_0"';
        const setpoint = generator.valueToCode(block, 'SETPOINT', 0) || '0';
        const pv = generator.valueToCode(block, 'PV', 0) || '0';
        const kp = generator.valueToCode(block, 'KP', 0) || '1.0';
        const ki = generator.valueToCode(block, 'KI', 0) || '0.0';
        const kd = generator.valueToCode(block, 'KD', 0) || '0.0';
        const outMin = generator.valueToCode(block, 'OUT_MIN', 0) || '-100';
        const outMax = generator.valueToCode(block, 'OUT_MAX', 0) || '100';

        const command = `alg.computePID(${handle}, ${setpoint}, ${pv}, ${kp}, ${ki}, ${kd}, ${outMin}, ${outMax})`;

        return [command, 0];
    },
};
// clang-format on
