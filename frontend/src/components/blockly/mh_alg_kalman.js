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
                        name: 'myKalman',
                    },
                },
            },
        },
        MEASUREMENT: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0 },
            },
        },
        PROCESS_NOISE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0.1 },
            },
        },
        MEASURE_NOISE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 1 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_kalman',
        message0: 'Kalman filter %1 %2 measurement: %3 \n%4 process noise: %5 \n%6 measure noise: %7',
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
                name: 'MEASUREMENT',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'PROCESS_NOISE',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'MEASURE_NOISE',
                check: 'Number',
            },
        ],
        output: 'Number',
        colour: colorAlgorithms,
        tooltip:
            'Kalman filter. Weighs measurements by noise characteristics for principled smoothing. ' +
            'Process noise Q: how much the true value drifts per call. Measure noise R: sensor variance. ' +
            'Starting values: color sensor Q=0.01 R=10, IMU Q=0.1 R=1.0.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/FILTERS.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"kf_0"';
        const measurement = generator.valueToCode(block, 'MEASUREMENT', 0) || '0';
        const processNoise = generator.valueToCode(block, 'PROCESS_NOISE', 0) || '0.1';
        const measureNoise = generator.valueToCode(block, 'MEASURE_NOISE', 0) || '1';
        return [`alg.kalman(${handle}, ${measurement}, ${processNoise}, ${measureNoise})`, 0];
    },
};
// clang-format on
