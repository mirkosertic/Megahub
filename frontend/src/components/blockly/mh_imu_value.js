import {colorIMU} from './colors.js'

// clang-format off
export const definition = {
	category : 'IMU',
	colour : colorIMU,
	blockdefinition : {
		"type" : "mh_imu_value",
		"message0" : "Get %1 from IMU",
		"args0" : [
			{
				"type" : "field_dropdown",
				"name" : "VALUE",
				"options" : [
					["YAW", "YAW"],
					["PITCH", "PITCH"],
					["ROLL", "ROLL"],
					["ACCELERATION_X", "ACCELERATION_X" ],
					["ACCELERATION_Y", "ACCELERATION_Y"],
					["ACCELERATION_Z", "ACCELERATION_Z" ],
				]
			}
		],
		"output" : null,
		"colour" : colorIMU,
		"tooltip" : "Gets a value from the IMU",
		"helpUrl" : ""
	},
	generator : (block, generator) => {
		const value = block.getFieldValue('VALUE');
		const command = "imu.value(" + value + ")";
		return [ command, 0 ];
	}
};
// clang-format on
