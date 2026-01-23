import {colorControlFlow} from './colors.js'

// clang-format off
export const definition = {
	category : 'Control flow',
	colour : colorControlFlow,
	inputsForToolbox: {
		"STACKSIZE": {
          "shadow": {
            "type": "math_number",
            "fields": {
              "NUM": 4096
            }
          }
		}
	},
	blockdefinition : {
        "type" : "mh_startthread",
        "message0" : "Start thread %1 with stacksize %2 and profiling %3 and do %4",
        "args0" : [
			{
				"type" : "field_input",
				"name" : "NAME",
				"text" : "Name"
			},
			{
				"type": "input_value",
				"name": "STACKSIZE",
				"check": "Number",
			},
			{
				"type": "field_checkbox",
				"name": "PROFILING",
				"checked": true
			},
			{
				"type" : "input_statement",
				"name" : "DO"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
        "colour" : colorControlFlow,
        "tooltip" : "Starts a thread",
        "helpUrl" : "",
		"output": null,
	},
	generator : (block, generator) => {
		const name = block.getFieldValue('NAME');
		const stackSize = generator.valueToCode(block, 'STACKSIZE', 0);
		const profiling = block.getFieldValue('PROFILING') === "TRUE" ? true : false;

		const doCode = generator.statementToCode(block, 'DO');
		
		const outputConnection = block.outputConnection;
		const isUsedAsExpression = outputConnection && outputConnection.targetConnection;
  
		var code = "hub.startthread('" + name +"', '" + block.id + "', " + stackSize + ", " + profiling + ", function()\n" + doCode + "end)";
		if (isUsedAsExpression) {
  			return [code, 0];
  		}
		return code + "\n";
    }
};
// clang-format on
