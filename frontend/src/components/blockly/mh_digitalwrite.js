import {colorIO} from './colors.js'

// clang-format off
export const definition = {
	category : 'I/O',
	colour : colorIO,
	blockdefinition : {
		"type" : "mh_digitalwrite",
		"message0" : "Digital write %1 to %2",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "PIN",
				"options" : [
					[ "GPIO13", "GPIO13" ],
					[ "GPIO16", "GPIO16" ],
					[ "GPIO17", "GPIO17" ],
					[ "GPIO25", "GPIO25" ],
					[ "GPIO26", "GPIO26" ],
					[ "GPIO27", "GPIO27" ],
					[ "GPIO32", "GPIO32" ],
					[ "GPIO33", "GPIO33" ],
					[ "UART1_GP4", "UART1_GP4" ],
					[ "UART1_GP5", "UART1_GP5" ],
					[ "UART1_GP6", "UART1_GP6" ],
					[ "UART1_GP7", "UART1_GP7" ],
					[ "UART2_GP4", "UART2_GP4" ],
					[ "UART2_GP5", "UART2_GP5" ],
					[ "UART2_GP6", "UART2_GP6" ],
					[ "UART2_GP7", "UART2_GP7" ]
				]
			},
			{
				"type" : "input_value",
				"name" : "VALUE"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorIO,
		"tooltip" : "Set the state of a GPIO pin",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const pin = block.getFieldValue('PIN');

		const valueCode = generator.valueToCode(block, 'VALUE', 0);

		return "hub.digitalWrite(" + pin + "," + valueCode + ")\n";
	}
};
// clang-format on
