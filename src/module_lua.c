// module_lua.c
#include "module_lua.h"
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>

static lua_State* g_lua_state = NULL;

// Check if file exists
static int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

void lua_init(void) {
    if (g_lua_state) {
        printf("Lua state already initialized\n");
        return;
    }
    g_lua_state = luaL_newstate();
    if (!g_lua_state) {
        printf("Failed to create Lua state\n");
        return;
    }
    luaL_openlibs(g_lua_state); // Open standard Lua libraries
    printf("Lua state initialized\n");
}

bool lua_load_script(const char* filename) {
    if (!g_lua_state) {
        printf("Lua state not initialized\n");
        return false;
    }
    if (!file_exists(filename)) {
        printf("Lua script '%s' does not exist\n", filename);
        return false;
    }
    int result = luaL_dofile(g_lua_state, filename);
    if (result != LUA_OK) {
        const char* error = lua_tostring(g_lua_state, -1);
        printf("Lua script error: %s\n", error);
        lua_pop(g_lua_state, 1);
        return false;
    }
    printf("Lua script '%s' loaded successfully\n", filename);
    lua_getglobal(g_lua_state, "draw");
    if (lua_isfunction(g_lua_state, -1)) {
        printf("Draw function found in script\n");
    } else {
        printf("Warning: No 'draw' function found in script\n");
    }
    lua_pop(g_lua_state, 1);
    return true;
}

lua_State* lua_get_state(void) {
    return g_lua_state;
}

void lua_set_state(lua_State* L) {
    g_lua_state = L;
}

void lua_cleanup(void) {
    if (g_lua_state) {
        lua_close(g_lua_state);
        g_lua_state = NULL;
        printf("Lua state closed\n");
    }
}