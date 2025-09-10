// module_lua.h
#ifndef MODULE_LUA_H
#define MODULE_LUA_H

#include <lua.h>
#include <stdbool.h>

void lua_init(void);
bool lua_load_script(const char* filename);
lua_State* lua_get_state(void);
void lua_set_state(lua_State* L);
void lua_cleanup(void);

#endif