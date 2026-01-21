import {colorDebug} from './colors.js'

// clang-format off
export const definition = {
	category : 'Debug',
	colour : colorDebug,
	blockdefinition : {
		"type" : "mh_debug_millis",
		"message0" : "Get milliseconds since system startup",
		"args0" : [
		],
		"output" : null,
		"colour" : colorDebug,
		"tooltip" : "Get the number of milliseconds since system startup",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "millis()";

		return [ command, 0 ];
	}
};
// clang-format on
