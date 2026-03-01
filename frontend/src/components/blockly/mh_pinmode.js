import { colorIO } from './colors.js';
import { PINS_READWRITE } from './constants.js';

// clang-format off
export const definition = {
    category: 'I/O',
    colour: colorIO,
    blockdefinition: {
        type: 'mh_pinmode',
        message0: 'Set pin mode of %1 to %2',
        args0: [
            {
                type: 'field_dropdown',
                name: 'PIN',
                options: PINS_READWRITE,
            },
            {
                type: 'field_dropdown',
                name: 'MODE',
                options: [
                    ['PINMODE_INPUT', 'PINMODE_INPUT'],
                    ['PINMODE_INPUT_PULLUP', 'PINMODE_INPUT_PULLUP'],
                    ['PINMODE_INPUT_PULLDOWN', 'PINMODE_INPUT_PULLDOWN'],
                    ['PINMODE_OUTPUT', 'PINMODE_OUTPUT'],
                ],
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorIO,
        tooltip: 'Set the mode of a GPIO pin',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const pin = block.getFieldValue('PIN');
        const mode = block.getFieldValue('MODE');

        return 'hub.pinMode(' + pin + ',' + mode + ')\n';
    },
};
// clang-format on
