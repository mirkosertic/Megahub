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
		"type" : "mh_gamepad_buttonpressed",
		"message0" : "Checks if %2 is pressed on %1",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "GAMEPAD",
			},
			{
				"type" : "field_dropdown",
				"name" : "BUTTON",
				"options" : [
					[ "GAMEPAD_BUTTON_1", "GAMEPAD_BUTTON_1" ],
					[ "GAMEPAD_BUTTON_1", "GAMEPAD_BUTTON_2" ],
					[ "GAMEPAD_BUTTON_2", "GAMEPAD_BUTTON_3" ],
					[ "GAMEPAD_BUTTON_4", "GAMEPAD_BUTTON_4" ],
					[ "GAMEPAD_BUTTON_5", "GAMEPAD_BUTTON_5" ],
					[ "GAMEPAD_BUTTON_6", "GAMEPAD_BUTTON_6" ],
					[ "GAMEPAD_BUTTON_7", "GAMEPAD_BUTTON_7" ],
					[ "GAMEPAD_BUTTON_8", "GAMEPAD_BUTTON_8" ],
					[ "GAMEPAD_BUTTON_9", "GAMEPAD_BUTTON_9" ],
					[ "GAMEPAD_BUTTON_10", "GAMEPAD_BUTTON_10" ],
					[ "GAMEPAD_BUTTON_11", "GAMEPAD_BUTTON_11" ],
					[ "GAMEPAD_BUTTON_12", "GAMEPAD_BUTTON_12" ],
					[ "GAMEPAD_BUTTON_13", "GAMEPAD_BUTTON_13" ],
					[ "GAMEPAD_BUTTON_14", "GAMEPAD_BUTTON_14" ],
					[ "GAMEPAD_BUTTON_15", "GAMEPAD_BUTTON_15" ],
					[ "GAMEPAD_BUTTON_16", "GAMEPAD_BUTTON_16" ],																																																																											
				]
			}
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Checks if a specific button is pressed on a connected gamepad",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = generator.valueToCode(block, 'GAMEPAD', 0);

		const button = block.getFieldValue('BUTTON');

		const command = "gamepad.buttonpressed(" + gamepad + "," + button + ")";

		return [ command, 0 ];
	}
};
// clang-format on
