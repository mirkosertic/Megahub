import {colorUI} from './colors.js'

// clang-format off
export const definition = {
	category : 'UI',
	colour : colorUI,
	inputsForToolbox: {
		"VALUE": {
          "shadow": {
            "type": "text",
            "fields": {
              "TEXT": "Replace me"
            }
          }
		}
	},	
	blockdefinition : {
		"type" : "ui_show_value",
		"message0" : "Show value %1: %2 with style %3",
		"args0" : [
			{
				"type" : "field_input",
				"name" : "LABEL",
				"text" : "Label"
			},
			{
				"type" : "input_value",
				"name" : "VALUE"
			},
			{
				"type" : "field_dropdown",
				"name" : "STYLE",
				"options" : [
					[ "FORMAT_SIMPLE", "FORMAT_SIMPLE" ],
				]
			},
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorUI,
		"tooltip" : "Show value on the UI",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const labelCode = block.getFieldValue('LABEL');
		const valueCode = generator.valueToCode(block, 'VALUE', 0);
		const styleCode = block.getFieldValue('STYLE');

		return "ui.showvalue(\"" + labelCode + "\", " + styleCode + ", " + valueCode + ")\n";
	}
};
// clang-format on
