#include "megahub.h"

extern Megahub *getMegaHubRef(lua_State *L);

int imu_value(lua_State *luaState) {

	int value = lua_tointeger(luaState, 1);

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	DEBUG("Getting IMU value %d", value);
	if (value == YAW) {
		lua_pushnumber(luaState, imu->getYaw());
		return 1;
	} else if (value == PITCH) {
		lua_pushnumber(luaState, imu->getPitch());
		return 1;
	} else if (value == ROLL) {
		lua_pushnumber(luaState, imu->getRoll());
		return 1;
	} else if (value == ACCELERATION_X) {
		lua_pushnumber(luaState, imu->getAccelerationX());
		return 1;
	} else if (value == ACCELERATION_Y) {
		lua_pushnumber(luaState, imu->getAccelerationY());
		return 1;
	} else if (value == ACCELERATION_Z) {
		lua_pushnumber(luaState, imu->getAccelerationZ());
		return 1;
	}

	WARN("Not supported IMU value %d", value);
	lua_pushnumber(luaState, 0);
	return 1;
}

int imu_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"value", imu_value},
		{	 NULL,	   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
