// ---------------------------------------------------------------------------
// Unit tests for Lua syntax validation — T6
// Uses Unity test framework (PlatformIO native environment)
//
// Run with: pio test -e native --filter test_lua
//
// Tests the luaL_loadstring() pattern used by Megahub::checkLUACode().
// The Lua 5.4 library at lib/lua/ is pure C and builds on host unchanged.
// ---------------------------------------------------------------------------

#include <unity.h>
#include <cstdio>
#include <cstring>

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

void setUp()    {}
void tearDown() {}

// ---------------------------------------------------------------------------
// Helper: attempt to load Lua code; return true on success.
// On failure, errorOut (if non-null) receives the error message.
// ---------------------------------------------------------------------------
static bool tryLoad(const char *code, const char **errorOut = nullptr) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    int result = luaL_loadstring(L, code);
    bool ok = (result == LUA_OK);

    if (!ok && errorOut != nullptr) {
        *errorOut = lua_tostring(L, -1);
    }

    lua_close(L);
    return ok;
}

// ---------------------------------------------------------------------------
// LUA-01: Valid simple assignment — must succeed
// ---------------------------------------------------------------------------
static void test_LUA01_valid_simple_lua() {
    TEST_ASSERT_TRUE(tryLoad("x = 1 + 2"));
}

// ---------------------------------------------------------------------------
// LUA-02: Invalid syntax (unclosed 'if') — must fail
// ---------------------------------------------------------------------------
static void test_LUA02_invalid_syntax_fails() {
    TEST_ASSERT_FALSE(tryLoad("if true then"));
}

// ---------------------------------------------------------------------------
// LUA-03: Empty string — valid Lua (no statements), must succeed
// ---------------------------------------------------------------------------
static void test_LUA03_empty_string_succeeds() {
    TEST_ASSERT_TRUE(tryLoad(""));
}

// ---------------------------------------------------------------------------
// LUA-04: Valid complex Lua with functions and loops — must succeed
// ---------------------------------------------------------------------------
static void test_LUA04_valid_complex_lua() {
    const char *code =
        "function factorial(n)\n"
        "  if n <= 1 then return 1 end\n"
        "  return n * factorial(n - 1)\n"
        "end\n"
        "\n"
        "local sum = 0\n"
        "for i = 1, 10 do\n"
        "  sum = sum + i\n"
        "end\n";

    TEST_ASSERT_TRUE(tryLoad(code));
}

// ---------------------------------------------------------------------------
// LUA-05: Syntax error — error message must contain line number "3"
// ---------------------------------------------------------------------------
static void test_LUA05_error_message_contains_line_number() {
    const char *code =
        "x = 1\n"
        "y = 2\n"
        "if x < y\n";    // missing 'then' — error on or after line 3

    lua_State *L = luaL_newstate();
    int result = luaL_loadstring(L, code);
    TEST_ASSERT_NOT_EQUAL(LUA_OK, result);

    const char *msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    // Error message must contain "3" (the line with the syntax error)
    TEST_ASSERT_NOT_NULL(strstr(msg, "3"));

    lua_close(L);
}

// ---------------------------------------------------------------------------
// LUA-06: Valid Lua with multiple function definitions — must succeed
// ---------------------------------------------------------------------------
static void test_LUA06_multiple_functions() {
    const char *code =
        "function add(a, b) return a + b end\n"
        "function sub(a, b) return a - b end\n"
        "function mul(a, b) return a * b end\n"
        "local result = add(mul(3, 4), sub(10, 2))\n";

    TEST_ASSERT_TRUE(tryLoad(code));
}

// ---------------------------------------------------------------------------
// LUA-07: Missing 'end' for function — syntax error
// ---------------------------------------------------------------------------
static void test_LUA07_missing_end_fails() {
    const char *code =
        "function foo()\n"
        "  local x = 42\n";   // no 'end'

    TEST_ASSERT_FALSE(tryLoad(code));
}

// ---------------------------------------------------------------------------
// LUA-08: Valid Lua with table literals — must succeed
// ---------------------------------------------------------------------------
static void test_LUA08_table_literal_valid() {
    const char *code =
        "local t = {1, 2, 3, key = 'value'}\n"
        "local n = #t\n";

    TEST_ASSERT_TRUE(tryLoad(code));
}

// ---------------------------------------------------------------------------
// LUA-09: Error message is non-empty for invalid code
// ---------------------------------------------------------------------------
static void test_LUA09_error_message_not_empty() {
    lua_State *L = luaL_newstate();
    int result = luaL_loadstring(L, "this is not valid lua @@@");
    TEST_ASSERT_NOT_EQUAL(LUA_OK, result);

    const char *msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_GREATER_THAN(0, static_cast<int>(strlen(msg)));

    lua_close(L);
}

// ---------------------------------------------------------------------------
// LUA-10: Valid Lua with coroutines (Lua 5.4 feature) — must succeed
// ---------------------------------------------------------------------------
static void test_LUA10_coroutine_syntax_valid() {
    const char *code =
        "local co = coroutine.create(function()\n"
        "  coroutine.yield(1)\n"
        "  coroutine.yield(2)\n"
        "end)\n";

    TEST_ASSERT_TRUE(tryLoad(code));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_LUA01_valid_simple_lua);
    RUN_TEST(test_LUA02_invalid_syntax_fails);
    RUN_TEST(test_LUA03_empty_string_succeeds);
    RUN_TEST(test_LUA04_valid_complex_lua);
    RUN_TEST(test_LUA05_error_message_contains_line_number);
    RUN_TEST(test_LUA06_multiple_functions);
    RUN_TEST(test_LUA07_missing_end_fails);
    RUN_TEST(test_LUA08_table_literal_valid);
    RUN_TEST(test_LUA09_error_message_not_empty);
    RUN_TEST(test_LUA10_coroutine_syntax_valid);
    return UNITY_END();
}
