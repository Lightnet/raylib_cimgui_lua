// module_enet.c
#include "module_enet.h"
#include "module_lua.h"
#include <lauxlib.h>
#include <lualib.h>
#include <enet.h>
#include <string.h>
#include <stdio.h>

#define ENET_HOST_MT "ENetHost"
#define ENET_PEER_MT "ENetPeer"
#define ENET_PACKET_MT "ENetPacket"

// Local Lua state
static lua_State* g_lua_state = NULL;

// Helper to push ENetHost userdata
static ENetHost** push_enet_host(lua_State *L) {
    ENetHost **host = (ENetHost**)lua_newuserdata(L, sizeof(ENetHost*));
    luaL_getmetatable(L, ENET_HOST_MT);
    lua_setmetatable(L, -2);
    return host;
}

// Helper to push ENetPeer userdata
static ENetPeer** push_enet_peer(lua_State *L) {
    ENetPeer **peer = (ENetPeer**)lua_newuserdata(L, sizeof(ENetPeer*));
    luaL_getmetatable(L, ENET_PEER_MT);
    lua_setmetatable(L, -2);
    return peer;
}

// Helper to push ENetPacket userdata
static ENetPacket** push_enet_packet(lua_State *L) {
    ENetPacket **packet = (ENetPacket**)lua_newuserdata(L, sizeof(ENetPacket*));
    luaL_getmetatable(L, ENET_PACKET_MT);
    lua_setmetatable(L, -2);
    return packet;
}

// enet.initialize()
static int l_enet_initialize(lua_State *L) {
    int result = enet_initialize();
    lua_pushinteger(L, result);
    return 1;
}

// enet.deinitialize()
static int l_enet_deinitialize(lua_State *L) {
    enet_deinitialize();
    return 0;
}

// enet.host_create(address, peerCount, channelCount, incomingBandwidth, outgoingBandwidth)
static int l_enet_host_create(lua_State *L) {
    ENetAddress *address = NULL;
    if (lua_istable(L, 1)) {
        address = (ENetAddress*)malloc(sizeof(ENetAddress));
        memset(address, 0, sizeof(ENetAddress));
        lua_getfield(L, 1, "host");
        if (lua_isstring(L, -1)) {
            enet_address_set_host_new(address, lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        lua_getfield(L, 1, "port");
        address->port = lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 1, "sin6_scope_id");
        address->sin6_scope_id = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    size_t peerCount = luaL_checkinteger(L, 2);
    size_t channelCount = luaL_checkinteger(L, 3);
    enet_uint32 incomingBandwidth = luaL_optinteger(L, 4, 0);
    enet_uint32 outgoingBandwidth = luaL_optinteger(L, 5, 0);

    ENetHost *host = enet_host_create(address, peerCount, channelCount, incomingBandwidth, outgoingBandwidth);
    if (address) free(address);
    if (host == NULL) {
        lua_pushnil(L);
    } else {
        ENetHost **ud = push_enet_host(L);
        *ud = host;
    }
    return 1;
}

// enet.host_destroy(host)
static int l_enet_host_destroy(lua_State *L) {
    ENetHost **host = (ENetHost**)luaL_checkudata(L, 1, ENET_HOST_MT);
    if (*host) {
        enet_host_destroy(*host);
        *host = NULL;
    }
    return 0;
}

// enet.host_connect(host, address, channelCount, data)
static int l_enet_host_connect(lua_State *L) {
    ENetHost **host = (ENetHost**)luaL_checkudata(L, 1, ENET_HOST_MT);
    ENetAddress address = {0};
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "host");
        if (lua_isstring(L, -1)) {
            enet_address_set_host_new(&address, lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        lua_getfield(L, 2, "port");
        address.port = lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 2, "sin6_scope_id");
        address.sin6_scope_id = lua_tointeger(L, -1);
        lua_pop(L, 1);
    } else {
        lua_pushnil(L);
        return 1;
    }

    size_t channelCount = luaL_checkinteger(L, 3);
    enet_uint32 data = luaL_optinteger(L, 4, 0);

    ENetPeer *peer = enet_host_connect(*host, &address, channelCount, data);
    if (peer == NULL) {
        lua_pushnil(L);
    } else {
        ENetPeer **ud = push_enet_peer(L);
        *ud = peer;
    }
    return 1;
}

// enet.host_service(host, timeout)
static int l_enet_host_service(lua_State *L) {
    ENetHost **host = (ENetHost**)luaL_checkudata(L, 1, ENET_HOST_MT);
    enet_uint32 timeout = luaL_optinteger(L, 2, 1000);
    ENetEvent event;

    int result = enet_host_service(*host, &event, timeout);
    if (result > 0) {
        lua_newtable(L);
        lua_pushinteger(L, event.type);
        lua_setfield(L, -2, "type");
        if (event.peer) {
            ENetPeer **ud = push_enet_peer(L);
            *ud = event.peer;
            lua_setfield(L, -2, "peer");
        }
        lua_pushinteger(L, event.channelID);
        lua_setfield(L, -2, "channelID");
        if (event.packet) {
            ENetPacket **ud = push_enet_packet(L);
            *ud = event.packet;
            lua_setfield(L, -2, "packet");
        }
        return 1;
    }
    lua_pushinteger(L, result);
    return 1;
}

// enet.packet_create(data, flags)
static int l_enet_packet_create(lua_State *L) {
    size_t data_len;
    const char *data = luaL_checklstring(L, 1, &data_len);
    enet_uint32 flags = luaL_optinteger(L, 2, 0);

    ENetPacket *packet = enet_packet_create(data, data_len, flags);
    if (packet == NULL) {
        lua_pushnil(L);
    } else {
        ENetPacket **ud = push_enet_packet(L);
        *ud = packet;
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "enet_packet_ref");
    }
    return 1;
}

// enet.packet_destroy(packet)
static int l_enet_packet_destroy(lua_State *L) {
    ENetPacket **packet = (ENetPacket**)luaL_checkudata(L, 1, ENET_PACKET_MT);
    if (*packet) {
        enet_packet_destroy(*packet);
        *packet = NULL;
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "enet_packet_ref");
    }
    return 0;
}

// enet.packet_data(packet)
static int l_enet_packet_data(lua_State *L) {
    ENetPacket **packet = (ENetPacket**)luaL_checkudata(L, 1, ENET_PACKET_MT);
    if (*packet) {
        lua_pushlstring(L, (const char*)(*packet)->data, (*packet)->dataLength);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// enet.peer_send(peer, channelID, packet)
static int l_enet_peer_send(lua_State *L) {
    ENetPeer **peer = (ENetPeer**)luaL_checkudata(L, 1, ENET_PEER_MT);
    enet_uint8 channelID = luaL_checkinteger(L, 2);
    ENetPacket **packet = (ENetPacket**)luaL_checkudata(L, 3, ENET_PACKET_MT);

    int result = enet_peer_send(*peer, channelID, *packet);
    if (result == 0) {
        *packet = NULL;
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "enet_packet_ref");
    }
    lua_pushinteger(L, result);
    return 1;
}

// Metatables for cleanup
static int enet_host_gc(lua_State *L) {
    ENetHost **host = (ENetHost**)luaL_checkudata(L, 1, ENET_HOST_MT);
    if (*host) {
        enet_host_destroy(*host);
        *host = NULL;
    }
    return 0;
}

static int enet_packet_gc(lua_State *L) {
    ENetPacket **packet = (ENetPacket**)luaL_checkudata(L, 1, ENET_PACKET_MT);
    if (*packet) {
        enet_packet_destroy(*packet);
        *packet = NULL;
    }
    return 0;
}

static const luaL_Reg enet_funcs[] = {
    {"initialize", l_enet_initialize},
    {"deinitialize", l_enet_deinitialize},
    {"host_create", l_enet_host_create},
    {"host_destroy", l_enet_host_destroy},
    {"host_connect", l_enet_host_connect},
    {"host_service", l_enet_host_service},
    {"packet_create", l_enet_packet_create},
    {"packet_destroy", l_enet_packet_destroy},
    {"packet_data", l_enet_packet_data},
    {"peer_send", l_enet_peer_send},
    {NULL, NULL}
};

static const luaL_Reg enet_host_mt[] = {
    {"__gc", enet_host_gc},
    {NULL, NULL}
};

static const luaL_Reg enet_packet_mt[] = {
    {"__gc", enet_packet_gc},
    {NULL, NULL}
};

int luaopen_enet(lua_State *L) {
    // Register ENetHost metatable
    luaL_newmetatable(L, ENET_HOST_MT);
    luaL_setfuncs(L, enet_host_mt, 0);
    lua_pop(L, 1);

    // Register ENetPeer metatable
    luaL_newmetatable(L, ENET_PEER_MT);
    lua_pop(L, 1);

    // Register ENetPacket metatable
    luaL_newmetatable(L, ENET_PACKET_MT);
    luaL_setfuncs(L, enet_packet_mt, 0);
    lua_pop(L, 1);

    // Register enet module
    luaL_newlib(L, enet_funcs);

    // Add event type constants
    lua_pushinteger(L, ENET_EVENT_TYPE_NONE);
    lua_setfield(L, -2, "EVENT_TYPE_NONE");
    lua_pushinteger(L, ENET_EVENT_TYPE_CONNECT);
    lua_setfield(L, -2, "EVENT_TYPE_CONNECT");
    lua_pushinteger(L, ENET_EVENT_TYPE_DISCONNECT);
    lua_setfield(L, -2, "EVENT_TYPE_DISCONNECT");
    lua_pushinteger(L, ENET_EVENT_TYPE_RECEIVE);
    lua_setfield(L, -2, "EVENT_TYPE_RECEIVE");

    return 1;
}

void enet_init(void) {
    // Fetch Lua state
    g_lua_state = lua_get_state();
    if (!g_lua_state) {
        printf("Error: No Lua state available in enet_init\n");
        return;
    }

    // Initialize ENet
    if (enet_initialize() != 0) {
        printf("Error: Failed to initialize ENet\n");
        return;
    }

    // Register enet module in Lua
    luaopen_enet(g_lua_state);
    lua_setglobal(g_lua_state, "enet");
    lua_settop(g_lua_state, 0); // Clean stack

    printf("ENet module initialized\n");
}

void enet_update(void) {
    if (!g_lua_state) {
        printf("Error: No Lua state in enet_update\n");
        return;
    }
    lua_getglobal(g_lua_state, "network_update");
    if (lua_isfunction(g_lua_state, -1)) {
        if (lua_pcall(g_lua_state, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(g_lua_state, -1);
            printf("Lua network_update error: %s\n", error);
            lua_pop(g_lua_state, 1);
        }
    } else {
        lua_pop(g_lua_state, 1);
    }
    lua_settop(g_lua_state, 0);
}


void enet_cleanup(void) {
    enet_deinitialize();
    g_lua_state = NULL;
    printf("ENet module cleaned up\n");
}

