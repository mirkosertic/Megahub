import {colorGamepad} from './colors.js'

// clang-format off
export const definition = {
	category : 'Gamepad',
	colour : colorGamepad,
	blockdefinition : {
		"type" : "mh_gamepad_gamepad",
		"message0" : "%1",
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
		"tooltip" : "A Gamepad connection",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const gamepad = block.getFieldValue('GAMEPAD');

		return [ gamepad, 0 ];
	}
};
// clang-format on
