import { colorIO } from './colors.js';
import { PINS_READWRITE } from './constants.js';

// clang-format off
export const definition = {
    category: 'I/O',
    colour: colorIO,
    blockdefinition: {
        type: 'mh_digitalwrite',
        message0: 'Digital write %2 to %1',
        args0: [
            {
                type: 'field_dropdown',
                name: 'PIN',
                options: PINS_READWRITE,
            },
            {
                type: 'input_value',
                name: 'VALUE',
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorIO,
        tooltip: 'Set the state of a GPIO pin',
        helpUrl: '',
    },
    generator: (block, generator) => {
        const pin = block.getFieldValue('PIN');

        const valueCode = generator.valueToCode(block, 'VALUE', 0);

        return 'hub.digitalWrite(' + pin + ',' + valueCode + ')\n';
    },
};
// clang-format on
