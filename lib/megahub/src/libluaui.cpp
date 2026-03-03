#include "commands.h"
#include "megahub.h"

#include <ArduinoJson.h>
#include <array>

extern Megahub* getMegaHubRef(lua_State* L);

// Map pose ring buffer for BLE throttling
struct MapPose {
	float posX, posY, heading;
};

static constexpr int mapBufferSize = 8;
static std::array<MapPose, mapBufferSize> mapBuffer;
static int mapBufferHead = 0;
static int mapBufferCount = 0;
static uint32_t lastMapSendMs = 0;
static constexpr uint32_t mapSendIntervalMs = 200;

int ui_show_value(lua_State* luaState) {

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
		const char* strvalue = lua_tostring(luaState, 3);
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

int ui_map_point(lua_State* luaState) {
	float posX = (float) luaL_checknumber(luaState, 1);
	float posY = (float) luaL_checknumber(luaState, 2);
	float heading = (float) luaL_checknumber(luaState, 3);

	// Push to ring buffer (newest-wins: overwrites oldest when full)
	mapBuffer[static_cast<size_t>(mapBufferHead)] = {posX, posY, heading};
	mapBufferHead = (mapBufferHead + 1) % mapBufferSize;
	if (mapBufferCount < mapBufferSize) {
		mapBufferCount++;
	}

	uint32_t now = millis();
	if ((now - lastMapSendMs) >= mapSendIntervalMs) {
		// Flush buffer as JSON batch
		JsonDocument doc;
		doc["type"] = "map_points";
		JsonArray points = doc["points"].to<JsonArray>();

		// Calculate start index (oldest entry)
		int startIdx = ((mapBufferHead - mapBufferCount) + mapBufferSize) % mapBufferSize;
		for (int i = 0; i < mapBufferCount; i++) {
			int idx = (startIdx + i) % mapBufferSize;
			JsonObject ptObj = points.add<JsonObject>();
			ptObj["x"] = mapBuffer[static_cast<size_t>(idx)].posX;
			ptObj["y"] = mapBuffer[static_cast<size_t>(idx)].posY;
			ptObj["h"] = mapBuffer[static_cast<size_t>(idx)].heading;
		}

		String strCommand;
		serializeJson(doc, strCommand);
		Commands::instance()->queue(strCommand);

		mapBufferCount = 0;
		mapBufferHead = 0;
		lastMapSendMs = now;
	}

	return 0;
}

int ui_map_clear(lua_State* luaState) {
	// Discard ring buffer
	mapBufferCount = 0;
	mapBufferHead = 0;

	// Send clear event immediately (bypass timer)
	JsonDocument doc;
	doc["type"] = "map_clear";
	String strCommand;
	serializeJson(doc, strCommand);
	Commands::instance()->queue(strCommand);
	lastMapSendMs = millis();

	return 0;
}

int ui_library(lua_State* luaState) {
	const luaL_Reg hubfunctions[] = {
	    {"showvalue", ui_show_value},
        { "mappoint",  ui_map_point},
        { "mapclear",  ui_map_clear},
        {       NULL,          NULL}
    };
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
