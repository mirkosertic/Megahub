import {colorGamepad} from './colors.js'

// clang-format off
export const definition = {
	category : 'Gamepad',
	colour : colorGamepad,
	inputsForToolbox: {
		"GAMEPAD": {
          "shadow": {
            "type": "mh_gamepad_gamepad",
            "fields": {
              "GAMEPAD": "GAMEPAD1"
            }
          }
		}
	},
	blockdefinition : {
		"type" : "mh_gamepad_connected",
		"message0" : "Test if %1 is connected",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "GAMEPAD",
			},
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Tests if the Gamepad is connected",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = generator.valueToCode(block, 'GAMEPAD', 0);

		const command = "gamepad.connected(" + gamepad + ")";

		return [ command, 0 ];
	}
};
// clang-format on
