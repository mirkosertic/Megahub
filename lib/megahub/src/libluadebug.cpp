#include "megahub.h"

extern Megahub *getMegaHubRef(lua_State *L);

int debug_freeheap(lua_State *luaState) {

	long value = ESP.getFreeHeap();
	DEBUG("Free heap is %ld", value);

	lua_pushnumber(luaState, value);

	return 1;
}

int debug_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{		 "freeHeap",		 debug_freeheap},
		{			  NULL,				   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
