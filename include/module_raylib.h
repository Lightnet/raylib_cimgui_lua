// module_raylib.h
#ifndef MODULE_RAYLIB_H
#define MODULE_RAYLIB_H

#include <lua.h>

void raylib_init(void);
int luaopen_raylib(lua_State *L);

#endif