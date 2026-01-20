import {colorGamepad} from './colors.js'

// clang-format off
export const definition = {
	category : 'Gamepad',
	colour : colorGamepad,
	blockdefinition : {
		"type" : "mh_gamepad_connected",
		"message0" : "Test if %1 is connected",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "GAMEPAD",
				"options" : [
					[ "GAMEPAD1", "GAMEPAD1" ],
				]
			},
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Tests if the Gamepad is connected",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = block.getFieldValue('GAMEPAD');

		const command = "gamepad.connected(" + gamepad + ")";

		return [ command, 0 ];
	}
};
// clang-format on
