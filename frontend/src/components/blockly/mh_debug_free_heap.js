import {colorDebug} from './colors.js'

// clang-format off
export const definition = {
	category : 'Debug',
	colour : colorDebug,
	blockdefinition : {
		"type" : "mh_debug_free_heap",
		"message0" : "Get free HEAP memory",
		"args0" : [
		],
		"output" : null,
		"colour" : colorDebug,
		"tooltip" : "Get free HEAP memory",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "deb.freeHeap()";

		return [ command, 0 ];
	}
};
// clang-format on
