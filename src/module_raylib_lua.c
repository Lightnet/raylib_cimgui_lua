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
static int lua_imgui_begin(lua_State* L) {
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
    
    // Handle flags (third argument): int or table of strings/names
    ImGuiWindowFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiWindowFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            // Iterate over table and sum flag values by name
            lua_pushnil(L);  // First key
            while (lua_next(L, 3) != 0) {
                // Value at -1, key at -2
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    // Map name to value (add more mappings here for future flags)
                    static const struct {
                        const char* name;
                        ImGuiWindowFlags value;
                    } flag_map[] = {
                        {"WindowFlags_NoTitleBar", ImGuiWindowFlags_NoTitleBar},
                        {"WindowFlags_NoResize", ImGuiWindowFlags_NoResize},
                        {"WindowFlags_NoMove", ImGuiWindowFlags_NoMove},
                        {"WindowFlags_NoScrollbar", ImGuiWindowFlags_NoScrollbar},
                        {"WindowFlags_NoCollapse", ImGuiWindowFlags_NoCollapse},
                        {"WindowFlags_AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize},
                        {"WindowFlags_NoBackground", ImGuiWindowFlags_NoBackground},
                        {"WindowFlags_NoSavedSettings", ImGuiWindowFlags_NoSavedSettings},
                        {"WindowFlags_NoMouseInputs", ImGuiWindowFlags_NoMouseInputs},
                        {"WindowFlags_MenuBar", ImGuiWindowFlags_MenuBar},
                        {"WindowFlags_HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar},
                        {"WindowFlags_NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing},
                        {"WindowFlags_NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus},
                        {"WindowFlags_AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar},
                        {"WindowFlags_AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar},
                        {"WindowFlags_NoNavInputs", ImGuiWindowFlags_NoNavInputs},
                        {"WindowFlags_NoNavFocus", ImGuiWindowFlags_NoNavFocus},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);  // Pop value, keep key for next iteration
            }
        }
    }
    
    bool result = igBegin(title, p_open ? &open : NULL, flags);
    
    // Update the userdata if it was provided (now ud is in scope)
    if (p_open && ud) {
        *p_open = open;
    }
    
    lua_pushboolean(L, result);
    return 1;
}

// window end
// Lua-C function to end window
static int lua_imgui_end(lua_State* L) {
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
    int n = lua_gettop(L);
    if (n < 2) {
        igText("%s", fmt);
        return 0;
    }
    
    // Get string.format
    lua_getglobal(L, "string");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        igText("%s", fmt);
        return 0;
    }
    lua_getfield(L, -1, "format");
    lua_remove(L, -2);  // Remove string table
    
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        igText("%s", fmt);
        return 0;
    }
    
    // Push fmt as first arg
    lua_pushstring(L, fmt);
    
    // Push all additional args (from 2 to n)
    for (int i = 2; i <= n; ++i) {
        lua_pushvalue(L, i);
    }
    
    // Call string.format(fmt, args...)
    if (lua_pcall(L, n, 1, 0) != LUA_OK) {
        // Error in formatting, print and pop
        const char* err = lua_tostring(L, -1);
        printf("Text format error: %s\n", err);
        lua_pop(L, 1);
        // Fallback: use fmt as plain text
        igText("%s", fmt);
        return 0;
    }
    
    const char* formatted = lua_tostring(L, -1);
    lua_pop(L, 1);
    
    igText("%s", formatted);
    return 0;
}

// Lua-C function for slider float
static int lua_imgui_slider_float(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float v = (float)luaL_checknumber(L, 2);
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);
    const char* format = luaL_optstring(L, 5, "%.3f");
    ImGuiSliderFlags flags = luaL_optinteger(L, 6, 0);
    
    bool changed = igSliderFloat(label, &v, min, max, format, flags);
    lua_pushnumber(L, v);
    lua_pushboolean(L, changed);
    return 2;
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
    bool v = lua_toboolean(L, 2);
    
    bool changed = igCheckbox(label, &v);
    lua_pushboolean(L, v);
    lua_pushboolean(L, changed);
    return 2;
}

// Lua-C function to set dark style colors
static int lua_imgui_style_colors_dark(lua_State* L) {
    igStyleColorsDark(NULL);
    return 0;
}

// Lua-C function to set light style colors
static int lua_imgui_style_colors_light(lua_State* L) {
    igStyleColorsLight(NULL);
    return 0;
}

// Lua-C function to set classic style colors
static int lua_imgui_style_colors_classic(lua_State* L) {
    igStyleColorsClassic(NULL);
    return 0;
}

// Lua-C function to get ImGui version
static int lua_imgui_get_version(lua_State* L) {
    const char* version = igGetVersion();
    lua_pushstring(L, version);
    return 1;
}

// Lua-C function for menu bar
static int lua_imgui_begin_menu(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    
    bool enabled = true;  // Default
    if (lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2)) {
        enabled = lua_toboolean(L, 2) != 0;
    }
    
    bool result = igBeginMenu(label, enabled);
    lua_pushboolean(L, result);
    return 1;
}

//===============================================
// MENU
//===============================================

// Lua-C function for menu bar
static int lua_imgui_begin_menu_bar(lua_State* L) {
    bool result = igBeginMenuBar();
    lua_pushboolean(L, result);
    return 1;
}

// Lua-C function to end menu bar
static int lua_imgui_end_menu_bar(lua_State* L) {
    igEndMenuBar();
    return 0;
}

// Lua-C function for main menu bar
static int lua_imgui_begin_main_menu_bar(lua_State* L) {
    bool result = igBeginMainMenuBar();
    lua_pushboolean(L, result);
    return 1;
}

// Lua-C function to end main menu bar
static int lua_imgui_end_main_menu_bar(lua_State* L) {
    igEndMainMenuBar();
    return 0;
}

// Lua-C function to end menu
static int lua_imgui_end_menu(lua_State* L) {
    igEndMenu();
    return 0;
}

// Lua-C function for menu item
static int lua_imgui_menu_item(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    const char* shortcut = luaL_optstring(L, 2, NULL);
    
    bool selected = false;  // Default
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        selected = lua_toboolean(L, 3) != 0;
    }
    
    bool enabled = true;  // Default
    if (lua_gettop(L) >= 4 && !lua_isnoneornil(L, 4)) {
        enabled = lua_toboolean(L, 4) != 0;
    }
    
    bool activated = igMenuItem_Bool(label, shortcut, selected, enabled);
    lua_pushboolean(L, activated);
    return 1;
}

// Lua-C function to begin tooltip
static int lua_imgui_begin_tooltip(lua_State* L) {
    igBeginTooltip();
    return 0;
}

// Lua-C function to end tooltip
static int lua_imgui_end_tooltip(lua_State* L) {
    igEndTooltip();
    return 0;
}

// Lua-C function to set tooltip
static int lua_imgui_set_tooltip(lua_State* L) {
    const char* fmt = luaL_checkstring(L, 1);
    int n = lua_gettop(L);
    
    if (n == 1) {
        igSetTooltip("%s", fmt);
        return 0;
    }
    
    // Get string.format
    lua_getglobal(L, "string");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        igSetTooltip("%s", fmt);
        return 0;
    }
    lua_getfield(L, -1, "format");
    lua_remove(L, -2);
    
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        igSetTooltip("%s", fmt);
        return 0;
    }
    
    lua_pushstring(L, fmt);
    for (int i = 2; i <= n; ++i) {
        lua_pushvalue(L, i);
    }
    
    if (lua_pcall(L, n, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        printf("Tooltip format error: %s\n", err);
        lua_pop(L, 1);
        igSetTooltip("%s", fmt);
        return 0;
    }
    
    const char* formatted = lua_tostring(L, -1);
    lua_pop(L, 1);
    
    igSetTooltip("%s", formatted);
    return 0;
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

// Lua module registration
static const luaL_Reg imgui_functions[] = {
    // test
    {"cleanup", lua_imgui_cleanup},
    {"load_script", lua_load_script},
    // imgui
    {"Begin", lua_imgui_begin},
    {"End", lua_imgui_end},
    {"Text", lua_imgui_text},
    {"Textf", lua_imgui_text_formatted},
    {"SliderFloat", lua_imgui_slider_float},
    {"Button", lua_imgui_button},
    {"CheckBox", lua_imgui_checkbox},
    {"StyleColorsDark", lua_imgui_style_colors_dark},
    {"StyleColorsLight", lua_imgui_style_colors_light},
    {"StyleColorsClassic", lua_imgui_style_colors_classic},
    {"GetVersion", lua_imgui_get_version},
    {"BeginMenuBar", lua_imgui_begin_menu_bar},
    {"EndMenuBar", lua_imgui_end_menu_bar},
    {"BeginMainMenuBar", lua_imgui_begin_main_menu_bar},
    {"EndMainMenuBar", lua_imgui_end_main_menu_bar},
    {"BeginMenu", lua_imgui_begin_menu},
    {"EndMenu", lua_imgui_end_menu},
    {"MenuItem", lua_imgui_menu_item},
    {"BeginTooltip", lua_imgui_begin_tooltip},
    {"EndTooltip", lua_imgui_end_tooltip},
    {"SetTooltip", lua_imgui_set_tooltip},
    
    {NULL, NULL}
};

int luaopen_imgui(lua_State* L) {
    // Create imgui table
    luaL_newlib(L, imgui_functions);
    
    // Create WindowFlags sub-table
    lua_newtable(L);
    
    // Map common window flags as strings (matching flag_map in lua_imgui_begin)
    // Add more as needed for future flags
    struct {
        const char* full_name;   // Full string for lookup (value pushed)
        const char* lua_field;   // Clean field name for Lua access (key set)
        ImGuiWindowFlags value;  // Not used here, but for reference
    } flag_map[] = {
        {"WindowFlags_NoTitleBar", "NoTitleBar", ImGuiWindowFlags_NoTitleBar},
        {"WindowFlags_NoResize", "NoResize", ImGuiWindowFlags_NoResize},
        {"WindowFlags_NoMove", "NoMove", ImGuiWindowFlags_NoMove},
        {"WindowFlags_NoScrollbar", "NoScrollbar", ImGuiWindowFlags_NoScrollbar},
        {"WindowFlags_NoCollapse", "NoCollapse", ImGuiWindowFlags_NoCollapse},
        {"WindowFlags_AlwaysAutoResize", "AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize},
        {"WindowFlags_NoBackground", "NoBackground", ImGuiWindowFlags_NoBackground},
        {"WindowFlags_NoSavedSettings", "NoSavedSettings", ImGuiWindowFlags_NoSavedSettings},
        {"WindowFlags_NoMouseInputs", "NoMouseInputs", ImGuiWindowFlags_NoMouseInputs},
        {"WindowFlags_MenuBar", "MenuBar", ImGuiWindowFlags_MenuBar},  // 1024
        {"WindowFlags_HorizontalScrollbar", "HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar},
        {"WindowFlags_NoFocusOnAppearing", "NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing},
        {"WindowFlags_NoBringToFrontOnFocus", "NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus},
        {"WindowFlags_AlwaysVerticalScrollbar", "AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar},
        {"WindowFlags_AlwaysHorizontalScrollbar", "AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar},
        {"WindowFlags_NoNavInputs", "NoNavInputs", ImGuiWindowFlags_NoNavInputs},  // 65536
        {"WindowFlags_NoNavFocus", "NoNavFocus", ImGuiWindowFlags_NoNavFocus},     // 131072
        {"WindowFlags_NoDecoration", "NoDecoration", ImGuiWindowFlags_NoDecoration},
        {"WindowFlags_NoInputs", "NoInputs", ImGuiWindowFlags_NoInputs},
        {"WindowFlags_NoNav", "NoNav", ImGuiWindowFlags_NoNav},
        {NULL, NULL, 0}
    };
    
    for (int i = 0; flag_map[i].full_name != NULL; ++i) {
        lua_pushstring(L, flag_map[i].full_name);  // Push the full lookup string as value
        lua_setfield(L, -2, flag_map[i].lua_field);  // Set field with clean Lua name (e.g., "MenuBar")
    }
    
    // Set WindowFlags as a field in the main imgui table
    lua_setfield(L, -2, "WindowFlags");
    
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