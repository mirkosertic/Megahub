#include "megahub.h"

#include <map>
#include <string>

extern Megahub *getMegaHubRef(lua_State *L);

// PID controller state structure
struct PIDState {
	double integral;
	double prevError;
	double prevTime;
};

// Global state storage for PID controllers
// Using std::map for automatic memory management
static std::map<std::string, PIDState> pidStates;

/**
 * PID Controller implementation
 *
 * Lua signature: alg.PID(blockId, setpoint, pv, kp, ki, kd, outMin, outMax)
 *
 * Parameters:
 *   blockId  - Unique identifier for this PID controller instance
 *   setpoint - Desired target value
 *   pv       - Process variable (current measurement)
 *   kp       - Proportional gain
 *   ki       - Integral gain
 *   kd       - Derivative gain
 *   outMin   - Minimum output value (clamping)
 *   outMax   - Maximum output value (clamping)
 *
 * Returns:
 *   Control output value (clamped to outMin..outMax range)
 */
int alg_pid(lua_State *luaState) {
	// Get parameters from Lua stack
	const char *blockId = luaL_checkstring(luaState, 1);
	double setpoint = luaL_checknumber(luaState, 2);
	double pv = luaL_checknumber(luaState, 3);
	double kp = luaL_checknumber(luaState, 4);
	double ki = luaL_checknumber(luaState, 5);
	double kd = luaL_checknumber(luaState, 6);
	double outMin = luaL_checknumber(luaState, 7);
	double outMax = luaL_checknumber(luaState, 8);

	double now = millis() / 1000.0; // Convert to seconds

	// Initialize state for this PID controller if it doesn't exist
	if (pidStates.find(blockId) == pidStates.end()) {
		pidStates[blockId] = {0.0, 0.0, now};
		DEBUG("PID controller '%s' initialized", blockId);
	}

	PIDState &state = pidStates[blockId];
	double dt = now - state.prevTime;

	// Prevent division by zero or negative dt
	// Also skip first iteration (dt too small)
	if (dt <= 0.001) { // 1ms minimum
		DEBUG("PID controller '%s': dt too small (%.6f), returning 0", blockId, dt);
		lua_pushnumber(luaState, 0.0);
		return 1;
	}

	// Calculate error
	double error = setpoint - pv;

	// Proportional term
	double pTerm = kp * error;

	// Integral term with anti-windup
	state.integral += error * dt;

	// Anti-windup: prevent integral from growing when output is saturated
	// Back-calculate integral limit based on current output
	double preOutput = pTerm + ki * state.integral;
	if (ki != 0.0) { // Avoid division by zero
		if (preOutput > outMax) {
			state.integral = (outMax - pTerm) / ki;
		} else if (preOutput < outMin) {
			state.integral = (outMin - pTerm) / ki;
		}
	}

	double iTerm = ki * state.integral;

	// Derivative term (derivative on measurement to avoid setpoint kick)
	double derivative = (error - state.prevError) / dt;
	double dTerm = kd * derivative;

	// Calculate total output
	double output = pTerm + iTerm + dTerm;

	// Clamp output to range
	if (output > outMax) {
		output = outMax;
	} else if (output < outMin) {
		output = outMin;
	}

	// Update state for next iteration
	state.prevError = error;
	state.prevTime = now;

	DEBUG("PID '%s': SP=%.2f PV=%.2f Err=%.2f P=%.2f I=%.2f D=%.2f Out=%.2f",
		blockId, setpoint, pv, error, pTerm, iTerm, dTerm, output);

	// Push result to Lua
	lua_pushnumber(luaState, output);
	return 1;
}

/**
 * Reset a PID controller's state
 *
 * Lua signature: alg.resetPID(blockId)
 *
 * Parameters:
 *   blockId - Unique identifier for the PID controller to reset
 */
int alg_reset_pid(lua_State *luaState) {
	const char *blockId = luaL_checkstring(luaState, 1);

	auto it = pidStates.find(blockId);
	if (it != pidStates.end()) {
		double now = millis() / 1000.0;
		it->second = {0.0, 0.0, now};
		DEBUG("PID controller '%s' reset", blockId);
	} else {
		WARN("PID controller '%s' not found for reset", blockId);
	}

	return 0;
}

/**
 * Clear all PID controller states
 * Useful for cleanup or when starting a new script
 *
 * Lua signature: alg.clearAllPID()
 */
int alg_clear_all_pid(lua_State *luaState) {
	size_t count = pidStates.size();
	pidStates.clear();
	DEBUG("Cleared %d PID controller states", count);
	return 0;
}

/**
 * Algorithm library registration
 * Exports the library as "alg" to Lua
 */
int alg_library(lua_State *luaState) {
	const luaL_Reg algfunctions[] = {
		{			"PID",			alg_pid},
		{	   "resetPID",	  alg_reset_pid},
		{	"clearAllPID", alg_clear_all_pid},
		{			 NULL,			   NULL}
	};
	luaL_newlib(luaState, algfunctions);
	return 1;
}
