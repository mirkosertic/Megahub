import { colorLego } from './colors.js';

// clang-format off
export const definition = {
    category: 'LEGO©',
    colour: colorLego,
    inputsForToolbox: {
        PORT: {
            shadow: {
                type: 'mh_port',
                fields: {
                    PORT: 'PORT1',
                },
            },
        },
        MODE: {
            shadow: {
                type: 'lego_get_device_mode',
                inputs: {
                    PORT: {
                        shadow: {
                            type: 'mh_port',
                            fields: {
                                PORT: 'PORT1',
                            },
                        },
                    },
                },
            },
        },
    },
    blockdefinition: {
        type: 'lego_get_mode_dataset',
        message0: 'Get dataset %3 from mode %2 of %1',
        args0: [
            {
                type: 'input_value',
                name: 'PORT',
            },
            {
                type: 'input_value',
                name: 'MODE',
            },
            {
                type: 'input_value',
                name: 'DATASET',
            },
        ],
        output: null,
        colour: colorLego,
        tooltip: 'Get a dataset value from a specific mode of a port',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const port = generator.valueToCode(block, 'PORT', 0);
        const modeCode = generator.valueToCode(block, 'MODE', 0);
        const datasetCode = generator.valueToCode(block, 'DATASET', 0);

        const command = 'lego.getmodedataset(' + port + ',' + modeCode + ',' + datasetCode + ')';
        return [command, 0];
    },
};
// clang-format on
