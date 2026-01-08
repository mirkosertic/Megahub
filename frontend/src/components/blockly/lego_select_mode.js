import {colorLego} from './colors.js'

// clang-format off
export const definition = {
	category : 'LEGO©',
	colour : colorLego,
	blockdefinition : {
		"type" : "lego_select_mode",
		"message0" : "Set the mode of %1 to %2",
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
			{
				"type" : "input_value",
				"name" : "MODE"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorLego,
		"tooltip" : "Sets the mode of a port",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const port = block.getFieldValue('PORT');

		const modeCode = generator.valueToCode(block, 'MODE', 0);

		return "lego.selectmode(" + port + ", " + modeCode + ")\n";
	}
};
// clang-format on
