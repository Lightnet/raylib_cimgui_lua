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
#include <float.h>

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

// Lua-C function to set custom ImGui style colors
static int lua_imgui_set_style_custom(lua_State* L) {
    const char* col_name = luaL_checkstring(L, 1); // First argument: ImGuiCol_ name
    luaL_checktype(L, 2, LUA_TTABLE); // Second argument: color array

    ImGuiStyle* style = igGetStyle(); // Get the current ImGui style
    if (!style) {
        luaL_error(L, "Failed to get ImGui style");
        return 0;
    }

    // Map of ImGuiCol_ names to enum values, matching luaopen_imgui Col sub-table
    static const struct {
        const char* name;
        ImGuiCol value;
    } col_map[] = {
        {"Col_Text", ImGuiCol_Text},
        {"Col_TextDisabled", ImGuiCol_TextDisabled},
        {"Col_WindowBg", ImGuiCol_WindowBg},
        {"Col_ChildBg", ImGuiCol_ChildBg},
        {"Col_PopupBg", ImGuiCol_PopupBg},
        {"Col_Border", ImGuiCol_Border},
        {"Col_BorderShadow", ImGuiCol_BorderShadow},
        {"Col_FrameBg", ImGuiCol_FrameBg},
        {"Col_FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"Col_FrameBgActive", ImGuiCol_FrameBgActive},
        {"Col_TitleBg", ImGuiCol_TitleBg},
        {"Col_TitleBgActive", ImGuiCol_TitleBgActive},
        {"Col_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"Col_MenuBarBg", ImGuiCol_MenuBarBg},
        {"Col_ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"Col_ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"Col_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"Col_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"Col_CheckMark", ImGuiCol_CheckMark},
        {"Col_SliderGrab", ImGuiCol_SliderGrab},
        {"Col_SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"Col_Button", ImGuiCol_Button},
        {"Col_ButtonHovered", ImGuiCol_ButtonHovered},
        {"Col_ButtonActive", ImGuiCol_ButtonActive},
        {"Col_Header", ImGuiCol_Header},
        {"Col_HeaderHovered", ImGuiCol_HeaderHovered},
        {"Col_HeaderActive", ImGuiCol_HeaderActive},
        {"Col_Separator", ImGuiCol_Separator},
        {"Col_SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"Col_SeparatorActive", ImGuiCol_SeparatorActive},
        {"Col_ResizeGrip", ImGuiCol_ResizeGrip},
        {"Col_ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"Col_ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"Col_Tab", ImGuiCol_Tab},
        {"Col_TabHovered", ImGuiCol_TabHovered},
        // {"Col_TabActive", ImGuiCol_TabActive},
        // {"Col_TabUnfocused", ImGuiCol_TabUnfocused},
        // {"Col_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"Col_PlotLines", ImGuiCol_PlotLines},
        {"Col_PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"Col_PlotHistogram", ImGuiCol_PlotHistogram},
        {"Col_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"Col_TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"Col_TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"Col_TableBorderLight", ImGuiCol_TableBorderLight},
        {"Col_TableRowBg", ImGuiCol_TableRowBg},
        {"Col_TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"Col_TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"Col_DragDropTarget", ImGuiCol_DragDropTarget},
        // {"Col_NavHighlight", ImGuiCol_NavHighlight},
        {"Col_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"Col_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"Col_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
        {NULL, 0}
    };

    printf("SetStyleCustom: Processing color key: %s\n", col_name);
    ImGuiCol col_idx = -1;
    for (int i = 0; col_map[i].name != NULL; ++i) {
        if (strcmp(col_name, col_map[i].name) == 0) {
            col_idx = col_map[i].value;
            break;
        }
    }

    if (col_idx == -1) {
        printf("SetStyleCustom: Error: Invalid ImGuiCol name '%s'\n", col_name);
        luaL_error(L, "Invalid ImGuiCol name: %s", col_name);
        return 0;
    }

    printf("SetStyleCustom: Value for %s is a table\n", col_name);
    ImVec4 color = {0.0f, 0.0f, 0.0f, 1.0f}; // Default to opaque black
    int len = lua_rawlen(L, 2);
    printf("SetStyleCustom: Color array length for %s: %d\n", col_name, len);
    if (len < 3 || len > 4) {
        printf("SetStyleCustom: Error: Invalid color array length %d for %s\n", len, col_name);
        luaL_error(L, "Color array for %s must have 3 or 4 components, got %d", col_name, len);
        return 0;
    }

    lua_geti(L, 2, 1);
    if (!lua_isnumber(L, -1)) {
        printf("SetStyleCustom: Error: Element 1 for %s is not a number\n", col_name);
        luaL_error(L, "Color element 1 for %s is not a number", col_name);
        lua_pop(L, 1);
        return 0;
    }
    color.x = (float)luaL_checknumber(L, -1);
    lua_geti(L, 2, 2);
    if (!lua_isnumber(L, -1)) {
        printf("SetStyleCustom: Error: Element 2 for %s is not a number\n", col_name);
        luaL_error(L, "Color element 2 for %s is not a number", col_name);
        lua_pop(L, 2);
        return 0;
    }
    color.y = (float)luaL_checknumber(L, -1);
    lua_geti(L, 2, 3);
    if (!lua_isnumber(L, -1)) {
        printf("SetStyleCustom: Error: Element 3 for %s is not a number\n", col_name);
        luaL_error(L, "Color element 3 for %s is not a number", col_name);
        lua_pop(L, 3);
        return 0;
    }
    color.z = (float)luaL_checknumber(L, -1);
    if (len == 4) {
        lua_geti(L, 2, 4);
        if (!lua_isnumber(L, -1)) {
            printf("SetStyleCustom: Error: Element 4 for %s is not a number\n", col_name);
            luaL_error(L, "Color element 4 for %s is not a number", col_name);
            lua_pop(L, 1);
            lua_pop(L, 3);
            return 0;
        }
        color.w = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 3);

    // Validate color values
    if (color.x < 0.0f || color.x > 1.0f || color.y < 0.0f || color.y > 1.0f ||
        color.z < 0.0f || color.z > 1.0f || color.w < 0.0f || color.w > 1.0f) {
        printf("SetStyleCustom: Error: Color values for %s out of range [0.0, 1.0]\n", col_name);
        luaL_error(L, "Color values for %s out of range [0.0, 1.0]", col_name);
        return 0;
    }

    style->Colors[col_idx] = color;
    printf("SetStyleCustom: Set style color %s: R=%.2f, G=%.2f, B=%.2f, A=%.2f\n",
           col_name, color.x, color.y, color.z, color.w);

    return 0; // No return value
}

// does not work.
// Lua-C function to set custom ImGui style colors in groups
static int lua_imgui_set_style_customs(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE); // Expect a table mapping ImGuiCol_ names to color arrays

    ImGuiStyle* style = igGetStyle(); // Get the current ImGui style
    if (!style) {
        luaL_error(L, "Failed to get ImGui style");
        return 0;
    }

    // Map of ImGuiCol_ names to enum values, matching luaopen_imgui Col sub-table
    static const struct {
        const char* name;
        ImGuiCol value;
    } col_map[] = {
        {"Col_Text", ImGuiCol_Text},
        {"Col_TextDisabled", ImGuiCol_TextDisabled},
        {"Col_WindowBg", ImGuiCol_WindowBg},
        {"Col_ChildBg", ImGuiCol_ChildBg},
        {"Col_PopupBg", ImGuiCol_PopupBg},
        {"Col_Border", ImGuiCol_Border},
        {"Col_BorderShadow", ImGuiCol_BorderShadow},
        {"Col_FrameBg", ImGuiCol_FrameBg},
        {"Col_FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"Col_FrameBgActive", ImGuiCol_FrameBgActive},
        {"Col_TitleBg", ImGuiCol_TitleBg},
        {"Col_TitleBgActive", ImGuiCol_TitleBgActive},
        {"Col_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"Col_MenuBarBg", ImGuiCol_MenuBarBg},
        {"Col_ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"Col_ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"Col_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"Col_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"Col_CheckMark", ImGuiCol_CheckMark},
        {"Col_SliderGrab", ImGuiCol_SliderGrab},
        {"Col_SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"Col_Button", ImGuiCol_Button},
        {"Col_ButtonHovered", ImGuiCol_ButtonHovered},
        {"Col_ButtonActive", ImGuiCol_ButtonActive},
        {"Col_Header", ImGuiCol_Header},
        {"Col_HeaderHovered", ImGuiCol_HeaderHovered},
        {"Col_HeaderActive", ImGuiCol_HeaderActive},
        {"Col_Separator", ImGuiCol_Separator},
        {"Col_SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"Col_SeparatorActive", ImGuiCol_SeparatorActive},
        {"Col_ResizeGrip", ImGuiCol_ResizeGrip},
        {"Col_ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"Col_ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"Col_Tab", ImGuiCol_Tab},
        {"Col_TabHovered", ImGuiCol_TabHovered},
        // {"Col_TabActive", ImGuiCol_TabActive},
        // {"Col_TabUnfocused", ImGuiCol_TabUnfocused},
        // {"Col_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"Col_PlotLines", ImGuiCol_PlotLines},
        {"Col_PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"Col_PlotHistogram", ImGuiCol_PlotHistogram},
        {"Col_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"Col_TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"Col_TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"Col_TableBorderLight", ImGuiCol_TableBorderLight},
        {"Col_TableRowBg", ImGuiCol_TableRowBg},
        {"Col_TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"Col_TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"Col_DragDropTarget", ImGuiCol_DragDropTarget},
        // {"Col_NavHighlight", ImGuiCol_NavHighlight},
        {"Col_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"Col_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"Col_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
        {NULL, 0}
    };

    // Iterate over the input table
    lua_pushnil(L); // First key
    while (lua_next(L, 1) != 0) {
        if (!lua_isstring(L, -2)) {
            printf("SetStyleCustoms: Error: Non-string key in table, got %s\n", luaL_typename(L, -2));
            lua_pop(L, 1); // Pop value, keep key for next iteration
            continue;
        }

        const char* col_name = lua_tostring(L, -2);
        printf("SetStyleCustoms: Processing color key: %s\n", col_name);
        ImGuiCol col_idx = -1;

        // Find the ImGuiCol_ enum value
        for (int i = 0; col_map[i].name != NULL; ++i) {
            if (strcmp(col_name, col_map[i].name) == 0) {
                col_idx = col_map[i].value;
                break;
            }
        }

        if (col_idx == -1) {
            printf("SetStyleCustoms: Error: Invalid ImGuiCol name '%s'\n", col_name);
            lua_pop(L, 1);
            continue;
        }

        if (!lua_istable(L, -1)) {
            printf("SetStyleCustoms: Error: Value for %s is not a table, got %s\n", col_name, luaL_typename(L, -1));
            lua_pop(L, 1);
            continue;
        }

        printf("SetStyleCustoms: Value for %s is a table\n", col_name);
        ImVec4 color = {0.0f, 0.0f, 0.0f, 1.0f}; // Default to opaque black
        int len = lua_rawlen(L, -1);
        printf("SetStyleCustoms: Color array length for %s: %d\n", col_name, len);
        if (len < 3 || len > 4) {
            printf("SetStyleCustoms: Error: Invalid color array length %d for %s\n", len, col_name);
            lua_pop(L, 1);
            continue;
        }

        lua_geti(L, -1, 1);
        if (!lua_isnumber(L, -1)) {
            printf("SetStyleCustoms: Error: Element 1 for %s is not a number\n", col_name);
            lua_pop(L, 1);
            lua_pop(L, 1);
            continue;
        }
        color.x = (float)luaL_checknumber(L, -1);
        lua_geti(L, -1, 2);
        if (!lua_isnumber(L, -1)) {
            printf("SetStyleCustoms: Error: Element 2 for %s is not a number\n", col_name);
            lua_pop(L, 2);
            lua_pop(L, 1);
            continue;
        }
        color.y = (float)luaL_checknumber(L, -1);
        lua_geti(L, -1, 3);
        if (!lua_isnumber(L, -1)) {
            printf("SetStyleCustoms: Error: Element 3 for %s is not a number\n", col_name);
            lua_pop(L, 3);
            lua_pop(L, 1);
            continue;
        }
        color.z = (float)luaL_checknumber(L, -1);
        if (len == 4) {
            lua_geti(L, -1, 4);
            if (!lua_isnumber(L, -1)) {
                printf("SetStyleCustoms: Error: Element 4 for %s is not a number\n", col_name);
                lua_pop(L, 1);
                lua_pop(L, 3);
                lua_pop(L, 1);
                continue;
            }
            color.w = (float)luaL_checknumber(L, -1);
            lua_pop(L, 1);
        }
        lua_pop(L, 3);

        // Validate color values
        if (color.x < 0.0f || color.x > 1.0f || color.y < 0.0f || color.y > 1.0f ||
            color.z < 0.0f || color.z > 1.0f || color.w < 0.0f || color.w > 1.0f) {
            printf("SetStyleCustoms: Error: Color values for %s out of range [0.0, 1.0]\n", col_name);
            lua_pop(L, 1);
            continue;
        }

        style->Colors[col_idx] = color;
        printf("SetStyleCustoms: Set style color %s: R=%.2f, G=%.2f, B=%.2f, A=%.2f\n",
               col_name, color.x, color.y, color.z, color.w);

        lua_pop(L, 1); // Pop value, keep key for next iteration
    }

    return 0; // No return value
}

// Lua-C function to get a custom ImGui style color
static int lua_imgui_get_style_custom(lua_State* L) {
    const char* col_name = luaL_checkstring(L, 1);

    ImGuiStyle* style = igGetStyle();
    if (!style) {
        luaL_error(L, "Failed to get ImGui style");
        return 0;
    }

    // Map of ImGuiCol_ names to enum values, matching luaopen_imgui Col sub-table
    static const struct {
        const char* name;
        ImGuiCol value;
    } col_map[] = {
        {"Col_Text", ImGuiCol_Text},
        {"Col_TextDisabled", ImGuiCol_TextDisabled},
        {"Col_WindowBg", ImGuiCol_WindowBg},
        {"Col_ChildBg", ImGuiCol_ChildBg},
        {"Col_PopupBg", ImGuiCol_PopupBg},
        {"Col_Border", ImGuiCol_Border},
        {"Col_BorderShadow", ImGuiCol_BorderShadow},
        {"Col_FrameBg", ImGuiCol_FrameBg},
        {"Col_FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"Col_FrameBgActive", ImGuiCol_FrameBgActive},
        {"Col_TitleBg", ImGuiCol_TitleBg},
        {"Col_TitleBgActive", ImGuiCol_TitleBgActive},
        {"Col_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"Col_MenuBarBg", ImGuiCol_MenuBarBg},
        {"Col_ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"Col_ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"Col_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"Col_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"Col_CheckMark", ImGuiCol_CheckMark},
        {"Col_SliderGrab", ImGuiCol_SliderGrab},
        {"Col_SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"Col_Button", ImGuiCol_Button},
        {"Col_ButtonHovered", ImGuiCol_ButtonHovered},
        {"Col_ButtonActive", ImGuiCol_ButtonActive},
        {"Col_Header", ImGuiCol_Header},
        {"Col_HeaderHovered", ImGuiCol_HeaderHovered},
        {"Col_HeaderActive", ImGuiCol_HeaderActive},
        {"Col_Separator", ImGuiCol_Separator},
        {"Col_SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"Col_SeparatorActive", ImGuiCol_SeparatorActive},
        {"Col_ResizeGrip", ImGuiCol_ResizeGrip},
        {"Col_ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"Col_ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"Col_Tab", ImGuiCol_Tab},
        {"Col_TabHovered", ImGuiCol_TabHovered},
        // {"Col_TabActive", ImGuiCol_TabActive},
        // {"Col_TabUnfocused", ImGuiCol_TabUnfocused},
        // {"Col_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"Col_PlotLines", ImGuiCol_PlotLines},
        {"Col_PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"Col_PlotHistogram", ImGuiCol_PlotHistogram},
        {"Col_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"Col_TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"Col_TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"Col_TableBorderLight", ImGuiCol_TableBorderLight},
        {"Col_TableRowBg", ImGuiCol_TableRowBg},
        {"Col_TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"Col_TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"Col_DragDropTarget", ImGuiCol_DragDropTarget},
        // {"Col_NavHighlight", ImGuiCol_NavHighlight},
        {"Col_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"Col_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"Col_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
        {NULL, 0}
    };

    ImGuiCol col_idx = -1;
    for (int i = 0; col_map[i].name != NULL; ++i) {
        if (strcmp(col_name, col_map[i].name) == 0) {
            col_idx = col_map[i].value;
            break;
        }
    }

    if (col_idx == -1) {
        luaL_error(L, "Invalid ImGuiCol name: %s", col_name);
        return 0;
    }

    ImVec4 color = style->Colors[col_idx];
    lua_newtable(L);
    lua_pushnumber(L, color.x);
    lua_seti(L, -2, 1); // R
    lua_pushnumber(L, color.y);
    lua_seti(L, -2, 2); // G
    lua_pushnumber(L, color.z);
    lua_seti(L, -2, 3); // B
    lua_pushnumber(L, color.w);
    lua_seti(L, -2, 4); // A

    return 1; // Return color table
}

// Start a tab bar
static int lua_imgui_begin_tab_bar(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool result = igBeginTabBar(label, 0); // Default flags
    lua_pushboolean(L, result);
    return 1;
}

// End a tab bar
static int lua_imgui_end_tab_bar(lua_State* L) {
    igEndTabBar();
    return 0;
}

// Start a tab item, returns true if selected
static int lua_imgui_begin_tab_item(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool open = true; // Default to open
    int top = lua_gettop(L);
    if (top >= 2 && !lua_isnil(L, 2)) {
        open = lua_toboolean(L, 2); // Optional open parameter
    }
    bool result = igBeginTabItem(label, &open, 0); // Default flags
    lua_pushboolean(L, result);
    lua_pushboolean(L, open); // Return modified open state
    return 2;
}

// End a tab item
static int lua_imgui_end_tab_item(lua_State* L) {
    igEndTabItem();
    return 0;
}

// Check if the last item is hovered
static int lua_imgui_is_item_hovered(lua_State* L) {
    bool result = igIsItemHovered(0); // Default flags
    lua_pushboolean(L, result);
    return 1;
}

// Check if the last item is active (being held/dragged)
static int lua_imgui_is_item_active(lua_State* L) {
    bool result = igIsItemActive();
    lua_pushboolean(L, result);
    return 1;
}

// Check if the last item was clicked
static int lua_imgui_is_item_clicked(lua_State* L) {
    int button = luaL_optinteger(L, 1, 0); // Default to left mouse button
    bool result = igIsItemClicked(button);
    lua_pushboolean(L, result);
    return 1;
}

// Place the next item on the same line
static int lua_imgui_same_line(lua_State* L) {
    float offset_from_start_x = luaL_optnumber(L, 1, 0.0f); // Optional offset
    float spacing_w = luaL_optnumber(L, 2, -1.0f); // Optional spacing
    igSameLine(offset_from_start_x, spacing_w);
    return 0;
}

// Draw a horizontal separator
static int lua_imgui_separator(lua_State* L) {
    igSeparator();
    return 0;
}

// Add vertical spacing
static int lua_imgui_spacing(lua_State* L) {
    igSpacing();
    return 0;
}

// Start a table
static int lua_imgui_begin_table(lua_State* L) {
    const char* str_id = luaL_checkstring(L, 1);
    int column = luaL_checkinteger(L, 2);
    int flags = luaL_optinteger(L, 3, 0); // Optional flags
    float outer_size_x = luaL_optnumber(L, 4, 0.0f); // Optional outer size x
    float outer_size_y = luaL_optnumber(L, 5, 0.0f); // Optional outer size y
    float inner_width = luaL_optnumber(L, 6, 0.0f); // Optional inner width
    bool result = igBeginTable(str_id, column, flags, (ImVec2){outer_size_x, outer_size_y}, inner_width);
    lua_pushboolean(L, result);
    return 1;
}

// End a table
static int lua_imgui_end_table(lua_State* L) {
    igEndTable();
    return 0;
}

// Advance to the next row in a table
static int lua_imgui_table_next_row(lua_State* L) {
    float min_row_height = luaL_optnumber(L, 1, 0.0f); // Optional min row height
    igTableNextRow(0, min_row_height); // Default flags
    return 0;
}

// Advance to the next column in the current row
static int lua_imgui_table_next_column(lua_State* L) {
    bool result = igTableNextColumn();
    lua_pushboolean(L, result);
    return 1;
}

// Set the current column index
static int lua_imgui_table_set_column_index(lua_State* L) {
    int column_n = luaL_checkinteger(L, 1);
    bool result = igTableSetColumnIndex(column_n);
    lua_pushboolean(L, result);
    return 1;
}

// Create an input text field
static int lua_imgui_input_text(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    size_t buffer_size = luaL_optinteger(L, 2, 256); // Default buffer size
    int flags = luaL_optinteger(L, 3, 0); // Optional flags
    
    // Allocate buffer for input text
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        luaL_error(L, "Failed to allocate buffer for InputText");
        return 0;
    }
    memset(buffer, 0, buffer_size);
    
    // Optional initial value
    if (lua_type(L, 4) == LUA_TSTRING) {
        const char* initial = lua_tostring(L, 4);
        strncpy(buffer, initial, buffer_size - 1);
    }
    
    bool result = igInputText(label, buffer, buffer_size, flags, NULL, NULL);
    lua_pushboolean(L, result); // Return whether the text was edited
    lua_pushstring(L, buffer); // Return the current text
    free(buffer);
    return 2;
}

// Multiline text input
static int lua_imgui_input_text_multiline(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    size_t buffer_size = luaL_optinteger(L, 2, 1024); // Default buffer size
    float size_x = luaL_optnumber(L, 3, 0.0f); // Optional size x
    float size_y = luaL_optnumber(L, 4, 0.0f); // Optional size y
    int flags = luaL_optinteger(L, 5, 0); // Optional flags
    
    // Allocate buffer for input text
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        luaL_error(L, "Failed to allocate buffer for InputTextMultiline");
        return 0;
    }
    memset(buffer, 0, buffer_size);
    
    // Optional initial value
    if (lua_type(L, 6) == LUA_TSTRING) {
        const char* initial = lua_tostring(L, 6);
        strncpy(buffer, initial, buffer_size - 1);
    }
    
    bool result = igInputTextMultiline(label, buffer, buffer_size, (ImVec2){size_x, size_y}, flags, NULL, NULL);
    lua_pushboolean(L, result); // Return whether the text was edited
    lua_pushstring(L, buffer); // Return the current text
    free(buffer);
    return 2;
}

// Radio button widget
static int lua_imgui_radio_button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int value = luaL_checkinteger(L, 2); // Current value
    int v_button = luaL_checkinteger(L, 3); // Value for this radio button
    bool result = igRadioButton_IntPtr(label, &value, v_button);
    lua_pushboolean(L, result); // Return whether the radio button was clicked
    lua_pushinteger(L, value); // Return the updated value
    return 2;
}

// Progress bar widget
static int lua_imgui_progress_bar(lua_State* L) {
    float fraction = luaL_checknumber(L, 1); // Progress fraction (0.0 to 1.0)
    float size_x = luaL_optnumber(L, 2, -1.0f); // Optional size x
    float size_y = luaL_optnumber(L, 3, 0.0f); // Optional size y
    const char* overlay = luaL_optstring(L, 4, NULL); // Optional overlay text
    igProgressBar(fraction, (ImVec2){size_x, size_y}, overlay);
    return 0;
}

// Tree node widget
static int lua_imgui_tree_node(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool result = igTreeNode_Str(label);
    lua_pushboolean(L, result); // Return whether the node is open
    return 1;
}

// End a tree node (call if TreeNode returns true)
static int lua_imgui_tree_pop(lua_State* L) {
    igTreePop();
    return 0;
}

// Bullet point
static int lua_imgui_bullet(lua_State* L) {
    igBullet();
    return 0;
}

// Colored text
static int lua_imgui_text_colored(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE); // Color table {r, g, b, a}
    const char* text = luaL_checkstring(L, 2);
    
    ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
    int len = lua_rawlen(L, 1);
    if (len < 3 || len > 4) {
        luaL_error(L, "Color array must have 3 or 4 components, got %d", len);
        return 0;
    }
    
    lua_geti(L, 1, 1);
    color.x = (float)luaL_checknumber(L, -1);
    lua_geti(L, 1, 2);
    color.y = (float)luaL_checknumber(L, -1);
    lua_geti(L, 1, 3);
    color.z = (float)luaL_checknumber(L, -1);
    if (len == 4) {
        lua_geti(L, 1, 4);
        color.w = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 3);
    
    if (color.x < 0.0f || color.x > 1.0f || color.y < 0.0f || color.y > 1.0f ||
        color.z < 0.0f || color.z > 1.0f || color.w < 0.0f || color.w > 1.0f) {
        luaL_error(L, "Color values out of range [0.0, 1.0]");
        return 0;
    }
    
    igTextColored(color, "%s", text);
    return 0;
}

// Combo box (dropdown menu)
static int lua_imgui_combo(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int current_item = luaL_checkinteger(L, 2); // Current selected item index (0-based)
    luaL_checktype(L, 3, LUA_TTABLE); // Items as a Lua table
    int flags = luaL_optinteger(L, 4, 0); // Optional flags

    // Get items from Lua table
    int item_count = lua_rawlen(L, 3);
    if (item_count == 0) {
        luaL_error(L, "Combo items table is empty");
        return 0;
    }

    // Allocate array for C strings
    const char** items = (const char**)malloc(item_count * sizeof(const char*));
    if (!items) {
        luaL_error(L, "Failed to allocate memory for Combo items");
        return 0;
    }

    // Populate items array
    for (int i = 0; i < item_count; i++) {
        lua_geti(L, 3, i + 1); // Lua tables are 1-based
        if (!lua_isstring(L, -1)) {
            free(items);
            luaL_error(L, "Item %d is not a string", i + 1);
            return 0;
        }
        items[i] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    bool changed = igCombo_Str_arr(label, &current_item, items, item_count, item_count);
    free(items);
    lua_pushboolean(L, changed); // Whether selection changed
    lua_pushinteger(L, current_item); // New selected index
    return 2;
}

// List box
static int lua_imgui_list_box(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int current_item = luaL_checkinteger(L, 2); // Current selected item index (0-based)
    luaL_checktype(L, 3, LUA_TTABLE); // Items as a Lua table
    float height_in_items = luaL_optnumber(L, 4, -1.0f); // Optional height in items

    // Get items from Lua table
    int item_count = lua_rawlen(L, 3);
    if (item_count == 0) {
        luaL_error(L, "ListBox items table is empty");
        return 0;
    }

    // Allocate array for C strings
    const char** items = (const char**)malloc(item_count * sizeof(const char*));
    if (!items) {
        luaL_error(L, "Failed to allocate memory for ListBox items");
        return 0;
    }

    // Populate items array
    for (int i = 0; i < item_count; i++) {
        lua_geti(L, 3, i + 1); // Lua tables are 1-based
        if (!lua_isstring(L, -1)) {
            free(items);
            luaL_error(L, "Item %d is not a string", i + 1);
            return 0;
        }
        items[i] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    bool changed = igListBox_Str_arr(label, &current_item, items, item_count, height_in_items);
    free(items);
    lua_pushboolean(L, changed); // Whether selection changed
    lua_pushinteger(L, current_item); // New selected index
    return 2;
}

// Plot lines
static int lua_imgui_plot_lines(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // Values as a Lua table
    const char* overlay_text = luaL_optstring(L, 3, NULL); // Optional overlay
    float scale_min = luaL_optnumber(L, 4, FLT_MAX); // Optional scale min
    float scale_max = luaL_optnumber(L, 5, FLT_MAX); // Optional scale max
    float graph_width = luaL_optnumber(L, 6, 0.0f); // Optional width
    float graph_height = luaL_optnumber(L, 7, 0.0f); // Optional height

    // Get values from Lua table
    int value_count = lua_rawlen(L, 2);
    if (value_count == 0) {
        luaL_error(L, "PlotLines values table is empty");
        return 0;
    }

    // Allocate array for values
    float* values = (float*)malloc(value_count * sizeof(float));
    if (!values) {
        luaL_error(L, "Failed to allocate memory for PlotLines values");
        return 0;
    }

    // Populate values array
    for (int i = 0; i < value_count; i++) {
        lua_geti(L, 2, i + 1); // Lua tables are 1-based
        if (!lua_isnumber(L, -1)) {
            free(values);
            luaL_error(L, "Value %d is not a number", i + 1);
            return 0;
        }
        values[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    igPlotLines_FloatPtr(label, values, value_count, 0, overlay_text, scale_min, scale_max, (ImVec2){graph_width, graph_height}, sizeof(float));
    free(values);
    return 0;
}

// Plot histogram
static int lua_imgui_plot_histogram(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // Values as a Lua table
    const char* overlay_text = luaL_optstring(L, 3, NULL); // Optional overlay
    float scale_min = luaL_optnumber(L, 4, FLT_MAX); // Optional scale min
    float scale_max = luaL_optnumber(L, 5, FLT_MAX); // Optional scale max
    float graph_width = luaL_optnumber(L, 6, 0.0f); // Optional width
    float graph_height = luaL_optnumber(L, 7, 0.0f); // Optional height

    // Get values from Lua table
    int value_count = lua_rawlen(L, 2);
    if (value_count == 0) {
        luaL_error(L, "PlotHistogram values table is empty");
        return 0;
    }

    // Allocate array for values
    float* values = (float*)malloc(value_count * sizeof(float));
    if (!values) {
        luaL_error(L, "Failed to allocate memory for PlotHistogram values");
        return 0;
    }

    // Populate values array
    for (int i = 0; i < value_count; i++) {
        lua_geti(L, 2, i + 1); // Lua tables are 1-based
        if (!lua_isnumber(L, -1)) {
            free(values);
            luaL_error(L, "Value %d is not a number", i + 1);
            return 0;
        }
        values[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    igPlotHistogram_FloatPtr(label, values, value_count, 0, overlay_text, scale_min, scale_max, (ImVec2){graph_width, graph_height}, sizeof(float));
    free(values);
    return 0;
}

// Vertical slider
static int lua_imgui_v_slider_float(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float v = (float)luaL_checknumber(L, 2); // Current value
    float v_min = (float)luaL_checknumber(L, 3); // Min value
    float v_max = (float)luaL_checknumber(L, 4); // Max value
    float size_x = luaL_optnumber(L, 5, 18.0f); // Optional size x
    float size_y = luaL_optnumber(L, 6, 160.0f); // Optional size y
    const char* format = luaL_optstring(L, 7, "%.3f"); // Optional format
    int flags = luaL_optinteger(L, 8, 0); // Optional flags

    bool changed = igVSliderFloat(label, (ImVec2){size_x, size_y}, &v, v_min, v_max, format, flags);
    lua_pushboolean(L, changed); // Whether value changed
    lua_pushnumber(L, v); // New value
    return 2;
}

// Begin a child window
static int lua_imgui_begin_child(lua_State* L) {
    const char* str_id = luaL_checkstring(L, 1);
    float size_x = luaL_optnumber(L, 2, 0.0f); // Optional size x
    float size_y = luaL_optnumber(L, 3, 0.0f); // Optional size y
    bool border = luaL_optinteger(L, 4, 0); // Optional border
    int flags = luaL_optinteger(L, 5, 0); // Optional flags
    bool result = igBeginChild_Str(str_id, (ImVec2){size_x, size_y}, border, flags);
    lua_pushboolean(L, result); // Return whether the child window is visible
    return 1;
}

// End a child window
static int lua_imgui_end_child(lua_State* L) {
    igEndChild();
    return 0;
}

// Check if the window is appearing
static int lua_imgui_is_window_appearing(lua_State* L) {
    bool result = igIsWindowAppearing();
    lua_pushboolean(L, result);
    return 1;
}

// Check if the window is collapsed
static int lua_imgui_is_window_collapsed(lua_State* L) {
    bool result = igIsWindowCollapsed();
    lua_pushboolean(L, result);
    return 1;
}

// Check if the window is focused
static int lua_imgui_is_window_focused(lua_State* L) {
    int flags = luaL_optinteger(L, 1, 0); // Optional flags
    bool result = igIsWindowFocused(flags);
    lua_pushboolean(L, result);
    return 1;
}

// Check if the window is hovered
static int lua_imgui_is_window_hovered(lua_State* L) {
    int flags = luaL_optinteger(L, 1, 0); // Optional flags
    bool result = igIsWindowHovered(flags);
    lua_pushboolean(L, result);
    return 1;
}

// Get the window position
static int lua_imgui_get_window_pos(lua_State* L) {
    ImVec2 pos;
    igGetWindowPos(&pos);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}

// Get the window size
static int lua_imgui_get_window_size(lua_State* L) {
    ImVec2 size;
    igGetWindowSize(&size);
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

// Get the window width
static int lua_imgui_get_window_width(lua_State* L) {
    float width = igGetWindowWidth();
    lua_pushnumber(L, width);
    return 1;
}

// Get the window height
static int lua_imgui_get_window_height(lua_State* L) {
    float height = igGetWindowHeight();
    lua_pushnumber(L, height);
    return 1;
}

// Collapsing header
static int lua_imgui_collapsing_header(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int flags = luaL_optinteger(L, 2, 0); // Optional flags
    bool result = igCollapsingHeader_TreeNodeFlags(label, flags);
    lua_pushboolean(L, result); // Return whether the header is open
    return 1;
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
    // New style functions
    {"SetStyleCustom", lua_imgui_set_style_custom},
    // {"SetStyleCustoms", lua_imgui_set_style_customs}, // Table-based, does not work.
    {"GetStyleCustom", lua_imgui_get_style_custom},
    // New tab functions
    {"BeginTabBar", lua_imgui_begin_tab_bar},
    {"EndTabBar", lua_imgui_end_tab_bar},
    {"BeginTabItem", lua_imgui_begin_tab_item},
    {"EndTabItem", lua_imgui_end_tab_item},

    {"IsItemHovered", lua_imgui_is_item_hovered},
    {"IsItemActive", lua_imgui_is_item_active},
    {"IsItemClicked", lua_imgui_is_item_clicked},
    {"SameLine", lua_imgui_same_line},
    {"Separator", lua_imgui_separator},
    {"Spacing", lua_imgui_spacing},
    {"BeginTable", lua_imgui_begin_table},
    {"EndTable", lua_imgui_end_table},
    {"TableNextRow", lua_imgui_table_next_row},
    {"TableNextColumn", lua_imgui_table_next_column},
    {"TableSetColumnIndex", lua_imgui_table_set_column_index},
    {"InputText", lua_imgui_input_text},

    {"InputTextMultiline", lua_imgui_input_text_multiline},
    {"RadioButton", lua_imgui_radio_button},
    {"ProgressBar", lua_imgui_progress_bar},
    {"TreeNode", lua_imgui_tree_node},
    {"TreePop", lua_imgui_tree_pop},
    {"Bullet", lua_imgui_bullet},
    {"TextColored", lua_imgui_text_colored},


    {"Combo", lua_imgui_combo},
    {"ListBox", lua_imgui_list_box},
    {"PlotLines", lua_imgui_plot_lines},
    {"PlotHistogram", lua_imgui_plot_histogram},
    {"VSliderFloat", lua_imgui_v_slider_float},

    {"BeginChild", lua_imgui_begin_child},
    {"EndChild", lua_imgui_end_child},
    {"IsWindowAppearing", lua_imgui_is_window_appearing},
    {"IsWindowCollapsed", lua_imgui_is_window_collapsed},
    {"IsWindowFocused", lua_imgui_is_window_focused},
    {"IsWindowHovered", lua_imgui_is_window_hovered},
    {"GetWindowPos", lua_imgui_get_window_pos},
    {"GetWindowSize", lua_imgui_get_window_size},
    {"GetWindowWidth", lua_imgui_get_window_width},
    {"GetWindowHeight", lua_imgui_get_window_height},
    {"CollapsingHeader", lua_imgui_collapsing_header},


    {NULL, NULL}
};

// table, variable flags
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

    // Create Col sub-table for ImGuiCol_ enums
    lua_newtable(L);
    struct {
        const char* full_name;
        const char* lua_field;
        ImGuiCol value;
    } col_map[] = {
        {"Col_Text", "Text", ImGuiCol_Text},
        {"Col_TextDisabled", "TextDisabled", ImGuiCol_TextDisabled},
        {"Col_WindowBg", "WindowBg", ImGuiCol_WindowBg},
        {"Col_ChildBg", "ChildBg", ImGuiCol_ChildBg},
        {"Col_PopupBg", "PopupBg", ImGuiCol_PopupBg},
        {"Col_Border", "Border", ImGuiCol_Border},
        {"Col_BorderShadow", "BorderShadow", ImGuiCol_BorderShadow},
        {"Col_FrameBg", "FrameBg", ImGuiCol_FrameBg},
        {"Col_FrameBgHovered", "FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"Col_FrameBgActive", "FrameBgActive", ImGuiCol_FrameBgActive},
        {"Col_TitleBg", "TitleBg", ImGuiCol_TitleBg},
        {"Col_TitleBgActive", "TitleBgActive", ImGuiCol_TitleBgActive},
        {"Col_TitleBgCollapsed", "TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"Col_MenuBarBg", "MenuBarBg", ImGuiCol_MenuBarBg},
        {"Col_ScrollbarBg", "ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"Col_ScrollbarGrab", "ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"Col_ScrollbarGrabHovered", "ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"Col_ScrollbarGrabActive", "ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"Col_CheckMark", "CheckMark", ImGuiCol_CheckMark},
        {"Col_SliderGrab", "SliderGrab", ImGuiCol_SliderGrab},
        {"Col_SliderGrabActive", "SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"Col_Button", "Button", ImGuiCol_Button},
        {"Col_ButtonHovered", "ButtonHovered", ImGuiCol_ButtonHovered},
        {"Col_ButtonActive", "ButtonActive", ImGuiCol_ButtonActive},
        {"Col_Header", "Header", ImGuiCol_Header},
        {"Col_HeaderHovered", "HeaderHovered", ImGuiCol_HeaderHovered},
        {"Col_HeaderActive", "HeaderActive", ImGuiCol_HeaderActive},
        {"Col_Separator", "Separator", ImGuiCol_Separator},
        {"Col_SeparatorHovered", "SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"Col_SeparatorActive", "SeparatorActive", ImGuiCol_SeparatorActive},
        {"Col_ResizeGrip", "ResizeGrip", ImGuiCol_ResizeGrip},
        {"Col_ResizeGripHovered", "ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"Col_ResizeGripActive", "ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"Col_Tab", "Tab", ImGuiCol_Tab},
        {"Col_TabHovered", "TabHovered", ImGuiCol_TabHovered},
        {"Col_HeaderActive", "HeaderActive", ImGuiCol_HeaderActive},
        {"Col_TabSelected", "TabSelected", ImGuiCol_TabSelected},
        // {"Col_TabUnfocusedActive", "TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"Col_PlotLines", "PlotLines", ImGuiCol_PlotLines},
        {"Col_PlotLinesHovered", "PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"Col_PlotHistogram", "PlotHistogram", ImGuiCol_PlotHistogram},
        {"Col_PlotHistogramHovered", "PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"Col_TableHeaderBg", "TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"Col_TableBorderStrong", "TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"Col_TableBorderLight", "TableBorderLight", ImGuiCol_TableBorderLight},
        {"Col_TableRowBg", "TableRowBg", ImGuiCol_TableRowBg},
        {"Col_TableRowBgAlt", "TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"Col_TextSelectedBg", "TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"Col_DragDropTarget", "DragDropTarget", ImGuiCol_DragDropTarget},
        {"Col_NavWindowingHighlight", "NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"Col_NavWindowingHighlight", "NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"Col_NavWindowingDimBg", "NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"Col_ModalWindowDimBg", "ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
        {NULL, NULL, 0}
    };
    
    for (int i = 0; col_map[i].full_name != NULL; ++i) {
        lua_pushstring(L, col_map[i].full_name);
        lua_setfield(L, -2, col_map[i].lua_field);
    }
    lua_setfield(L, -2, "Col");

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

// clean up
void cimgui_cleanup(void) {
    if (g_imgui_context) {
        igDestroyContext(g_imgui_context);
        g_imgui_context = NULL;
        g_lua_initialized = false;
    }
    g_lua_state = NULL;
    printf("cimgui module cleaned up\n");
}

// cimgui call draw
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
