import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category : 'Control flow',
	colour : colorControlFlow,
	blockdefinition : {
        "type" : "mh_main_control_loop",
        "message0" : "Main control loop do %1",
        "args0" : [
			{
				"type" : "input_statement",
				"name" : "DO"
			}
		],
        "colour" : colorControlFlow,
        "tooltip" : "Main control loop",
        "helpUrl" : ""
	},
	generator : (block, generator) => {
		const doCode = generator.statementToCode(block, 'DO');

		return "hub.main_control_loop(function()\n" + doCode + "end)";
    }
};
// clang-format on
