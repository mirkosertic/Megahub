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
                        name: 'myRobot',
                    },
                },
            },
        },
        LEFT_TICKS: {
            shadow: {
                type: 'lego_get_mode_dataset',
                inputs: {
                    PORT: {
                        shadow: {
                            type: 'mh_port',
                            fields: { PORT: 'PORT1' },
                        },
                    },
                    DATASET: {
                        shadow: {
                            type: 'math_number',
                            fields: { NUM: 0 },
                        },
                    },
                },
            },
        },
        RIGHT_TICKS: {
            shadow: {
                type: 'lego_get_mode_dataset',
                inputs: {
                    PORT: {
                        shadow: {
                            type: 'mh_port',
                            fields: { PORT: 'PORT2' },
                        },
                    },
                    DATASET: {
                        shadow: {
                            type: 'math_number',
                            fields: { NUM: 0 },
                        },
                    },
                },
            },
        },
        YAW: {
            shadow: {
                type: 'mh_imu_value',
                fields: { VALUE: 'YAW' },
            },
        },
        M_PER_TICK: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0.0005 },
            },
        },
        WHEELBASE: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0.12 },
            },
        },
        IMU_WEIGHT: {
            shadow: {
                type: 'math_number',
                fields: { NUM: 0.8 },
            },
        },
    },
    blockdefinition: {
        type: 'mh_alg_dr_update',
        message0:
            'Update DR %1 %2 left ticks: %3 right ticks: %4 \n%5 yaw (deg): %6 m/tick: %7 wheelbase (m): %8 \n%9 IMU weight: %10',
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
                name: 'LEFT_TICKS',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'RIGHT_TICKS',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'YAW',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'M_PER_TICK',
                check: 'Number',
            },
            {
                type: 'input_value',
                name: 'WHEELBASE',
                check: 'Number',
            },
            {
                type: 'input_dummy',
            },
            {
                type: 'input_value',
                name: 'IMU_WEIGHT',
                check: 'Number',
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorAlgorithms,
        tooltip:
            'Updates the dead reckoning state with new encoder and IMU readings. ' +
            'The handle comes from the "Initialize DR" block stored in a variable. ' +
            'Requires LEGO motor in POS mode (selectmode port 2). ' +
            'See DEADRECKONING.md for algorithm details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '"dr_0"';
        const lTicks = generator.valueToCode(block, 'LEFT_TICKS', 0) || '0';
        const rTicks = generator.valueToCode(block, 'RIGHT_TICKS', 0) || '0';
        const yaw = generator.valueToCode(block, 'YAW', 0) || '0';
        const mpt = generator.valueToCode(block, 'M_PER_TICK', 0) || '0.0005';
        const wb = generator.valueToCode(block, 'WHEELBASE', 0) || '0.12';
        const imuWeight = generator.valueToCode(block, 'IMU_WEIGHT', 0) || '0.8';

        return `alg.updateDR(${handle}, ${lTicks}, ${rTicks}, ${yaw}, ${wb}, ${mpt}, ${imuWeight})\n`;
    },
};
// clang-format on
