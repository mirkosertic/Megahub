import {colorFastLED} from './colors.js'

// clang-format off
export const definition = {
	category : 'FastLED',
	colour : colorFastLED,
	blockdefinition : {
		"type" : "mh_fastled_addleds",
		"message0" : "Initialize FastLED of type %1 on pin %2 with %3 LEDs",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "TYPE",
				"options" : [
					[ "NEOPIXEL", "NEOPIXEL" ],
				]
			},
			{
				"type" : "field_dropdown",
				"name" : "PIN",
				"options" : [
					[ "GPIO13", "GPIO13" ],
					[ "GPIO16", "GPIO16" ],
					[ "GPIO17", "GPIO17" ],
					[ "GPIO25", "GPIO25" ],
					[ "GPIO26", "GPIO26" ],
					[ "GPIO27", "GPIO27" ],
					[ "GPIO32", "GPIO32" ],
					[ "GPIO33", "GPIO33" ],
				]
			},
			{
				"type" : "input_value",
				"name" : "NUM_LEDS"
			}
		],
		"previousStatement" : true,
		"nextStatement" : true,
		"colour" : colorFastLED,
		"tooltip" : "Initialize FastLED",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const type = block.getFieldValue('TYPE');
		const pin = block.getFieldValue('PIN');
		const valueCode = generator.valueToCode(block, 'NUM_LEDS', 0);

		return "fastled.addleds(" + type + "," + pin + "," + valueCode + ")\n";
	}
};
// clang-format on
