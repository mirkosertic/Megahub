import {colorIO} from './colors.js'

// clang-format off
export const definition = {
	category : 'I/O',
	colour : colorIO,
	blockdefinition : {
		"type" : "mh_set_motor_speed",
		"message0" : "Set Motor speed of %1 to %2",
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
				"name" : "VALUE"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorIO,
		"tooltip" : "Set the speed of a connected motor",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const port = block.getFieldValue('PORT');

		const valueCode = generator.valueToCode(block, 'VALUE', 0);

		return "hub.setmotorspeed(" + port + "," + valueCode + ")\n";
	}
};
// clang-format on
