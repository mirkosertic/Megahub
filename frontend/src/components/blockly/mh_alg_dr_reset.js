import { colorAlgorithms } from './colors.js';

// clang-format off
export const definition = {
    category: 'Algorithms',
    colour: colorAlgorithms,
    blockdefinition: {
        type: 'mh_alg_dr_reset',
        message0: 'Reset DR pose of %1',
        args0: [
            {
                type: 'input_value',
                name: 'HANDLE',
            },
        ],
        previousStatement: true,
        nextStatement: true,
        colour: colorAlgorithms,
        tooltip:
            'Resets the dead reckoning pose to (0, 0, 0°). The next update call re-bootstraps ' +
            'tick tracking so no spurious jump occurs. See DEADRECKONING.md for details.',
        helpUrl: 'https://github.com/mirkosertic/Megahub/DEADRECKONING.md',
    },
    generator: (block, generator) => {
        const handle = generator.valueToCode(block, 'HANDLE', 0) || '""';
        return `alg.drReset(${handle})\n`;
    },
};
// clang-format on
