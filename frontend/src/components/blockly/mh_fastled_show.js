import {colorFastLED} from './colors.js'

// clang-format off
export const definition = {
	category : 'FastLED',
	colour : colorFastLED,
	blockdefinition : {
		"type" : "mh_fastled_show",
		"message0" : "FastLED show",
		"args0" : [
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorFastLED,
		"tooltip" : "Shop current FastLED values to the LED strip",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		return "fastled.show()\n";
	}
};
// clang-format on
