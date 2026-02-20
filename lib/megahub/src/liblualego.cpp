#include "megahub.h"

extern Megahub *getMegaHubRef(lua_State *L);

int lego_getmodedataset(lua_State *luaState) {

	int port = luaL_checkinteger(luaState, 1);
	int dataset = luaL_checkinteger(luaState, 2);

	DEBUG("Getting dataset %d of port %d", dataset, port);

	Megahub *megahub = getMegaHubRef(luaState);
	LegoDevice *device = megahub->port(port);
	if (device != nullptr) {
		int selectedMode = device->getSelectedModeIndex();
		if (selectedMode != -1) {
			Mode *mode = device->getMode(selectedMode);
			if (mode != nullptr) {
				Dataset *ds = mode->getDataset(dataset);
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
						default: {
							WARN("Unsupported data format for mode %d and dataset %d : %d", selectedMode, dataset, ds->getType());
						}
					}
				} else {
					WARN("Could not get dataset %d for selected mode of port %d", dataset, port);
				}
			} else {
				WARN("Could not get selected mode for port %d", port);
			}
		} else {
			WARN("Selected mode is undefined : %d", selectedMode);
		}
	} else {
		WARN("Could not get device for port %d", port);
	}

	lua_pushinteger(luaState, 0);
	return 1;
}

int lego_select_mode(lua_State *luaState) {

	int port = luaL_checkinteger(luaState, 1);
	int mode = luaL_checkinteger(luaState, 2);

	DEBUG("Setting port %d to mode %d", port, mode);

	Megahub *megahub = getMegaHubRef(luaState);
	LegoDevice *device = megahub->port(port);
	if (device != nullptr) {
		device->selectMode(mode);
	} else {
		WARN("Could not get device for port %d", port);
	}

	lua_pushinteger(luaState, 0);
	return 1;	

	return 0;
}

int lego_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"getmodedataset", lego_getmodedataset},
		{"selectmode", lego_select_mode},		
		{			NULL,				NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
