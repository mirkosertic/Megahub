import {colorIO} from './colors.js'

// clang-format off
export const definition = {
	category : 'I/O',
	colour : colorIO,
	blockdefinition : {
		"type" : "mh_port",
		"message0" : "%1",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "PORT",
				"options" : [
					[ "PORT1", "PORT1" ],
					[ "PORT2", "PORT2" ],
					[ "PORT3", "PORT3" ],
					[ "PORT4", "PORT4" ]
				]
			},
		],
		"output" : null,
		"colour" : colorIO,
		"tooltip" : "A LEGO© intergface port",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const port = block.getFieldValue('PORT');

		return [ port, 0 ];
	}
};
// clang-format on
