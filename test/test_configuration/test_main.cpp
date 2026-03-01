// ---------------------------------------------------------------------------
// Unit tests for Configuration JSON parsing — T5
// Uses Unity test framework (PlatformIO native environment)
//
// Run with: pio test -e native --filter test_configuration
//
// Tests the ArduinoJson parsing patterns used by Configuration::load()
// in isolation — no FS, no WiFi, no Arduino String dependency.
// Reproduces the exact key-extraction logic from configuration.cpp.
// ---------------------------------------------------------------------------

#include <unity.h>
#include <cstdio>
#include <cstring>
#include <cstdbool>

// ---------------------------------------------------------------------------
// Logging stubs
// ---------------------------------------------------------------------------
#define INFO(msg, ...)  do { printf("[INFO] " msg "\n", ##__VA_ARGS__); } while(0)
#define WARN(msg, ...)  do { printf("[WARN] " msg "\n", ##__VA_ARGS__); } while(0)
#define ERROR(msg, ...) do { printf("[ERRO] " msg "\n", ##__VA_ARGS__); } while(0)
#define DEBUG(msg, ...) do {} while(0)

#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Inline reproduction of Configuration::load() key-extraction logic
//
// Parses a JSON string and populates the four fields that Configuration
// actually reads. Returns false if deserialization fails.
// ---------------------------------------------------------------------------
struct ConfigValues {
    bool btEnabled    = true;   // default: BT on
    bool wifiEnabled  = false;  // default: WiFi off
    const char *ssid  = "";
    const char *pwd   = "";
};

static bool parseConfig(const char *json, ConfigValues &out) {
    JsonDocument document;
    DeserializationError error = deserializeJson(document, json);
    if (error) {
        WARN("Could not read configuration: %s", error.c_str());
        return false;
    }

    if (document["bluetoothEnabled"].is<bool>()) {
        out.btEnabled = document["bluetoothEnabled"].as<bool>();
    }
    if (document["wifiEnabled"].is<bool>()) {
        out.wifiEnabled = document["wifiEnabled"].as<bool>();
    }
    if (document["ssid"].is<const char *>()) {
        out.ssid = document["ssid"].as<const char *>();
    }
    if (document["pwd"].is<const char *>()) {
        out.pwd = document["pwd"].as<const char *>();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------
void setUp()    {}
void tearDown() {}

// CFG-01: Full valid JSON — all four keys extracted correctly
static void test_CFG01_full_valid_json() {
    const char *json = R"({"bluetoothEnabled":true,"wifiEnabled":true,"ssid":"mynet","pwd":"pass123"})";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg.btEnabled);
    TEST_ASSERT_TRUE(cfg.wifiEnabled);
    TEST_ASSERT_EQUAL_STRING("mynet",   cfg.ssid);
    TEST_ASSERT_EQUAL_STRING("pass123", cfg.pwd);
}

// CFG-02: Only bluetoothEnabled present — other keys stay at defaults
static void test_CFG02_partial_json_defaults_retained() {
    const char *json = R"({"bluetoothEnabled":false})";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(cfg.btEnabled);
    TEST_ASSERT_FALSE(cfg.wifiEnabled);   // default
    TEST_ASSERT_EQUAL_STRING("", cfg.ssid); // default
    TEST_ASSERT_EQUAL_STRING("", cfg.pwd);  // default
}

// CFG-03: Wrong type for "bluetoothEnabled" (string instead of bool) → ignored, default kept
static void test_CFG03_wrong_type_ignored() {
    // is<bool>() returns false for a string value → key is skipped
    const char *json = R"({"bluetoothEnabled":"yes","wifiEnabled":1})";
    ConfigValues cfg;
    cfg.btEnabled   = true;   // set a known non-default so we can detect a bad override
    cfg.wifiEnabled = false;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg.btEnabled);    // unchanged because "yes" is not a bool
    TEST_ASSERT_FALSE(cfg.wifiEnabled); // unchanged because 1 is not a bool (ArduinoJson strict)
}

// CFG-04: Empty JSON object — all defaults, no error
static void test_CFG04_empty_json_object() {
    const char *json = "{}";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg.btEnabled);     // default
    TEST_ASSERT_FALSE(cfg.wifiEnabled);  // default
    TEST_ASSERT_EQUAL_STRING("", cfg.ssid);
    TEST_ASSERT_EQUAL_STRING("", cfg.pwd);
}

// CFG-05: Invalid JSON — deserialisation fails, function returns false
static void test_CFG05_invalid_json_returns_false() {
    const char *json = "{not valid json!!!}";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);
    TEST_ASSERT_FALSE(ok);
}

// CFG-06: wifiEnabled false explicitly — keeps false
static void test_CFG06_wifi_disabled_explicitly() {
    const char *json = R"({"bluetoothEnabled":true,"wifiEnabled":false,"ssid":"net","pwd":"pw"})";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(cfg.btEnabled);
    TEST_ASSERT_FALSE(cfg.wifiEnabled);
    TEST_ASSERT_EQUAL_STRING("net", cfg.ssid);
    TEST_ASSERT_EQUAL_STRING("pw",  cfg.pwd);
}

// CFG-07: bluetoothEnabled false — BT is disabled
static void test_CFG07_bt_disabled() {
    const char *json = R"({"bluetoothEnabled":false,"wifiEnabled":true})";
    ConfigValues cfg;
    bool ok = parseConfig(json, cfg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(cfg.btEnabled);
    TEST_ASSERT_TRUE(cfg.wifiEnabled);
}

// CFG-08: Empty JSON string (null terminator only) — deserialisation fails
static void test_CFG08_empty_string_returns_false() {
    ConfigValues cfg;
    bool ok = parseConfig("", cfg);
    TEST_ASSERT_FALSE(ok);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_CFG01_full_valid_json);
    RUN_TEST(test_CFG02_partial_json_defaults_retained);
    RUN_TEST(test_CFG03_wrong_type_ignored);
    RUN_TEST(test_CFG04_empty_json_object);
    RUN_TEST(test_CFG05_invalid_json_returns_false);
    RUN_TEST(test_CFG06_wifi_disabled_explicitly);
    RUN_TEST(test_CFG07_bt_disabled);
    RUN_TEST(test_CFG08_empty_string_returns_false);
    return UNITY_END();
}
