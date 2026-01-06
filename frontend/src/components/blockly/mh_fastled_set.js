import {colorFastLED} from './colors.js'

// clang-format off
export const definition = {
	category : 'FastLED',
	colour : colorFastLED,
	blockdefinition : {
		"type" : "mh_fastled_set",
		"message0" : "Set LED #%1 to color %2",
		"args0" : [
			{
				"type" : "input_value",
				"name" : "INDEX"
			},
			{
				"type" : "field_colour",
				"name" : "COLOR",
				"colour" : "#00a000"
			},
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorFastLED,
		"tooltip" : "Set LED color",
		"helpUrl" : "",
		"inputsInline" : true
	},
	generator : (block, generator) => {
		const index = generator.valueToCode(block, 'INDEX', 0);
		const color = block.getFieldValue('COLOR');

		const num = parseInt(color.replace('#', ''), 16);

		const r = (num >> 16) & 255;
		const g = (num >> 8) & 255;
		const b = num & 255

		return "fastled.set(" + index + " ," + r + ", " + g + "," + b + ")\n";
	}
};
// clang-format on
