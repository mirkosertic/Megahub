import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category: 'Control flow',
	colour: colorControlFlow,
	inputsForToolbox: {
		"VALUE": {
          "shadow": {
            "type": "math_number",
            "fields": {
              "NUM": 1000
            }
          }
		}
	},
	blockdefinition: {
		"type": "mh_wait",
		"message0": "Wait %1ms",
		"args0": [
			{
				"type": "input_value",
				"name": "VALUE",
				"check": "Number",
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
