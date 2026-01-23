import {colorLego} from './colors.js'

// clang-format off
export const definition = {
	category : 'LEGO©',
	colour : colorLego,
	inputsForToolbox: {
		"PORT": {
          "shadow": {
            "type": "mh_port",
            "fields": {
              "PORT": "PORT1"
            }
          }
		}
	},
	blockdefinition : {
		"type" : "lego_select_mode",
		"message0" : "Set the mode of %1 to %2",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "PORT",
			},
			{
				"type" : "input_value",
				"name" : "MODE"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorLego,
		"tooltip" : "Sets the mode of a port",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const port = generator.valueToCode(block, 'PORT', 0);

		const modeCode = generator.valueToCode(block, 'MODE', 0);

		return "lego.selectmode(" + port + ", " + modeCode + ")\n";
	}
};
// clang-format on
