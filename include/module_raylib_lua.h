// module_raylib_lua.h
#ifndef MODULE_RAYLIB_LUA_H
#define MODULE_RAYLIB_LUA_H

#include <stdbool.h>
#include <lua.h>  // For lua_State

void rl_lua_imgui_init(void);
void rl_lua_imgui_new_frame(void);
void rl_lua_imgui_render(void);
void rl_lua_imgui_cleanup(void);
bool rl_lua_load_script(const char* filename);
void rl_lua_call_draw(void);  // Added: Prototype for draw function call

extern lua_State* g_lua_state;  // Added: Extern declaration
#endif