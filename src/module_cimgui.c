// module_raylib_lua.c
#include "module_cimgui.h"
#include "module_lua.h"
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

// extern lua_State* g_lua_state;  // Declare global

// Global ImGui context pointer
static ImGuiContext* g_imgui_context = NULL;
static bool g_lua_initialized = false;
static lua_State* g_lua_state = NULL; // Local Lua state

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

static int lua_imgui_color_edit3(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    
    // Expect a table with 3 numbers for col[3]
    luaL_checktype(L, 2, LUA_TTABLE);
    float col[3];
    for (int i = 0; i < 3; i++) {
        lua_geti(L, 2, i + 1);
        col[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    
    ImGuiColorEditFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiColorEditFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            lua_pushnil(L);
            while (lua_next(L, 3) != 0) {
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    static const struct {
                        const char* name;
                        ImGuiColorEditFlags value;
                    } flag_map[] = {
                        {"NoAlpha", ImGuiColorEditFlags_NoAlpha},
                        {"NoPicker", ImGuiColorEditFlags_NoPicker},
                        {"NoOptions", ImGuiColorEditFlags_NoOptions},
                        {"NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
                        {"NoInputs", ImGuiColorEditFlags_NoInputs},
                        {"NoTooltip", ImGuiColorEditFlags_NoTooltip},
                        {"NoLabel", ImGuiColorEditFlags_NoLabel},
                        {"NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
                        {"NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
                        {"AlphaBar", ImGuiColorEditFlags_AlphaBar},
                        // {"AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
                        {"AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
                        {"HDR", ImGuiColorEditFlags_HDR},
                        {"DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
                        {"DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
                        {"DisplayHex", ImGuiColorEditFlags_DisplayHex},
                        {"Uint8", ImGuiColorEditFlags_Uint8},
                        {"Float", ImGuiColorEditFlags_Float},
                        {"PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
                        {"PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
                        {"InputRGB", ImGuiColorEditFlags_InputRGB},
                        {"InputHSV", ImGuiColorEditFlags_InputHSV},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);
            }
        }
    }
    
    bool changed = igColorEdit3(label, col, flags);
    
    lua_newtable(L);
    for (int i = 0; i < 3; i++) {
        lua_pushnumber(L, col[i]);
        lua_seti(L, -2, i + 1);
    }
    
    lua_pushboolean(L, changed);
    return 2;
}

static int lua_imgui_color_edit4(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    
    // Expect a table with 4 numbers for col[4]
    luaL_checktype(L, 2, LUA_TTABLE);
    float col[4];
    for (int i = 0; i < 4; i++) {
        lua_geti(L, 2, i + 1);
        col[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    
    // Handle flags (third argument): int or table of strings
    ImGuiColorEditFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiColorEditFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            lua_pushnil(L);
            while (lua_next(L, 3) != 0) {
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    static const struct {
                        const char* name;
                        ImGuiColorEditFlags value;
                    } flag_map[] = {
                        {"NoAlpha", ImGuiColorEditFlags_NoAlpha},
                        {"NoPicker", ImGuiColorEditFlags_NoPicker},
                        {"NoOptions", ImGuiColorEditFlags_NoOptions},
                        {"NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
                        {"NoInputs", ImGuiColorEditFlags_NoInputs},
                        {"NoTooltip", ImGuiColorEditFlags_NoTooltip},
                        {"NoLabel", ImGuiColorEditFlags_NoLabel},
                        {"NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
                        {"NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
                        {"AlphaBar", ImGuiColorEditFlags_AlphaBar},
                        // {"AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
                        {"AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
                        {"HDR", ImGuiColorEditFlags_HDR},
                        {"DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
                        {"DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
                        {"DisplayHex", ImGuiColorEditFlags_DisplayHex},
                        {"Uint8", ImGuiColorEditFlags_Uint8},
                        {"Float", ImGuiColorEditFlags_Float},
                        {"PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
                        {"PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
                        {"InputRGB", ImGuiColorEditFlags_InputRGB},
                        {"InputHSV", ImGuiColorEditFlags_InputHSV},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);
            }
        }
    }
    
    bool changed = igColorEdit4(label, col, flags);
    
    // Push modified color values back as a table
    lua_newtable(L);
    for (int i = 0; i < 4; i++) {
        lua_pushnumber(L, col[i]);
        lua_seti(L, -2, i + 1);
    }
    
    lua_pushboolean(L, changed);
    return 2; // Return color table and changed flag
}

static int lua_imgui_color_picker3(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    
    // Expect a table with 3 numbers for col[3]
    luaL_checktype(L, 2, LUA_TTABLE);
    float col[3];
    for (int i = 0; i < 3; i++) {
        lua_geti(L, 2, i + 1);
        col[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    
    // Handle flags (third argument): int or table of strings
    ImGuiColorEditFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiColorEditFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            lua_pushnil(L);
            while (lua_next(L, 3) != 0) {
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    static const struct {
                        const char* name;
                        ImGuiColorEditFlags value;
                    } flag_map[] = {
                        {"NoAlpha", ImGuiColorEditFlags_NoAlpha},
                        {"NoPicker", ImGuiColorEditFlags_NoPicker},
                        {"NoOptions", ImGuiColorEditFlags_NoOptions},
                        {"NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
                        {"NoInputs", ImGuiColorEditFlags_NoInputs},
                        {"NoTooltip", ImGuiColorEditFlags_NoTooltip},
                        {"NoLabel", ImGuiColorEditFlags_NoLabel},
                        {"NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
                        {"NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
                        {"AlphaBar", ImGuiColorEditFlags_AlphaBar},
                        // {"AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
                        {"AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
                        {"HDR", ImGuiColorEditFlags_HDR},
                        {"DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
                        {"DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
                        {"DisplayHex", ImGuiColorEditFlags_DisplayHex},
                        {"Uint8", ImGuiColorEditFlags_Uint8},
                        {"Float", ImGuiColorEditFlags_Float},
                        {"PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
                        {"PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
                        {"InputRGB", ImGuiColorEditFlags_InputRGB},
                        {"InputHSV", ImGuiColorEditFlags_InputHSV},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);
            }
        }
    }
    
    bool changed = igColorPicker3(label, col, flags);
    
    // Push modified color values back as a table
    lua_newtable(L);
    for (int i = 0; i < 3; i++) {
        lua_pushnumber(L, col[i]);
        lua_seti(L, -2, i + 1);
    }
    
    lua_pushboolean(L, changed);
    return 2; // Return color table and changed flag
}

static int lua_imgui_color_picker4(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    
    // Expect a table with 4 numbers for col[4]
    luaL_checktype(L, 2, LUA_TTABLE);
    float col[4];
    for (int i = 0; i < 4; i++) {
        lua_geti(L, 2, i + 1);
        col[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    
    // Handle flags (third argument): int or table of strings
    ImGuiColorEditFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiColorEditFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            lua_pushnil(L);
            while (lua_next(L, 3) != 0) {
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    static const struct {
                        const char* name;
                        ImGuiColorEditFlags value;
                    } flag_map[] = {
                        {"NoAlpha", ImGuiColorEditFlags_NoAlpha},
                        {"NoPicker", ImGuiColorEditFlags_NoPicker},
                        {"NoOptions", ImGuiColorEditFlags_NoOptions},
                        {"NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
                        {"NoInputs", ImGuiColorEditFlags_NoInputs},
                        {"NoTooltip", ImGuiColorEditFlags_NoTooltip},
                        {"NoLabel", ImGuiColorEditFlags_NoLabel},
                        {"NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
                        {"NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
                        {"AlphaBar", ImGuiColorEditFlags_AlphaBar},
                        // {"AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
                        {"AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
                        {"HDR", ImGuiColorEditFlags_HDR},
                        {"DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
                        {"DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
                        {"DisplayHex", ImGuiColorEditFlags_DisplayHex},
                        {"Uint8", ImGuiColorEditFlags_Uint8},
                        {"Float", ImGuiColorEditFlags_Float},
                        {"PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
                        {"PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
                        {"InputRGB", ImGuiColorEditFlags_InputRGB},
                        {"InputHSV", ImGuiColorEditFlags_InputHSV},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);
            }
        }
    }
    
    // Optional ref_col (fourth argument, table of 4 floats or nil)
    float* ref_col = NULL;
    float ref_col_array[4];
    if (lua_gettop(L) >= 4 && lua_istable(L, 4)) {
        for (int i = 0; i < 4; i++) {
            lua_geti(L, 4, i + 1);
            ref_col_array[i] = (float)luaL_checknumber(L, -1);
            lua_pop(L, 1);
        }
        ref_col = ref_col_array;
    }
    
    bool changed = igColorPicker4(label, col, flags, ref_col);
    
    // Push modified color values back as a table
    lua_newtable(L);
    for (int i = 0; i < 4; i++) {
        lua_pushnumber(L, col[i]);
        lua_seti(L, -2, i + 1);
    }
    
    lua_pushboolean(L, changed);
    return 2; // Return color table and changed flag
}


static int lua_imgui_color_button(lua_State* L) {
    const char* desc_id = luaL_checkstring(L, 1);
    
    // Expect a table with 4 numbers for col (ImVec4)
    luaL_checktype(L, 2, LUA_TTABLE);
    ImVec4 col;
    lua_geti(L, 2, 1);
    col.x = (float)luaL_checknumber(L, -1);
    lua_geti(L, 2, 2);
    col.y = (float)luaL_checknumber(L, -1);
    lua_geti(L, 2, 3);
    col.z = (float)luaL_checknumber(L, -1);
    lua_geti(L, 2, 4);
    col.w = (float)luaL_checknumber(L, -1);
    lua_pop(L, 4);
    
    // Handle flags (third argument): int or table of strings
    ImGuiColorEditFlags flags = 0;
    if (lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3)) {
        if (lua_isnumber(L, 3)) {
            flags = (ImGuiColorEditFlags)lua_tointeger(L, 3);
        } else if (lua_istable(L, 3)) {
            lua_pushnil(L);
            while (lua_next(L, 3) != 0) {
                if (lua_isstring(L, -1)) {
                    const char* flag_name = lua_tostring(L, -1);
                    static const struct {
                        const char* name;
                        ImGuiColorEditFlags value;
                    } flag_map[] = {
                        {"NoAlpha", ImGuiColorEditFlags_NoAlpha},
                        {"NoPicker", ImGuiColorEditFlags_NoPicker},
                        {"NoOptions", ImGuiColorEditFlags_NoOptions},
                        {"NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
                        {"NoInputs", ImGuiColorEditFlags_NoInputs},
                        {"NoTooltip", ImGuiColorEditFlags_NoTooltip},
                        {"NoLabel", ImGuiColorEditFlags_NoLabel},
                        {"NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
                        {"NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
                        {"AlphaBar", ImGuiColorEditFlags_AlphaBar},
                        // {"AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
                        {"AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
                        {"HDR", ImGuiColorEditFlags_HDR},
                        {"DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
                        {"DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
                        {"DisplayHex", ImGuiColorEditFlags_DisplayHex},
                        {"Uint8", ImGuiColorEditFlags_Uint8},
                        {"Float", ImGuiColorEditFlags_Float},
                        {"PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
                        {"PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
                        {"InputRGB", ImGuiColorEditFlags_InputRGB},
                        {"InputHSV", ImGuiColorEditFlags_InputHSV},
                        {NULL, 0}
                    };
                    for (int i = 0; flag_map[i].name != NULL; ++i) {
                        if (strcmp(flag_name, flag_map[i].name) == 0) {
                            flags |= flag_map[i].value;
                            break;
                        }
                    }
                }
                lua_pop(L, 1);
            }
        }
    }
    
    // Optional size (fourth argument, table with 2 numbers or default to {0, 0})
    ImVec2 size = {0.0f, 0.0f};
    if (lua_gettop(L) >= 4 && lua_istable(L, 4)) {
        lua_geti(L, 4, 1);
        size.x = (float)luaL_checknumber(L, -1);
        lua_geti(L, 4, 2);
        size.y = (float)luaL_checknumber(L, -1);
        lua_pop(L, 2);
    }
    
    bool pressed = igColorButton(desc_id, col, flags, size);
    
    lua_pushboolean(L, pressed);
    return 1; // Return pressed flag
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


// Lua module registration
static const luaL_Reg imgui_functions[] = {
    // test
    {"cleanup", lua_imgui_cleanup},
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
    // New color functions
    {"ColorEdit3", lua_imgui_color_edit3},
    {"ColorEdit4", lua_imgui_color_edit4},
    {"ColorPicker3", lua_imgui_color_picker3},
    {"ColorPicker4", lua_imgui_color_picker4},
    {"ColorButton", lua_imgui_color_button},
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

    // Create ColorEditFlags sub-table
    lua_newtable(L);
    struct {
        const char* full_name;
        const char* lua_field;
        ImGuiColorEditFlags value;
    } color_flag_map[] = {
        {"ColorEditFlags_NoAlpha", "NoAlpha", ImGuiColorEditFlags_NoAlpha},
        {"ColorEditFlags_NoPicker", "NoPicker", ImGuiColorEditFlags_NoPicker},
        {"ColorEditFlags_NoOptions", "NoOptions", ImGuiColorEditFlags_NoOptions},
        {"ColorEditFlags_NoSmallPreview", "NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview},
        {"ColorEditFlags_NoInputs", "NoInputs", ImGuiColorEditFlags_NoInputs},
        {"ColorEditFlags_NoTooltip", "NoTooltip", ImGuiColorEditFlags_NoTooltip},
        {"ColorEditFlags_NoLabel", "NoLabel", ImGuiColorEditFlags_NoLabel},
        {"ColorEditFlags_NoSidePreview", "NoSidePreview", ImGuiColorEditFlags_NoSidePreview},
        {"ColorEditFlags_NoDragDrop", "NoDragDrop", ImGuiColorEditFlags_NoDragDrop},
        {"ColorEditFlags_AlphaBar", "AlphaBar", ImGuiColorEditFlags_AlphaBar},
        // {"ColorEditFlags_AlphaPreview", "AlphaPreview", ImGuiColorEditFlags_AlphaPreview},
        {"ColorEditFlags_AlphaPreviewHalf", "AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf},
        {"ColorEditFlags_HDR", "HDR", ImGuiColorEditFlags_HDR},
        {"ColorEditFlags_DisplayRGB", "DisplayRGB", ImGuiColorEditFlags_DisplayRGB},
        {"ColorEditFlags_DisplayHSV", "DisplayHSV", ImGuiColorEditFlags_DisplayHSV},
        {"ColorEditFlags_DisplayHex", "DisplayHex", ImGuiColorEditFlags_DisplayHex},
        {"ColorEditFlags_Uint8", "Uint8", ImGuiColorEditFlags_Uint8},
        {"ColorEditFlags_Float", "Float", ImGuiColorEditFlags_Float},
        {"ColorEditFlags_PickerHueBar", "PickerHueBar", ImGuiColorEditFlags_PickerHueBar},
        {"ColorEditFlags_PickerHueWheel", "PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel},
        {"ColorEditFlags_InputRGB", "InputRGB", ImGuiColorEditFlags_InputRGB},
        {"ColorEditFlags_InputHSV", "InputHSV", ImGuiColorEditFlags_InputHSV},
        {NULL, NULL, 0}
    };
    
    for (int i = 0; color_flag_map[i].full_name != NULL; ++i) {
        lua_pushstring(L, color_flag_map[i].full_name);
        lua_setfield(L, -2, color_flag_map[i].lua_field);
    }
    lua_setfield(L, -2, "ColorEditFlags");

    
    return 1;
}


// Implementations for header-declared functions (stubs/no-ops since handled in main.c)
void cimgui_init(void) {
    // Fetch Lua state from module_lua
    g_lua_state = lua_get_state();
    if (!g_lua_state) {
        printf("Error: No Lua state available in cimgui_init\n");
        return;
    }
    
    // Register the imgui module in Lua
    luaopen_imgui(g_lua_state);
    lua_setglobal(g_lua_state, "imgui"); // Make imgui table globally accessible
    lua_settop(g_lua_state, 0); // Clean stack
    
    printf("cimgui module initialized\n");
}

void cimgui_new_frame(void) {
    // No-op or add frame-specific logic
}

void cimgui_render(void) {
    // No-op or add render-specific logic
}

void cimgui_cleanup(void) {
    if (g_lua_state) {
        lua_imgui_cleanup(g_lua_state);
    }
    g_lua_state = NULL; // Clear local state
    g_lua_initialized = false;
    printf("cimgui module cleaned up\n");
}

void cimgui_call_draw(void) {
    if (!g_lua_state) {
        printf("Error: No Lua state in cimgui_call_draw\n");
        return;
    }
    
    lua_getglobal(g_lua_state, "draw");
    if (lua_isfunction(g_lua_state, -1)) {
        if (lua_pcall(g_lua_state, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(g_lua_state, -1);
            printf("Lua draw error: %s\n", error);
            lua_pop(g_lua_state, 1);
        }
    } else {
        lua_pop(g_lua_state, 1);
        printf("Warning: 'draw' function not found in Lua script\n");
    }
    lua_settop(g_lua_state, 0); // Clean stack
}
