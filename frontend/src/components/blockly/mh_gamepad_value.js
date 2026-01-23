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
		"type" : "mh_gamepad_value",
		"message0" : "Get %2 from %1",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "GAMEPAD",
			},
			{
				"type" : "field_dropdown",
				"name" : "VALUE",
				"options" : [
					["GAMEPAD_LEFT_X", "GAMEPAD_LEFT_X"],
					["GAMEPAD_LEFT_Y", "GAMEPAD_LEFT_Y"],
					["GAMEPAD_RIGHT_X", "GAMEPAD_RIGHT_X"],
					["GAMEPAD_RIGHT_Y", "GAMEPAD_RIGHT_Y"],
				]
			}
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Gets a value from a connected Gamepad",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = generator.valueToCode(block, 'GAMEPAD', 0);
		const value = block.getFieldValue('VALUE');

		const command = "gamepad.value(" + gamepad + "," + value + ")";

		return [ command, 0 ];
	}
};
// clang-format on
