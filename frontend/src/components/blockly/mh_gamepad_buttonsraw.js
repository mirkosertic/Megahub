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
		"type" : "mh_gamepad_buttonsraw",
		"message0" : "Gets the raw button values of %1",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "GAMEPAD",
			},
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Gets the raw button values of a Gamepad in as a 32bit integer",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = generator.valueToCode(block, 'GAMEPAD', 0);

		const command = "gamepad.buttonsraw(" + gamepad + ")";

		return [ command, 0 ];
	}
};
// clang-format on
