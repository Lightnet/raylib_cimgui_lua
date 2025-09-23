//===============================================
// raylib, cimgui, lua sample build.
//===============================================

// #define ENET_IMPLEMENTATION

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "cimgui.h"
#include "cimgui_impl.h"

// #define RLGL_IMPLEMENTATION
#include "rlgl.h"
#define RAYMATH_STATIC_INLINE
#include "raymath.h"
#include "raylib.h" // For DrawCube in default UI

#include <GLFW/glfw3.h>

#include "module_lua.h"
#include "module_cimgui.h"
#include "module_enet.h"
#include "module_raylib.h"

// #include "drawcube.h"

#include <stdio.h>              // Required for: printf()
#include <math.h>               // For fmodf

#define igGetIO igGetIO_Nil

// #define RED        (Color){ 230, 41, 55, 255 }     // Red
// #define RAYWHITE   (Color){ 245, 245, 245, 255 }   // My own White (raylib logo)
// #define DARKGRAY   (Color){ 80, 80, 80, 255 }      // Dark Gray

// // Color, 4 components, R8G8B8A8 (32bit)
// typedef struct Color {
//     unsigned char r;        // Color red value
//     unsigned char g;        // Color green value
//     unsigned char b;        // Color blue value
//     unsigned char a;        // Color alpha value
// } Color;

// // Camera type, defines a camera position/orientation in 3d space
// typedef struct Camera {
//     Vector3 position;       // Camera position
//     Vector3 target;         // Camera target it looks-at
//     Vector3 up;             // Camera up vector (rotation over its axis)
//     float fovy;             // Camera field-of-view apperture in Y (degrees) in perspective, used as near plane width in orthographic
//     int projection;         // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
// } Camera;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void ErrorCallback(int error, const char *description);
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);  // For resize

// Function to draw a simple colored cube (faces with different colors)
// static void DrawCube(Vector3 position) {
//     // Simple cube with size 1.0f, centered at position
//     float size = 1.0f;

//     // Vertices for the cube (24 for faces)
//     Vector3 cubeVertices[24];  // 4 vertices per face * 6 faces
//     // Front face
//     cubeVertices[0] = (Vector3){-size, -size, +size};
//     cubeVertices[1] = (Vector3){+size, -size, +size};
//     cubeVertices[2] = (Vector3){+size, +size, +size};
//     cubeVertices[3] = (Vector3){-size, +size, +size};
//     // Back face
//     cubeVertices[4] = (Vector3){-size, -size, -size};
//     cubeVertices[5] = (Vector3){-size, +size, -size};
//     cubeVertices[6] = (Vector3){+size, +size, -size};
//     cubeVertices[7] = (Vector3){+size, -size, -size};
//     // Top face
//     cubeVertices[8] = (Vector3){-size, +size, -size};
//     cubeVertices[9] = (Vector3){-size, +size, +size};
//     cubeVertices[10] = (Vector3){+size, +size, +size};
//     cubeVertices[11] = (Vector3){+size, +size, -size};
//     // Bottom face
//     cubeVertices[12] = (Vector3){-size, -size, -size};
//     cubeVertices[13] = (Vector3){+size, -size, -size};
//     cubeVertices[14] = (Vector3){+size, -size, +size};
//     cubeVertices[15] = (Vector3){-size, -size, +size};
//     // Right face
//     cubeVertices[16] = (Vector3){+size, -size, -size};
//     cubeVertices[17] = (Vector3){+size, +size, -size};
//     cubeVertices[18] = (Vector3){+size, +size, +size};
//     cubeVertices[19] = (Vector3){+size, -size, +size};
//     // Left face
//     cubeVertices[20] = (Vector3){-size, -size, -size};
//     cubeVertices[21] = (Vector3){-size, -size, +size};
//     cubeVertices[22] = (Vector3){-size, +size, +size};
//     cubeVertices[23] = (Vector3){-size, +size, -size};

//     // Indices for triangles (6 indices per face: two triangles)
//     int indices[36] = {
//         0,1,2, 2,3,0,   // Front
//         4,5,6, 6,7,4,   // Back
//         8,9,10, 10,11,8, // Top
//         12,13,14, 14,15,12, // Bottom
//         16,17,18, 18,19,16, // Right
//         20,21,22, 22,23,20  // Left
//     };

//     // Colors per face (RGBA)
//     Color colors[6] = {
//         {255,0,0,255},    // Front: Red
//         {0,255,0,255},    // Back: Green
//         {0,0,255,255},    // Top: Blue
//         {255,255,0,255},  // Bottom: Yellow
//         {255,0,255,255},  // Right: Magenta
//         {0,255,255,255}   // Left: Cyan
//     };

//     // Translate to position before drawing (simple, no stack needed here)
//     rlTranslatef(position.x, position.y, position.z);

//     rlBegin(RL_TRIANGLES);
//     for (int face = 0; face < 6; ++face) {
//         // Set color for this face (scalar calls)
//         rlColor4ub(colors[face].r, colors[face].g, colors[face].b, colors[face].a);
//         // Draw 6 vertices for the two triangles of this face
//         for (int i = 0; i < 6; ++i) {
//             int idx = face * 6 + i;
//             Vector3 v = cubeVertices[indices[idx]];
//             rlVertex3f(v.x, v.y, v.z);
//         }
//     }
//     rlEnd();
// }

// Forward declaration for file_exists (add this line before main())
static int file_exists(const char* filename);

int main(int argc, char** argv) {
    int screenWidth = 800;
    int screenHeight = 450;
    const char *glsl_version = "#version 130";

    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "rlgl + ImGui + 3D Cube", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window, 200, 200);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);  // Handle resize

    // Make the OpenGL context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable VSync for smoothness

    // Load OpenGL 3.3 supported extensions
    rlLoadExtensions(glfwGetProcAddress);

    // Initialize OpenGL context (states and resources)
    rlglInit(screenWidth, screenHeight);

    // Set clear color (do this after rlglInit)
    rlClearColor(245, 245, 200, 255);  // Light yellow background

    // Enable depth test for 3D
    rlEnableDepthTest();

    // Camera setup
    Camera camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point (cube)
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector
    camera.fovy = 45.0f;                                // Field of view Y

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };        // Cube at center
    float rotation = 0.0f;  // For animation (updated by slider or auto)

    // Initialize Lua
    lua_init();

    // Setup ImGui
    // need to setup here for lua init setup else it crashed.
    igCreateContext(NULL);
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle* style = igGetStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    igStyleColorsDark(NULL);

    // Initialize cimgui this goes here since we need imgui else error.
    cimgui_init(); // init lua cimgui module
    enet_init(); // init network lua module
    raylib_init();

    // Load Lua and check script
    const char* lua_script = "script.lua";
    if (argc > 1) {
        lua_script = argv[1];
        printf("Using Lua script from arg: %s\n", lua_script);
    }
    bool use_lua = false;
    if (file_exists(lua_script)) {
        use_lua = lua_load_script(lua_script);
        if (!use_lua) {
            printf("Failed to load Lua script '%s', falling back to default UI\n", lua_script);
        }
    } else {
        printf("Lua script '%s' does not exist, using default UI\n", lua_script);
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        
        // Auto-rotate cube
        if (!use_lua || igIsItemActive() == false) {
            // rotation = fmodf(time * 30.0f, 360.0f);
            rotation = fmodf(time * 90.0f, 360.0f);
        }

        glfwPollEvents();
        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
        rlClearScreenBuffers();

        // ImGui frame start
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        bool showDemoWindow = true;
        
        if (use_lua) {
            // For Lua: Call cimgui_call_draw if implemented, then draw
            igNewFrame();
            cimgui_call_draw();  // Call script's draw()
            // enet_update(); // Add network processing // better to single loop from cimgui_call_draw()

            //show demo for refs.
            if (showDemoWindow)
                igShowDemoWindow(&showDemoWindow);
            igRender();
        } else {
            // Default UI
            igNewFrame();
            igBegin("Hello, world!", NULL, 0);
            igText("This is some useful text.");
            igText("3D Cube should now rotate below!");
            igText("Current Rotation: %.1f degrees", rotation);
            if (igSliderFloat("Cube Y Rotation", &rotation, 0.0f, 360.0f, "%.0f degrees", 0)) {
                // Slider changed
            }
            igEnd();
            igRender();
        }

        // 3D Rendering (unchanged)
        float aspect = (float)screenWidth / (float)screenHeight;
        Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
        rlSetMatrixProjection(proj);

        Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
        Matrix rot = MatrixRotateY(rotation * DEG2RAD);
        Matrix trans = MatrixTranslate(cubePosition.x, cubePosition.y, cubePosition.z);
        Matrix model = MatrixMultiply(rot, trans);
        Matrix modelView = MatrixMultiply(model, view);
        rlSetMatrixModelview(modelView);

        // custom build
        // DrawCube((Vector3){0.0f, 0.0f, 0.0f});

        // raylib works
        DrawCube(cubePosition, 0.1f, 0.1f, 0.5f, RED); // Use raylib's DrawCube for default UI


        rlDrawRenderBatchActive();

        // Reset state for ImGui ???
        // glUseProgram(0);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    if (use_lua) {
        lua_State* L = lua_get_state();
        if (L) {
            lua_getglobal(L, "cleanup");
            if (lua_isfunction(L, -1)) {
                lua_pcall(L, 0, 0, 0);
            }
            // Pop any remaining stack items if needed
            lua_settop(L, 0);
        }
    }
    enet_cleanup();      // Call before Lua close
    cimgui_cleanup();    // Call before Lua close
    lua_cleanup();       // Now safe to close Lua state
    rlglClose();
    glfwDestroyWindow(window);
    glfwTerminate();


    // if (use_lua) {
    //     lua_State* L = lua_get_state();
    //     if (L) {
    //         lua_getglobal(L, "cleanup");
    //         if (lua_isfunction(L, -1)) {
    //             lua_pcall(L, 0, 0, 0);
    //         }
    //         lua_cleanup();
    //     }
    // }
    // enet_cleanup();
    // cimgui_cleanup();
    // lua_cleanup();
    
    // rlglClose();
    // glfwDestroyWindow(window);
    // glfwTerminate();
    return 0;
}

// Add the file_exists function from module_raylib_lua.c to main file or include the header
static int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definitions
//----------------------------------------------------------------------------------

static void ErrorCallback(int error, const char *description) {
    fprintf(stderr, "%s", description);
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

// Resize callback: Update viewport
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    rlViewport(0, 0, width, height);
}