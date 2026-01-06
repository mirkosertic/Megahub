import {colorIMU} from './colors.js'

// clang-format off
export const definition = {
	category : 'IMU',
	colour : colorIMU,
	blockdefinition : {
		"type" : "mh_imu_roll",
		"message0" : "Get IMU Roll",
		"args0" : [
		],
		"output" : null,
		"colour" : colorIMU,
		"tooltip" : "Wait",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "imu.roll()";
		return [ command, 0 ];
	}
};
// clang-format on
