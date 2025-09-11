// module_raylib.c
#include "module_raylib.h"
#include "module_lua.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "rlgl.h"
#include "raymath.h"
#include <GLFW/glfw3.h>

// Helper function to push a Vector3 to Lua
static void push_vector3(lua_State *L, Vector3 v) {
    lua_newtable(L);
    lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
}

// Helper function to get a Vector3 from Lua
static Vector3 get_vector3(lua_State *L, int index) {
    Vector3 v = {0};
    lua_getfield(L, index, "x"); v.x = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "y"); v.y = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "z"); v.z = lua_tonumber(L, -1); lua_pop(L, 1);
    return v;
}

// Helper function to push a Matrix to Lua
static void push_matrix(lua_State *L, Matrix m) {
    lua_newtable(L);
    lua_pushnumber(L, m.m0);  lua_setfield(L, -2, "m0");
    lua_pushnumber(L, m.m1);  lua_setfield(L, -2, "m1");
    lua_pushnumber(L, m.m2);  lua_setfield(L, -2, "m2");
    lua_pushnumber(L, m.m3);  lua_setfield(L, -2, "m3");
    lua_pushnumber(L, m.m4);  lua_setfield(L, -2, "m4");
    lua_pushnumber(L, m.m5);  lua_setfield(L, -2, "m5");
    lua_pushnumber(L, m.m6);  lua_setfield(L, -2, "m6");
    lua_pushnumber(L, m.m7);  lua_setfield(L, -2, "m7");
    lua_pushnumber(L, m.m8);  lua_setfield(L, -2, "m8");
    lua_pushnumber(L, m.m9);  lua_setfield(L, -2, "m9");
    lua_pushnumber(L, m.m10); lua_setfield(L, -2, "m10");
    lua_pushnumber(L, m.m11); lua_setfield(L, -2, "m11");
    lua_pushnumber(L, m.m12); lua_setfield(L, -2, "m12");
    lua_pushnumber(L, m.m13); lua_setfield(L, -2, "m13");
    lua_pushnumber(L, m.m14); lua_setfield(L, -2, "m14");
    lua_pushnumber(L, m.m15); lua_setfield(L, -2, "m15");
}

// Helper function to get a Matrix from Lua
static Matrix get_matrix(lua_State *L, int index) {
    Matrix m = {0};
    lua_getfield(L, index, "m0");  m.m0  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m1");  m.m1  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m2");  m.m2  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m3");  m.m3  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m4");  m.m4  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m5");  m.m5  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m6");  m.m6  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m7");  m.m7  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m8");  m.m8  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m9");  m.m9  = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m10"); m.m10 = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m11"); m.m11 = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m12"); m.m12 = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m13"); m.m13 = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m14"); m.m14 = lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, index, "m15"); m.m15 = lua_tonumber(L, -1); lua_pop(L, 1);
    return m;
}

// Lua binding for GetTime
static int lua_raylib_get_time(lua_State *L) {
    lua_pushnumber(L, glfwGetTime());
    return 1;
}

// Lua binding for MatrixPerspective
static int lua_raylib_matrix_perspective(lua_State *L) {
    float fovy = luaL_checknumber(L, 1);
    float aspect = luaL_checknumber(L, 2);
    float near = luaL_checknumber(L, 3);
    float far = luaL_checknumber(L, 4);
    Matrix proj = MatrixPerspective(fovy, aspect, near, far);
    push_matrix(L, proj);
    return 1;
}

// Lua binding for MatrixLookAt
static int lua_raylib_matrix_look_at(lua_State *L) {
    Vector3 eye = get_vector3(L, 1);
    Vector3 target = get_vector3(L, 2);
    Vector3 up = get_vector3(L, 3);
    Matrix view = MatrixLookAt(eye, target, up);
    push_matrix(L, view);
    return 1;
}

// Lua binding for MatrixRotateY
static int lua_raylib_matrix_rotate_y(lua_State *L) {
    float angle = luaL_checknumber(L, 1);
    Matrix rot = MatrixRotateY(angle);
    push_matrix(L, rot);
    return 1;
}

// Lua binding for MatrixTranslate
static int lua_raylib_matrix_translate(lua_State *L) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    Matrix trans = MatrixTranslate(x, y, z);
    push_matrix(L, trans);
    return 1;
}

// Lua binding for MatrixMultiply
static int lua_raylib_matrix_multiply(lua_State *L) {
    Matrix m1 = get_matrix(L, 1);
    Matrix m2 = get_matrix(L, 2);
    Matrix result = MatrixMultiply(m1, m2);
    push_matrix(L, result);
    return 1;
}

// Lua binding for rlSetMatrixProjection
static int lua_raylib_set_matrix_projection(lua_State *L) {
    Matrix proj = get_matrix(L, 1);
    rlSetMatrixProjection(proj);
    return 0;
}

// Lua binding for rlSetMatrixModelview
static int lua_raylib_set_matrix_modelview(lua_State *L) {
    Matrix modelview = get_matrix(L, 1);
    rlSetMatrixModelview(modelview);
    return 0;
}

// Lua binding for rlBegin
static int lua_raylib_begin(lua_State *L) {
    int mode = luaL_checkinteger(L, 1);
    rlBegin(mode);
    return 0;
}

// Lua binding for rlEnd
static int lua_raylib_end(lua_State *L) {
    rlEnd();
    return 0;
}

// Lua binding for rlVertex3f
static int lua_raylib_vertex3f(lua_State *L) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    rlVertex3f(x, y, z);
    return 0;
}

// Lua binding for rlColor4ub
static int lua_raylib_color4ub(lua_State *L) {
    unsigned char r = luaL_checkinteger(L, 1);
    unsigned char g = luaL_checkinteger(L, 2);
    unsigned char b = luaL_checkinteger(L, 3);
    unsigned char a = luaL_checkinteger(L, 4);
    rlColor4ub(r, g, b, a);
    return 0;
}

// Lua binding for rlTranslatef
static int lua_raylib_translatef(lua_State *L) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    rlTranslatef(x, y, z);
    return 0;
}

// Lua binding for lua_raylib_render (placeholder)
static int lua_raylib_render(lua_State *L) {
    luaL_error(L, "lua_raylib_render is a placeholder; use Lua script's render() instead");
    return 0;
}

// Register raylib functions to Lua global 'rl' table
static const struct luaL_Reg raylib_funcs[] = {
    {"GetTime", lua_raylib_get_time},
    {"MatrixPerspective", lua_raylib_matrix_perspective},
    {"MatrixLookAt", lua_raylib_matrix_look_at},
    {"MatrixRotateY", lua_raylib_matrix_rotate_y},
    {"MatrixTranslate", lua_raylib_matrix_translate},
    {"MatrixMultiply", lua_raylib_matrix_multiply},
    {"rlSetMatrixProjection", lua_raylib_set_matrix_projection},
    {"rlSetMatrixModelview", lua_raylib_set_matrix_modelview},
    {"rlBegin", lua_raylib_begin},
    {"rlEnd", lua_raylib_end},
    {"rlVertex3f", lua_raylib_vertex3f},
    {"rlColor4ub", lua_raylib_color4ub},
    {"rlTranslatef", lua_raylib_translatef},
    {"render", lua_raylib_render},
    {NULL, NULL}
};

// Initialize raylib module
void raylib_init(void) {
    lua_State *L = lua_get_state();
    if (!L) {
        printf("Error: No Lua state available in raylib_init\n");
        return;
    }
    
    lua_newtable(L);
    luaL_setfuncs(L, raylib_funcs, 0);
    lua_setglobal(L, "rl");
    lua_settop(L, 0);
    
    printf("raylib module initialized\n");
}

// Lua module entry point
int luaopen_raylib(lua_State *L) {
    raylib_init();
    return 0;
}