#include "megahub.h"

extern Megahub *getMegaHubRef(lua_State *L);

int imu_yaw(lua_State *luaState) {

	DEBUG("Getting IMU yaw");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getYaw());

	return 1;
}

int imu_pitch(lua_State *luaState) {

	DEBUG("Getting IMU pitch");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getPitch());

	return 1;
}

int imu_roll(lua_State *luaState) {

	DEBUG("Getting IMU roll");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getRoll());

	return 1;
}

int imu_accelerationx(lua_State *luaState) {

	DEBUG("Getting IMU acceleration x");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationX());

	return 1;
}

int imu_accelerationy(lua_State *luaState) {

	DEBUG("Getting IMU acceleration y");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationY());

	return 1;
}

int imu_accelerationz(lua_State *luaState) {

	DEBUG("Getting IMU acceleration z");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationZ());

	return 1;
}

int imu_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{		  "yaw",			imu_yaw},
		{		 "pitch",		  imu_pitch},
		{		 "roll",			 imu_roll},
		{"accelerationX", imu_accelerationx},
		{"accelerationY", imu_accelerationy},
		{"accelerationZ", imu_accelerationz},
		{		   NULL,			   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
