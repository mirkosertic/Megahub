import {colorAlgorithms} from './colors.js'

// clang-format off
export const definition = {
	category : 'Algorithms',
	colour : colorAlgorithms,
	inputsForToolbox: {
		"SETPOINT": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 100
				}
			}
		},
		"PV": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 0
				}
			}
		},
		"KP": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 1.0
				}
			}
		},
		"KI": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 0.0
				}
			}
		},
		"KD": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 0.0
				}
			}
		},
		"OUT_MIN": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": -127
				}
			}
		},
		"OUT_MAX": {
			"shadow": {
				"type": "math_number",
				"fields": {
					"NUM": 128
				}
			}
		}
	},
	blockdefinition : {
		"type" : "mh_alg_pid",
		"message0" : "PID controller %1 setpoint: %2 PV: %3 \n%4 Kp: %5 Ki: %6 Kd: %7 %8 \noutput: %9 to %10",
		"args0" : [
			{
				"type" : "input_dummy"
			},
			{
				"type" : "input_value",
				"name" : "SETPOINT",
				"check" : "Number"
			},
			{
				"type" : "input_value",
				"name" : "PV",
				"check" : "Number"
			},
			{
				"type" : "input_dummy"
			},
			{
				"type" : "input_value",
				"name" : "KP",
				"check" : "Number"
			},
			{
				"type" : "input_value",
				"name" : "KI",
				"check" : "Number"
			},
			{
				"type" : "input_value",
				"name" : "KD",
				"check" : "Number"
			},
			{
				"type" : "input_dummy"
			},
			{
				"type" : "input_value",
				"name" : "OUT_MIN",
				"check" : "Number"
			},
			{
				"type" : "input_value",
				"name" : "OUT_MAX",
				"check" : "Number"
			}
		],
		"output" : "Number",
		"colour" : colorAlgorithms,
		"tooltip" : "Proportional-Integral-Derivative controller. Returns control output based on setpoint and process variable (PV).",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const setpoint = generator.valueToCode(block, 'SETPOINT', 0) || '0';
		const pv = generator.valueToCode(block, 'PV', 0) || '0';
		const kp = generator.valueToCode(block, 'KP', 0) || '1.0';
		const ki = generator.valueToCode(block, 'KI', 0) || '0.0';
		const kd = generator.valueToCode(block, 'KD', 0) || '0.0';
		const outMin = generator.valueToCode(block, 'OUT_MIN', 0) || '-100';
		const outMax = generator.valueToCode(block, 'OUT_MAX', 0) || '100';

		// Use block ID as unique identifier for state
		const blockId = `"PID_${block.id}"`;

		const command = `alg.PID(${blockId}, ${setpoint}, ${pv}, ${kp}, ${ki}, ${kd}, ${outMin}, ${outMax})`;

		return [ command, 0 ];
	}
};
// clang-format on
