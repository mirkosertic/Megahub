import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category: 'Control flow',
	colour: colorControlFlow,
	blockdefinition: {
		"type": "mh_init",
		"message0": "Initialization do %1",
		"args0": [
			{
				"type": "input_statement",
				"name": "DO"
			}
		],
		"colour": colorControlFlow,
		"tooltip": "Initialization",
		"helpUrl": ""
	},
	generator: (block, generator) => {
		var statements = generator.statementToCode(block, 'DO');

		return "hub.init(function()\n" + statements + "end)\n";
	}
};
// clang-format on
