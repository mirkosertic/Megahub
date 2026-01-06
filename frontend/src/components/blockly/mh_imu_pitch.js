import {colorIMU} from './colors.js'

// clang-format off
export const definition = {
	category : 'IMU',
	colour : colorIMU,
	blockdefinition : {
		"type" : "mh_imu_pitch",
		"message0" : "Get IMU Pitch",
		"args0" : [
		],
		"output" : null,
		"colour" : colorIMU,
		"tooltip" : "Wait",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "imu.pitch()";
		return [ command, 0 ];
	}
};
// clang-format on
