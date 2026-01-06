import {colorIMU} from './colors.js'

// clang-format off
export const definition = {
	category : 'IMU',
	colour : colorIMU,
	blockdefinition : {
		"type" : "mh_imu_acceleration_x",
		"message0" : "Get IMU Acceleration X",
		"args0" : [
		],
		"output" : null,
		"colour" : colorIMU,
		"tooltip" : "Wait",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const command = "imu.accelerationX()";
		return [ command, 0 ];
	}
};
// clang-format on
