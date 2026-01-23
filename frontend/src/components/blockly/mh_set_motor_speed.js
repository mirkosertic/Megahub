import {colorIO} from './colors.js'

// clang-format off
export const definition = {
	category : 'I/O',
	colour : colorIO,
	inputsForToolbox: {
		"PORT": {
          "shadow": {
            "type": "mh_port",
            "fields": {
              "PORT": "PORT1"
            }
          }
		}
	},	
	blockdefinition : {
		"type" : "mh_set_motor_speed",
		"message0" : "Set Motor speed of %1 to %2",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "PORT",
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
		const port = generator.valueToCode(block, 'PORT', 0);

		const valueCode = generator.valueToCode(block, 'VALUE', 0);

		return "hub.setmotorspeed(" + port + "," + valueCode + ")\n";
	}
};
// clang-format on
