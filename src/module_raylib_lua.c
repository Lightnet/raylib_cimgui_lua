// module_raylib_lua.c
#include "module_raylib_lua.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include "cimgui.h"
#include "cimgui_impl.h"
#include "rlgl.h"
#include "raymath.h"

#define igGetIO igGetIO_Nil

extern lua_State* g_lua_state;  // Declare global

// Global ImGui context pointer
static ImGuiContext* g_imgui_context = NULL;
static bool g_lua_initialized = false;

// Function to check if file exists
static int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Lua-C function to cleanup ImGui
static int lua_imgui_cleanup(lua_State* L) {
    if (!g_imgui_context) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    igDestroyContext(g_imgui_context);
    g_imgui_context = NULL;
    g_lua_initialized = false;
    
    lua_pushboolean(L, 1);
    return 1;
}

// Lua-C function to create a window
static int lua_imgui_begin_window(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    
    bool* p_open = NULL;
    bool open = true;  // Default for igBegin if p_open is provided
    void* ud = NULL;   // Declare ud here for function-wide scope
    
    // Handle optional p_open (second argument)
    if (lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2)) {
        // It's a userdata (bool pointer) - test and assign
        ud = luaL_testudata(L, 2, "imgui.bool");
        if (ud) {
            p_open = (bool*)ud;
            open = *p_open;  // Get current value
        } else {
            // If not userdata, treat as boolean value (for backward compat or simple use)
            open = lua_toboolean(L, 2);
            p_open = &open;  // Temporary local for igBegin
        }
    }
    
    ImGuiWindowFlags flags = luaL_optinteger(L, 3, 0);
    
    bool result = igBegin(title, p_open ? &open : NULL, flags);
    
    // Update the userdata if it was provided (now ud is in scope)
    if (p_open && ud) {
        *p_open = open;
    }
    
    lua_pushboolean(L, result);
    return 1;
}

// Lua-C function to end window
static int lua_imgui_end_window(lua_State* L) {
    igEnd();
    lua_pushboolean(L, 1);
    return 1;
}

// Lua-C function to add text
static int lua_imgui_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    igText("%s", text);
    return 0;
}

// Lua-C function to add formatted text
static int lua_imgui_text_formatted(lua_State* L) {
    const char* fmt = luaL_checkstring(L, 1);
    const char* args = luaL_checkstring(L, 2);
    igText(fmt, args);
    return 0;
}

// Lua-C function for slider float
static int lua_imgui_slider_float(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float* v = (float*)luaL_checkudata(L, 2, "imgui.float");
    if (!v) {
        luaL_error(L, "Second argument must be a float userdata");
        return 0;
    }
    
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);
    const char* format = luaL_optstring(L, 5, "%.3f");
    ImGuiSliderFlags flags = luaL_optinteger(L, 6, 0);
    
    bool changed = igSliderFloat(label, v, min, max, format, flags);
    lua_pushboolean(L, changed);
    return 1;
}

// Lua-C function to create float userdata
static int lua_imgui_create_float(lua_State* L) {
    float* f = (float*)lua_newuserdata(L, sizeof(float));
    *f = luaL_checknumber(L, 1);
    luaL_getmetatable(L, "imgui.float");
    lua_setmetatable(L, -2);
    return 1;
}

// Lua-C function to get float value
static int lua_imgui_get_float(lua_State* L) {
    float* f = (float*)luaL_checkudata(L, 1, "imgui.float");
    if (!f) {
        luaL_error(L, "Argument must be a float userdata");
        return 0;
    }
    lua_pushnumber(L, *f);
    return 1;
}

// Lua-C function for button
static int lua_imgui_button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float width = luaL_optnumber(L, 2, 0.0f);
    float height = luaL_optnumber(L, 3, 0.0f);
    
    bool pressed = false;
    if (width > 0.0f && height > 0.0f) {
        pressed = igButton(label, (ImVec2){width, height});
    } else {
        pressed = igButton(label, (ImVec2){0, 0});
    }
    
    lua_pushboolean(L, pressed);
    return 1;
}

// Lua-C function for checkbox
static int lua_imgui_checkbox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool* v = (bool*)luaL_checkudata(L, 2, "imgui.bool");
    if (!v) {
        luaL_error(L, "Second argument must be a bool userdata");
        return 0;
    }
    
    bool changed = igCheckbox(label, v);
    lua_pushboolean(L, changed);
    return 1;
}

// Lua-C function to create bool userdata
static int lua_imgui_create_bool(lua_State* L) {
    bool* b = (bool*)lua_newuserdata(L, sizeof(bool));
    *b = lua_toboolean(L, 1);
    luaL_getmetatable(L, "imgui.bool");
    lua_setmetatable(L, -2);
    return 1;
}

// Lua-C function to get bool value
static int lua_imgui_get_bool(lua_State* L) {
    bool* b = (bool*)luaL_checkudata(L, 1, "imgui.bool");
    if (!b) {
        luaL_error(L, "Argument must be a bool userdata");
        return 0;
    }
    lua_pushboolean(L, *b);
    return 1;
}

// Lua-C function to load and execute Lua script
static int lua_load_script(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    
    if (!file_exists(filename)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "File does not exist");
        return 2;
    }
    
    int result = luaL_dofile(L, filename);
    if (result != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_pushboolean(L, 0);
        lua_pushstring(L, error);
        return 2;
    }
    
    lua_pushboolean(L, 1);
    lua_pushnil(L);
    return 2;
}

// Metatable for float userdata
static const luaL_Reg float_meta[] = {
    {"__tostring", lua_imgui_get_float},
    {NULL, NULL}
};

// Metatable for bool userdata
static const luaL_Reg bool_meta[] = {
    {"__tostring", lua_imgui_get_bool},
    {NULL, NULL}
};

// Lua module registration
static const luaL_Reg imgui_functions[] = {
    {"cleanup", lua_imgui_cleanup},
    {"begin_window", lua_imgui_begin_window},
    {"end_window", lua_imgui_end_window},
    {"text", lua_imgui_text},
    {"textf", lua_imgui_text_formatted},
    {"slider_float", lua_imgui_slider_float},
    {"button", lua_imgui_button},
    {"checkbox", lua_imgui_checkbox},
    {"create_float", lua_imgui_create_float},
    {"create_bool", lua_imgui_create_bool},
    {"load_script", lua_load_script},
    {NULL, NULL}
};

int luaopen_imgui(lua_State* L) {
    // Create metatables
    luaL_newmetatable(L, "imgui.float");
    luaL_setfuncs(L, float_meta, 0);
    lua_pop(L, 1);
    
    luaL_newmetatable(L, "imgui.bool");
    luaL_setfuncs(L, bool_meta, 0);
    lua_pop(L, 1);
    
    // Create imgui table
    luaL_newlib(L, imgui_functions);
    return 1;
}

// Implementations for header-declared functions (stubs/no-ops since handled in main.c)
void rl_lua_imgui_init(void) {
    // ImGui init handled in main.c; can add Lua-specific init here if needed
    printf("Lua ImGui init (noop)\n");
}

void rl_lua_imgui_new_frame(void) {
    // ImGui NewFrame already called in main.c; can add Lua-specific frame logic here
    // e.g., pass time or other globals to Lua if needed
}

void rl_lua_imgui_render(void) {
    // igRender already called in main.c; noop for now
}

void rl_lua_imgui_cleanup(void) {
    lua_imgui_cleanup(NULL);
}

void rl_lua_call_draw() {
    if (!g_lua_state) return;
    
    lua_getglobal(g_lua_state, "draw");
    if (lua_isfunction(g_lua_state, -1)) {
        if (lua_pcall(g_lua_state, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(g_lua_state, -1);
            printf("Lua draw error: %s\n", error);
            lua_pop(g_lua_state, 1);
        }
    } else {
        lua_pop(g_lua_state, 1);  // Clean up non-function
        printf("Warning: 'draw' function not found in Lua script.\n");
    }
    lua_settop(g_lua_state, 0);  // Clean stack
}

bool rl_lua_load_script(const char* filename) {
    lua_State* L = luaL_newstate();
    if (!L) return false;
    g_lua_state = L;
    
    luaL_openlibs(L);  // Open standard Lua libraries
    
    // Manually load the imgui module (bypasses require for static embedding)
    luaopen_imgui(L);  // Pushes the imgui table onto the stack
    lua_setglobal(L, "imgui");  // Sets _G.imgui = table (now accessible without require)
    
    // Ensure stack is clean
    lua_settop(L, 0);
    
    int result = luaL_dofile(L, filename);
    if (result == LUA_OK) {
        printf("Lua script '%s' loaded and executed successfully.\n", filename);
        
        // Optional: Check if draw function exists for later calls
        lua_getglobal(L, "draw");
        if (lua_isfunction(L, -1)) {
            printf("Draw function found in script.\n");
        } else {
            printf("Warning: No 'draw' function found in script.\n");
        }
        lua_pop(L, 1);  // Clean up
        return true;
    } else {
        const char* error = lua_tostring(L, -1);
        printf("Lua script error: %s\n", error);
        lua_pop(L, 1);
        lua_close(L);
        g_lua_state = NULL;
        return false;
    }
    // Note: State remains open for ongoing use (closed in main.c)
}