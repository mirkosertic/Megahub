import {colorGamepad} from './colors.js'

// clang-format off
export const definition = {
	category : 'Gamepad',
	colour : colorGamepad,
	blockdefinition : {
		"type" : "mh_gamepad_value",
		"message0" : "Get %2 from %1",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "GAMEPAD",
				"options" : [
					[ "GAMEPAD1", "GAMEPAD1" ],
				]
			},
			{
				"type" : "field_dropdown",
				"name" : "VALUE",
				"options" : [
					["GAMEPAD_LEFT_X", "GAMEPAD_LEFT_X"],
					["GAMEPAD_LEFT_Y", "GAMEPAD_LEFT_Y"],
					["GAMEPAD_RIGHT_X", "GAMEPAD_RIGHT_X"],
					[ "GAMEPAD_RIGHT_Y", "GAMEPAD_RIGHT_Y" ],
				]
			}
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Gets a value from a connected Gamepad",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = block.getFieldValue('GAMEPAD');
		const value = block.getFieldValue('VALUE');

		const command = "gamepad.value(" + gamepad + "," + value + ")";

		return [ command, 0 ];
	}
};
// clang-format on
