cmake_minimum_required(VERSION 3.15)
project(raylib_cimgui_lua)

include(FetchContent)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Fetch raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(raylib)

# Fetch cimgui with a stable tag
FetchContent_Declare(
    cimgui
    GIT_REPOSITORY https://github.com/cimgui/cimgui.git
    GIT_TAG 1.92.1 # Matches ImGui 1.92.1 updated.
)
FetchContent_MakeAvailable(cimgui)

# Define cimgui sources
set(IMGUI_SOURCES
    ${cimgui_SOURCE_DIR}/cimgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

# Build cimgui as a static library with a custom name
add_library(custom_cimgui STATIC ${IMGUI_SOURCES})

# Set include directories for custom_cimgui
target_include_directories(custom_cimgui PUBLIC
    ${cimgui_SOURCE_DIR}                            # cimgui
    ${cimgui_SOURCE_DIR}/imgui                      # imgui
    ${raylib_SOURCE_DIR}/src/external/glfw/include  # glfw
)

# Set compile definitions for custom_cimgui (no CIMGUI_DEFINE_ENUMS_AND_STRUCTS)
target_compile_definitions(custom_cimgui PUBLIC
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    IMGUI_IMPL_API=extern\ \"C\"
)

# Link custom_cimgui with GLFW and OpenGL
target_link_libraries(custom_cimgui PUBLIC glfw OpenGL::GL)

#================================================
# Lua
#================================================
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lua)

# Lua doesn't have a CMake build, so we manually compile it
# List Lua source files (core and libraries, excluding lua.c and luac.c)
set(LUA_SRC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lcorolib.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldblib.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/linit.c
    ${lua_SOURCE_DIR}/liolib.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/loadlib.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/loslib.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
    ${lua_SOURCE_DIR}/lutf8lib.c
)

# Create a static Lua library
add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})

#================================================
# ENET
#================================================
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/zpl-c/enet.git
    GIT_TAG v2.6.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(enet)



#================================================
# Application
#================================================
set(APP_NAME ril)
set(SRC_FILES
    src/module_cimgui.c
    src/module_lua.c
    src/module_enet.c
)

add_executable(${APP_NAME}
    ${SRC_FILES}
    src/main.c
)

# Link application with custom_cimgui
target_link_libraries(${APP_NAME} PRIVATE 
    raylib
    custom_cimgui
    lua
)

# Include directories for the application
target_include_directories(${APP_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include                   # include
    ${lua_SOURCE_DIR}                               # lua
    ${cimgui_SOURCE_DIR}                            # imgui
    ${cimgui_SOURCE_DIR}/imgui                      # imgui
    ${raylib_SOURCE_DIR}/src                        # raylib include
    ${raylib_SOURCE_DIR}/src/external/glfw/include  # glfw
    ${enet_SOURCE_DIR}/include                      # enet
)

# Set compile definitions for the application
# this will enable for cimgui windows and graphics defines
target_compile_definitions(${APP_NAME} PRIVATE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    CIMGUI_USE_GLFW=1
    CIMGUI_USE_OPENGL3=1
    ENET_IMPLEMENTATION=1
)

# Windows-specific settings
if(WIN32)
    # ws2_32 # network
    target_link_libraries(${APP_NAME} PRIVATE ws2_32 gdi32 user32 shell32)
    # target_link_options(${APP_NAME} PRIVATE -static-libgcc)

    target_link_options(${APP_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++  # Uncomment if C++ code is used
        -static           # Avoid full static linking to prevent issues with system libraries
    )
endif()