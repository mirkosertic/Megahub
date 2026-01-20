import {colorGamepad} from './colors.js'

// clang-format off
export const definition = {
	category : 'Gamepad',
	colour : colorGamepad,
	blockdefinition : {
		"type" : "mh_gamepad_buttonpressed",
		"message0" : "Checks if %2 is pressed on %1",
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
				"name" : "BUTTON",
				"options" : [
					[ "GAMEPAD_BUTTON_1", "GAMEPAD_BUTTON_1" ],
				]
			}
		],
		"output": null,
		"colour" : colorGamepad,
		"tooltip" : "Checks if a specific button is pressed on a connected gamepad",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = block.getFieldValue('GAMEPAD');
		const button = block.getFieldValue('BUTTON');

		const command = "gamepad.buttonpressed(" + gamepad + "," + button + ")";

		return [ command, 0 ];
	}
};
// clang-format on
