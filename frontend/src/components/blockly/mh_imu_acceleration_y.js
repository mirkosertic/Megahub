import {colorIMU} from './colors.js'

// clang-format off
export const definition = {
	category : 'IMU',
	colour : colorIMU,
	blockdefinition : {
		"type" : "mh_imu_acceleration_y",
		"message0" : "Get IMU Acceleration Y",
		"args0" : [
		],
		"output" : null,
		"colour" : colorIMU,
		"tooltip" : "Wait",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "imu.accelerationY()";
		return [ command, 0 ];
	}
};
// clang-format on
