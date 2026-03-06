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
    },
    blockdefinition: {
        type: 'lego_get_device_mode',
        message0: 'Selected mode of %1',
        args0: [
            {
                type: 'input_value',
                name: 'PORT',
            },
        ],
        output: null,
        colour: colorLego,
        tooltip: 'Returns the currently selected mode index of a port',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const port = generator.valueToCode(block, 'PORT', 0);
        return ['lego.getdevicemode(' + port + ')', 0];
    },
};
// clang-format on
