import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category: 'Control flow',
	colour: colorControlFlow,
	blockdefinition: {
		"type": "mh_wait",
		"message0": "Wait %1ms",
		"args0": [
			{
				"type": "input_value",
				"name": "VALUE"
			}
		],
		"previousStatement": true,
		"nextStatement": true,
		"colour": colorControlFlow,
		"tooltip": "Wait",
		"helpUrl": ""
	},
	generator: (block, generator) => {
		const valueCode = generator.valueToCode(block, 'VALUE', 0);

		return "wait(" + valueCode + ")\n";
	}
};
// clang-format on
