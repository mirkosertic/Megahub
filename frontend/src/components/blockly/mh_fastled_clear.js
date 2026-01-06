import {colorFastLED} from './colors.js'

// clang-format off
export const definition = {
	category : 'FastLED',
	colour : colorFastLED,
	blockdefinition : {
		"type" : "mh_fastled_clear",
		"message0" : "FastLED clear",
		"args0" : [
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorFastLED,
		"tooltip" : "Clear current FastLED values and sent them to the LED strip",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		return "fastled.clear()\n";
	}
};
// clang-format on
