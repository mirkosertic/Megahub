import { colorUI } from './colors.js';

// clang-format off
export const definition = {
    category: 'UI',
    colour: colorUI,
    blockdefinition: {
        type: 'ui_map_clear',
        message0: 'Map: clear',
        args0: [],
        previousStatement: true,
        nextStatement: true,
        colour: colorUI,
        tooltip: 'Clears the map trail in the visualization panel. See DEADRECKONING.md for details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        return 'ui.mapclear()\n';
    },
};
// clang-format on
