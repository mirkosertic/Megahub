import {colorDebug} from './colors.js'

// clang-format off
export const definition = {
	category : 'Debug',
	colour : colorDebug,
	blockdefinition : {
		"type" : "mh_debug_loop_cycletime",
		"message0" : "Get average Lua cycle time",
		"args0" : [
		],
		"output" : null,
		"colour" : colorDebug,
		"tooltip" : "Get free average Lua main control loop cycle time",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "deb.luamaincycletime()";

		return [ command, 0 ];
	}
};
// clang-format on
