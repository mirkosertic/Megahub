import { colorIO } from './colors.js';
import { PINS_READ } from './constants.js';

// clang-format off
export const definition = {
    category: 'I/O',
    colour: colorIO,
    blockdefinition: {
        type: 'mh_digitalread',
        message0: 'Digital Read %1',
        args0: [
            {
                type: 'field_dropdown',
                name: 'PIN',
                options: PINS_READ,
            },
        ],
        output: null,
        colour: colorIO,
        tooltip: 'Liest den Zustand eines GPIO-Pins',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const pin = block.getFieldValue('PIN');

        const command = 'hub.digitalRead(' + pin + ')';

        return [command, 0];
    },
};
// clang-format on
