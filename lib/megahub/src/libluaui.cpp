#include "commands.h"
#include "megahub.h"

#include <ArduinoJson.h>

extern Megahub *getMegaHubRef(lua_State *L);

int ui_show_value(lua_State *luaState) {

	DEBUG("UI show value called");
	JsonDocument doc;
	doc["type"] = "show_value";
	doc["label"] = lua_tostring(luaState, 1);

	int format = lua_tointeger(luaState, 2);
	if (format == FORMAT_SIMPLE) {
		doc["format"] = "simple";
	} else {
		doc["format"] = "unknown";
	}

	if (lua_isboolean(luaState, 3)) {
		bool boolvalue = lua_toboolean(luaState, 3);
		doc["value"] = boolvalue;
	} else if (lua_isnumber(luaState, 3)) {
		double numvalue = lua_tonumber(luaState, 3);
		doc["value"] = numvalue;
	} else if (lua_isstring(luaState, 3)) {
		const char *strvalue = lua_tostring(luaState, 3);
		doc["value"] = strvalue;
	} else {
		WARN("ui_show_value called with unsupported value type");
		doc["value"] = "unsupported";
	}

	String strCommand;
	serializeJson(doc, strCommand);

	DEBUG("UI show value command: %s", strCommand.c_str());

	// Enqueue command
	Commands::instance()->queue(strCommand);

	return 0;
}

int ui_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"showvalue", ui_show_value},
		{		 NULL,		   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
