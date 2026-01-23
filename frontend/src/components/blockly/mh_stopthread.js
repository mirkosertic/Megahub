import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category : 'Control flow',
	colour : colorControlFlow,
	blockdefinition : {
        "type" : "mh_stopthread",
        "message0" : "Stop thread %1",
        "args0" : [
			{
				"type": "input_value",
				"name": "HANDLE",
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
        "colour" : colorControlFlow,
        "tooltip" : "Stops a thread",
        "helpUrl" : "",
	},
	generator : (block, generator) => {
		const handle = generator.valueToCode(block, 'HANDLE', 0);
  
		return "hub.stopthread(" + handle +")\n";
    }
};
// clang-format on
