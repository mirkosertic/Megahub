import {colorIO} from './colors.js'

// clang-format off
export const definition = {
	category : 'I/O',
	colour : colorIO,
	blockdefinition : {
		"type" : "mh_digitalread",
		"message0" : "Digital Read %1",
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
					[ "GPIO34", "GPIO34" ],
					[ "GPIO35", "GPIO35" ],
					[ "GPIO36", "GPIO36" ],
					[ "GPIO39", "GPIO39" ],
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
		],
		"output" : null,
		"colour" : colorIO,
		"tooltip" : "Liest den Zustand eines GPIO-Pins",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const pin = block.getFieldValue('PIN');

		const command = "hub.digitalRead(" + pin + ")";

		return [ command, 0 ];
	}
};
// clang-format on
