#include "megahub.h"

#include <cmath>
#include <map>
#include <string>

extern Megahub* getMegaHubRef(lua_State* L);

// PID controller state structure
struct PIDState {
	double integral;
	double prevError;
	double prevTime;
};

// Global state storage for PID controllers
// Using std::map for automatic memory management
static std::map<std::string, PIDState> pidStates;
static int pidHandleCounter = 0;

// Dead Reckoning state structure
struct DRState {
	float x;       // meters, ROS REP 103 +X forward
	float y;       // meters, ROS REP 103 +Y left
	float heading; // radians, counterclockwise positive, stored internally
	int32_t prevLeftTicks;
	int32_t prevRightTicks;
	float prevYawDeg; // degrees, tracks previous IMU yaw for delta computation
	bool initialized; // false until first updateDR call
};

static std::map<std::string, DRState> drStates;
static int drHandleCounter = 0;
static portMUX_TYPE drMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * Initialize a new PID controller instance
 *
 * Lua signature: alg.initPID()
 *
 * Returns: handle string (e.g., "pid_0", "pid_1", ...)
 */
int alg_init_pid(lua_State* luaState) {
	std::string handle = "pid_" + std::to_string(pidHandleCounter++);
	double now = millis() / 1000.0;
	pidStates[handle] = PIDState{0.0, 0.0, now};
	lua_pushstring(luaState, handle.c_str());
	return 1;
}

/**
 * PID Controller computation with explicit handle
 *
 * Lua signature: alg.computePID(handle, setpoint, pv, kp, ki, kd, outMin, outMax)
 *
 * Parameters:
 *   handle   - Unique identifier for this PID controller instance (from initPID)
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
int alg_compute_pid(lua_State* luaState) {
	// Get parameters from Lua stack
	const char* handle = luaL_checkstring(luaState, 1);
	double setpoint = luaL_checknumber(luaState, 2);
	double pv = luaL_checknumber(luaState, 3);
	double kp = luaL_checknumber(luaState, 4);
	double ki = luaL_checknumber(luaState, 5);
	double kd = luaL_checknumber(luaState, 6);
	double outMin = luaL_checknumber(luaState, 7);
	double outMax = luaL_checknumber(luaState, 8);

	double now = millis() / 1000.0; // Convert to seconds

	// Check if state exists
	auto it = pidStates.find(handle);
	if (it == pidStates.end()) {
		WARN("PID controller '%s' not found - call initPID first", handle);
		lua_pushnumber(luaState, 0.0);
		return 1;
	}

	PIDState& state = it->second;
	double dt = now - state.prevTime;

	// Prevent division by zero or negative dt
	// Also skip first iteration (dt too small)
	if (dt <= 0.001) { // 1ms minimum
		DEBUG("PID controller '%s': dt too small (%.6f), returning 0", handle, dt);
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

	DEBUG("PID '%s': SP=%.2f PV=%.2f Err=%.2f P=%.2f I=%.2f D=%.2f Out=%.2f", handle, setpoint, pv, error, pTerm, iTerm,
	      dTerm, output);

	// Push result to Lua
	lua_pushnumber(luaState, output);
	return 1;
}

/**
 * Reset a PID controller's state
 *
 * Lua signature: alg.resetPID(handle)
 *
 * Parameters:
 *   handle - Unique identifier for the PID controller to reset
 */
int alg_reset_pid(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);

	auto it = pidStates.find(handle);
	if (it != pidStates.end()) {
		double now = millis() / 1000.0;
		it->second = {0.0, 0.0, now};
		DEBUG("PID controller '%s' reset", handle);
	} else {
		WARN("PID controller '%s' not found for reset", handle);
	}

	return 0;
}

/**
 * Clear all PID controller states
 * Useful for cleanup or when starting a new script
 *
 * Lua signature: alg.clearAllPID()
 */
int alg_clear_all_pid(lua_State* luaState) {
	size_t count = pidStates.size();
	pidStates.clear();
	pidHandleCounter = 0;
	DEBUG("Cleared %d PID controller states", count);
	return 0;
}

/**
 * Initialize a new Dead Reckoning instance
 *
 * Lua signature: alg.initDR()
 *
 * Returns: handle string (e.g., "dr_0", "dr_1", ...)
 */
int alg_init_dr(lua_State* luaState) {
	taskENTER_CRITICAL(&drMux);
	std::string handle = "dr_" + std::to_string(drHandleCounter++);
	drStates[handle] = DRState{.x = 0.0f,
	                           .y = 0.0f,
	                           .heading = 0.0f,
	                           .prevLeftTicks = 0,
	                           .prevRightTicks = 0,
	                           .prevYawDeg = 0.0f,
	                           .initialized = false};
	taskEXIT_CRITICAL(&drMux);

	lua_pushstring(luaState, handle.c_str());
	return 1;
}

/**
 * Dead Reckoning Update
 *
 * Lua signature: alg.updateDR(handle, leftTicks, rightTicks, yawDeg, wheelbase, mPerTick, imuWeight)
 */
int alg_update_dr(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);
	float leftTicks = (float) luaL_checknumber(luaState, 2);
	float rightTicks = (float) luaL_checknumber(luaState, 3);
	float yawDeg = (float) luaL_checknumber(luaState, 4);
	float wheelbase = (float) luaL_checknumber(luaState, 5);
	float mPerTick = (float) luaL_checknumber(luaState, 6);
	float imuWeight = (float) luaL_checknumber(luaState, 7);

	taskENTER_CRITICAL(&drMux);

	auto& state = drStates[handle];

	if (!state.initialized) {
		state.prevLeftTicks = (int32_t) leftTicks;
		state.prevRightTicks = (int32_t) rightTicks;
		state.prevYawDeg = yawDeg;
		state.x = state.y = state.heading = 0.0F;
		state.initialized = true;
		taskEXIT_CRITICAL(&drMux);
		return 0;
	}

	// 1. Wheel distances
	float deltaLeft = (leftTicks - (float) state.prevLeftTicks) * mPerTick;
	float deltaRight = (rightTicks - (float) state.prevRightTicks) * mPerTick;
	float dCenter = (deltaLeft + deltaRight) / 2.0F;

	// 2. Wheel-derived heading change
	float dThetaWheels = (wheelbase > 0.0F) ? (deltaRight - deltaLeft) / wheelbase : 0.0F;

	// 3. IMU-derived heading change with wrap-around normalization
	float dYawDeg = yawDeg - state.prevYawDeg;
	while (dYawDeg > 180.0F) {
		dYawDeg -= 360.0F;
	}
	while (dYawDeg < -180.0F) {
		dYawDeg += 360.0F;
	}
	float dThetaIMU = dYawDeg * static_cast<float>(M_PI / 180.0);

	// 4. Blend heading
	float dTheta = (1.0F - imuWeight) * dThetaWheels + imuWeight * dThetaIMU;

	// 5. Update heading
	state.heading += dTheta;

	// 6. Update position
	state.x += dCenter * cosf(state.heading);
	state.y += dCenter * sinf(state.heading);

	// 7. Store for next call
	state.prevLeftTicks = (int32_t) leftTicks;
	state.prevRightTicks = (int32_t) rightTicks;
	state.prevYawDeg = yawDeg;

	taskEXIT_CRITICAL(&drMux);

	return 0;
}

/**
 * Get a pose component from a DR state
 *
 * Lua signature: alg.drGet(handle, field)
 * field: "x", "y", or "heading"
 * Returns: float (x/y in meters, heading in degrees)
 */
int alg_dr_get(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);
	const char* field = luaL_checkstring(luaState, 2);

	taskENTER_CRITICAL(&drMux);
	auto it = drStates.find(handle);
	if (it == drStates.end()) {
		taskEXIT_CRITICAL(&drMux);
		WARN("drGet: handle '%s' not found", handle);
		lua_pushnumber(luaState, 0.0);
		return 1;
	}
	const DRState& state = it->second;
	float value = 0.0f;
	if (strcmp(field, "x") == 0) {
		value = state.x;
	} else if (strcmp(field, "y") == 0) {
		value = state.y;
	} else if (strcmp(field, "heading") == 0) {
		value = state.heading * (float) (180.0 / M_PI);
	} else {
		WARN("drGet: unknown field '%s'", field);
	}
	taskEXIT_CRITICAL(&drMux);

	lua_pushnumber(luaState, value);
	return 1;
}

/**
 * Reset a DR state to zero
 *
 * Lua signature: alg.drReset(handle)
 */
int alg_dr_reset(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);

	taskENTER_CRITICAL(&drMux);
	auto it = drStates.find(handle);
	if (it != drStates.end()) {
		it->second.x = 0.0f;
		it->second.y = 0.0f;
		it->second.heading = 0.0f;
		it->second.initialized = false; // re-bootstrap on next call
	} else {
		WARN("drReset: handle '%s' not found", handle);
	}
	taskEXIT_CRITICAL(&drMux);

	return 0;
}

/**
 * Inject a known pose into a DR state
 *
 * Lua signature: alg.drSetPose(handle, x, y, headingDeg)
 * x, y in meters; headingDeg in degrees
 */
int alg_dr_set_pose(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);
	float x = (float) luaL_checknumber(luaState, 2);
	float y = (float) luaL_checknumber(luaState, 3);
	float headingDeg = (float) luaL_checknumber(luaState, 4);

	taskENTER_CRITICAL(&drMux);
	auto& state = drStates[handle];
	state.x = x;
	state.y = y;
	state.heading = headingDeg * (float) (M_PI / 180.0);
	// Do NOT touch prevTicks or prevYawDeg — next updateDR computes deltas from current sensor values
	taskEXIT_CRITICAL(&drMux);

	return 0;
}

/**
 * Clear all DR states
 *
 * Lua signature: alg.clearAllDR()
 */
int alg_clear_all_dr(lua_State* luaState) {
	taskENTER_CRITICAL(&drMux);
	size_t count = drStates.size();
	drStates.clear();
	drHandleCounter = 0;
	taskEXIT_CRITICAL(&drMux);
	DEBUG("Cleared %d DR states", count);
	return 0;
}

// ---------- Moving Average ----------

static const int MAX_MA_WINDOW = 50;

struct MovingAvgState {
	float buf[MAX_MA_WINDOW];
	int head;
	int count;
	float sum;
};

static std::map<std::string, MovingAvgState> maStates;
static int maHandleCounter = 0;

/**
 * Initialize a new Moving Average filter instance
 *
 * Lua signature: alg.initMovingAvg()
 *
 * Returns: handle string (e.g., "ma_0", "ma_1", ...)
 */
int alg_init_moving_avg(lua_State* luaState) {
	std::string handle = "ma_" + std::to_string(maHandleCounter++);
	maStates[handle] = MovingAvgState{};
	lua_pushstring(luaState, handle.c_str());
	return 1;
}

/**
 * Moving Average filter computation with explicit handle
 *
 * Lua signature: alg.movingAvg(handle, value, windowSize)
 *
 * Parameters:
 *   handle     - Unique identifier for this filter instance (from initMovingAvg)
 *   value      - New sample to add
 *   windowSize - Number of samples to average (2–50; clamped at runtime)
 *
 * On first call the buffer is pre-filled with the initial value (no startup ramp).
 * Returns the arithmetic mean of the last N samples.
 */
int alg_moving_avg(lua_State* luaState) {
	const char* handle = luaL_checkstring(luaState, 1);
	float value = (float) luaL_checknumber(luaState, 2);
	int winSize = (int) luaL_checkinteger(luaState, 3);

	if (winSize < 2) {
		winSize = 2;
	}
	if (winSize > MAX_MA_WINDOW) {
		winSize = MAX_MA_WINDOW;
	}

	auto it = maStates.find(handle);
	if (it == maStates.end()) {
		WARN("movingAvg: handle '%s' not found - call initMovingAvg first", handle);
		lua_pushnumber(luaState, value);
		return 1;
	}

	MovingAvgState& s = it->second;

	// First call: count == 0 → pre-fill buffer
	if (s.count == 0) {
		s.count = winSize;
		s.head = 0;
		s.sum = value * winSize;
		for (int i = 0; i < winSize; ++i) {
			s.buf[i] = value;
		}
		lua_pushnumber(luaState, value);
		return 1;
	}

	// Window size changed → re-initialize
	if (s.count != winSize) {
		s.count = winSize;
		s.head = 0;
		s.sum = value * winSize;
		for (int i = 0; i < winSize; ++i) {
			s.buf[i] = value;
		}
		lua_pushnumber(luaState, value);
		return 1;
	}

	s.sum -= s.buf[s.head];
	s.buf[s.head] = value;
	s.sum += value;
	s.head = (s.head + 1) % winSize;

	lua_pushnumber(luaState, s.sum / winSize);
	return 1;
}

/**
 * Clear all Moving Average filter states
 *
 * Lua signature: alg.clearAllMovingAvg()
 */
int alg_clear_all_moving_avg(lua_State* luaState) {
	size_t count = maStates.size();
	maStates.clear();
	maHandleCounter = 0;
	DEBUG("Cleared %d moving average states", count);
	return 0;
}

// ---------- Map / Scale ----------

int alg_map(lua_State* luaState) {
	double value = luaL_checknumber(luaState, 1);
	double inMin = luaL_checknumber(luaState, 2);
	double inMax = luaL_checknumber(luaState, 3);
	double outMin = luaL_checknumber(luaState, 4);
	double outMax = luaL_checknumber(luaState, 5);

	if (inMax == inMin) {
		WARN("alg.map: inMax == inMin (%.4f), returning outMin", inMin);
		lua_pushnumber(luaState, outMin);
		return 1;
	}

	double result = outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
	lua_pushnumber(luaState, result);
	return 1;
}

/**
 * Reset all algorithm states.
 * Called by Megahub::executeLUACode() before each new program run to ensure
 * a clean slate. Not a Lua function — called directly from C++.
 */
void alg_reset_all_states() {
	pidStates.clear();
	pidHandleCounter = 0;

	taskENTER_CRITICAL(&drMux);
	drStates.clear();
	drHandleCounter = 0;
	taskEXIT_CRITICAL(&drMux);

	maStates.clear();
	maHandleCounter = 0;

	DEBUG("Algorithm states reset for new program run");
}

/**
 * Algorithm library registration
 * Exports the library as "alg" to Lua
 */
int alg_library(lua_State* luaState) {
	const luaL_Reg algfunctions[] = {
	    {	      "initPID",             alg_init_pid},
	    {       "computePID",          alg_compute_pid},
	    {	     "resetPID",            alg_reset_pid},
	    {      "clearAllPID",        alg_clear_all_pid},
	    {	       "initDR",              alg_init_dr},
	    {	     "updateDR",            alg_update_dr},
	    {	        "drGet",               alg_dr_get},
	    {	      "drReset",             alg_dr_reset},
	    {        "drSetPose",          alg_dr_set_pose},
	    {       "clearAllDR",         alg_clear_all_dr},
	    {    "initMovingAvg",      alg_init_moving_avg},
	    {        "movingAvg",           alg_moving_avg},
	    {"clearAllMovingAvg", alg_clear_all_moving_avg},
	    {	          "map",	              alg_map},
	    {	           NULL,	                 NULL}
    };
	luaL_newlib(luaState, algfunctions);
	return 1;
}
