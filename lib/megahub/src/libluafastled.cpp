#include "megahub.h"

#include <FastLED.h>

#define FASTLEDREF_NAME "FASTLEDREF"

extern Megahub *getMegaHubRef(lua_State *L);

int fastled_show(lua_State *luaState) {
	DEBUG("FastLED show");

	FastLED.show();

	return 0;
}

int fastled_clear(lua_State *luaState) {
	DEBUG("FastLED clear");

	FastLED.clear();

	return 0;
}

int fastled_set(lua_State *luaState) {
	DEBUG("FastLED set");

	int index = lua_tointeger(luaState, 1);
	int r = lua_tointeger(luaState, 2);
	int g = lua_tointeger(luaState, 3);
	int b = lua_tointeger(luaState, 4);

	lua_getfield(luaState, LUA_REGISTRYINDEX, FASTLEDREF_NAME);
	void **userdata = (void **) lua_touserdata(luaState, -1);
	lua_pop(luaState, 1);

	CRGB *leds = (CRGB *) (userdata ? *userdata : nullptr);
	if (leds != nullptr) {
		leds[index] = CRGB(r, g, b);
	} else {
		WARN("FastLED set called before addleds!");
	}

	return 0;
}

int fastled_addleds(lua_State *luaState) {
	DEBUG("FastLED addleds");

	int type = lua_tointeger(luaState, 1);
	int pin = lua_tointeger(luaState, 2);
	int numleds = lua_tointeger(luaState, 3);

	if (type == NEOPIXEL_TYPE) {
		CRGB *leds = new CRGB[numleds];
		switch (pin) {
			case GPIO_NUM_13:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_13>(leds, numleds);
				break;
			case GPIO_NUM_16:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_16>(leds, numleds);
				break;
			case GPIO_NUM_17:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_17>(leds, numleds);
				break;
			case GPIO_NUM_25:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_25>(leds, numleds);
				break;
			case GPIO_NUM_26:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_26>(leds, numleds);
				break;
			case GPIO_NUM_27:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_27>(leds, numleds);
				break;
			case GPIO_NUM_32:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_32>(leds, numleds);
				break;
			case GPIO_NUM_33:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_33>(leds, numleds);
				break;
			default:
				WARN("Unsupported pin %d for FastLED!");
				delete leds;
				return 0;
		}

		void **userdata = (void **) lua_newuserdata(luaState, sizeof(void *));
		*userdata = leds;
		lua_setfield(luaState, LUA_REGISTRYINDEX, FASTLEDREF_NAME);

	} else {
		WARN("FastLED addleds called with unknown type %d", type);
	}
	FastLED.clear();

	return 0;
}

int fastled_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{	 "show",	 fastled_show},
		{	 "clear",	  fastled_clear},
		{"addleds", fastled_addleds},
		{	 "set",		fastled_set},
		{	 NULL,			   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
