import {colorLego} from './colors.js'

// clang-format off
export const definition = {
	category : 'LEGO©',
	colour : colorLego,
	blockdefinition : {
		"type" : "lego_get_mode_dataset",
		"message0" : "Get dataset %2 from selected mode of %1",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "PORT",
				"options" : [
					[ "PORT1", "PORT1" ],
					[ "PORT2", "PORT2" ],
					[ "PORT3", "PORT3" ],
					[ "PORT4", "PORT4" ]
				]
			},
			{
				"type" : "input_value",
				"name" : "DATASET"
			}
		],
		"output" : null,
		"colour" : colorLego,
		"tooltip" : "Get a dataset value from the selected mode of a port",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const port = block.getFieldValue('PORT');

		const datasetCode = generator.valueToCode(block, 'DATASET', 0);

		const command = "lego.getmodedataset(" + port + "," + datasetCode + ")";
		return [ command, 0 ];
	}
};
// clang-format on
