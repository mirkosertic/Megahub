#include "megahub.h"

extern Megahub* getMegaHubRef(lua_State* L);

int lego_getdevicemode(lua_State* luaState) {

	int port = luaL_checkinteger(luaState, 1);

	DEBUG("Getting selected mode of port %d", port);

	Megahub* megahub = getMegaHubRef(luaState);
	LegoDevice* device = megahub->port(port);
	if (device != nullptr) {
		lua_pushinteger(luaState, device->getSelectedModeIndex());
	} else {
		WARN("Could not get device for port %d", port);
		lua_pushinteger(luaState, -1);
	}
	return 1;
}

int lego_getmodedataset(lua_State* luaState) {

	int port = luaL_checkinteger(luaState, 1);
	int modeIndex = luaL_checkinteger(luaState, 2);
	int dataset = luaL_checkinteger(luaState, 3);

	DEBUG("Getting dataset %d of mode %d of port %d", dataset, modeIndex, port);

	Megahub* megahub = getMegaHubRef(luaState);
	LegoDevice* device = megahub->port(port);
	if (device != nullptr) {
		if (modeIndex != -1) {
			Mode* mode = device->getMode(modeIndex);
			if (mode != nullptr) {
				Dataset* ds = mode->getDataset(dataset);
				if (ds != nullptr) {
					switch (ds->getType()) {
						case Format::FormatType::DATAFLOAT: {
							lua_pushnumber(luaState, ds->getDataAsFloat());
							return 1;
						}
						case Format::FormatType::DATA8: {
							lua_pushnumber(luaState, ds->getDataAsInt());
							return 1;
						}
						case Format::FormatType::DATA16: {
							lua_pushnumber(luaState, ds->getDataAsInt());
							return 1;
						}
						case Format::FormatType::DATA32: {
							lua_pushnumber(luaState, ds->getDataAsInt());
							return 1;
						}
						case Format::FormatType::UNKNOWN: {
							// Format was declared during handshake but no data frame has
							// arrived yet for this mode (e.g. COUNT on the Color+Distance
							// sensor is only sent spontaneously when the counter changes).
							// Return 0 silently — this is normal transient state, not an error.
							DEBUG("Mode %d dataset %d: no data received yet (UNKNOWN type), returning 0", modeIndex,
							      dataset);
							break;
						}
						default: {
							WARN("Unsupported data format for mode %d and dataset %d : %d", modeIndex, dataset,
							     static_cast<int>(ds->getType()));
						}
					}
				} else {
					WARN("Could not get dataset %d for mode %d of port %d", dataset, modeIndex, port);
				}
			} else {
				WARN("Could not get mode %d for port %d", modeIndex, port);
			}
		} else {
			WARN("Mode index is undefined : %d", modeIndex);
		}
	} else {
		WARN("Could not get device for port %d", port);
	}

	lua_pushinteger(luaState, 0);
	return 1;
}

int lego_select_mode(lua_State* luaState) {

	int port = luaL_checkinteger(luaState, 1);
	int mode = luaL_checkinteger(luaState, 2);

	DEBUG("Setting port %d to mode %d", port, mode);

	Megahub* megahub = getMegaHubRef(luaState);
	LegoDevice* device = megahub->port(port);
	if (device != nullptr) {
		device->selectMode(mode);
	} else {
		WARN("Could not get device for port %d", port);
	}

	lua_pushinteger(luaState, 0);
	return 1;

	return 0;
}

int lego_library(lua_State* luaState) {
	const luaL_Reg hubfunctions[] = {
	    { "getdevicemode",  lego_getdevicemode},
	    {"getmodedataset", lego_getmodedataset},
	    {    "selectmode",    lego_select_mode},
	    {	        NULL,	            NULL}
    };
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
