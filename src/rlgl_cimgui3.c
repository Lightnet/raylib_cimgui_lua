//===============================================
// Base simple setup test for raylib and cimgui.
//===============================================
#include "cimgui.h"
#include "cimgui_impl.h"

#define RLGL_IMPLEMENTATION
#include "rlgl.h"
#define RAYMATH_STATIC_INLINE
#include "raymath.h"

#include <GLFW/glfw3.h>

#include <stdio.h>              // Required for: printf()
#include <math.h>               // For fmodf

#define igGetIO igGetIO_Nil

#define RED        (Color){ 230, 41, 55, 255 }     // Red
#define RAYWHITE   (Color){ 245, 245, 245, 255 }   // My own White (raylib logo)
#define DARKGRAY   (Color){ 80, 80, 80, 255 }      // Dark Gray

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
} Color;

// Camera type, defines a camera position/orientation in 3d space
typedef struct Camera {
    Vector3 position;       // Camera position
    Vector3 target;         // Camera target it looks-at
    Vector3 up;             // Camera up vector (rotation over its axis)
    float fovy;             // Camera field-of-view apperture in Y (degrees) in perspective, used as near plane width in orthographic
    int projection;         // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
} Camera;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void ErrorCallback(int error, const char *description);
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);  // For resize

// Function to draw a simple colored cube (faces with different colors)
static void DrawCube(Vector3 position) {
    // Simple cube with size 1.0f, centered at position
    float size = 1.0f;

    // Vertices for the cube (24 for faces)
    Vector3 cubeVertices[24];  // 4 vertices per face * 6 faces
    // Front face
    cubeVertices[0] = (Vector3){-size, -size, +size};
    cubeVertices[1] = (Vector3){+size, -size, +size};
    cubeVertices[2] = (Vector3){+size, +size, +size};
    cubeVertices[3] = (Vector3){-size, +size, +size};
    // Back face
    cubeVertices[4] = (Vector3){-size, -size, -size};
    cubeVertices[5] = (Vector3){-size, +size, -size};
    cubeVertices[6] = (Vector3){+size, +size, -size};
    cubeVertices[7] = (Vector3){+size, -size, -size};
    // Top face
    cubeVertices[8] = (Vector3){-size, +size, -size};
    cubeVertices[9] = (Vector3){-size, +size, +size};
    cubeVertices[10] = (Vector3){+size, +size, +size};
    cubeVertices[11] = (Vector3){+size, +size, -size};
    // Bottom face
    cubeVertices[12] = (Vector3){-size, -size, -size};
    cubeVertices[13] = (Vector3){+size, -size, -size};
    cubeVertices[14] = (Vector3){+size, -size, +size};
    cubeVertices[15] = (Vector3){-size, -size, +size};
    // Right face
    cubeVertices[16] = (Vector3){+size, -size, -size};
    cubeVertices[17] = (Vector3){+size, +size, -size};
    cubeVertices[18] = (Vector3){+size, +size, +size};
    cubeVertices[19] = (Vector3){+size, -size, +size};
    // Left face
    cubeVertices[20] = (Vector3){-size, -size, -size};
    cubeVertices[21] = (Vector3){-size, -size, +size};
    cubeVertices[22] = (Vector3){-size, +size, +size};
    cubeVertices[23] = (Vector3){-size, +size, -size};

    // Indices for triangles (6 indices per face: two triangles)
    int indices[36] = {
        0,1,2, 2,3,0,   // Front
        4,5,6, 6,7,4,   // Back
        8,9,10, 10,11,8, // Top
        12,13,14, 14,15,12, // Bottom
        16,17,18, 18,19,16, // Right
        20,21,22, 22,23,20  // Left
    };

    // Colors per face (RGBA)
    Color colors[6] = {
        {255,0,0,255},    // Front: Red
        {0,255,0,255},    // Back: Green
        {0,0,255,255},    // Top: Blue
        {255,255,0,255},  // Bottom: Yellow
        {255,0,255,255},  // Right: Magenta
        {0,255,255,255}   // Left: Cyan
    };

    // Translate to position before drawing (simple, no stack needed here)
    rlTranslatef(position.x, position.y, position.z);

    rlBegin(RL_TRIANGLES);
    for (int face = 0; face < 6; ++face) {
        // Set color for this face (scalar calls)
        rlColor4ub(colors[face].r, colors[face].g, colors[face].b, colors[face].a);
        // Draw 6 vertices for the two triangles of this face
        for (int i = 0; i < 6; ++i) {
            int idx = face * 6 + i;
            Vector3 v = cubeVertices[indices[idx]];
            rlVertex3f(v.x, v.y, v.z);
        }
    }
    rlEnd();
}

int main() {
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

    // Setup ImGui
    igCreateContext(NULL);
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle* style = igGetStyle();
    // Optional: float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    // ImGuiStyle_ScaleAllSizes(style, main_scale);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    igStyleColorsDark(NULL);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        // Auto-rotate if not actively using slider (simple fallback; slider overrides)
        if (igIsItemActive() == false) {  // Check if slider is not being dragged
            rotation = fmodf(time * 30.0f, 360.0f);  // 30 degrees per second
        }

        glfwPollEvents();

        // Get current size (for dynamic support)
        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

        // Clear early (color + depth for 3D)
        rlClearScreenBuffers();

        // ImGui frame start
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();

        // Build ImGui UI
        igBegin("Hello, world!", NULL, 0);
        igText("This is some useful text.");
        igText("3D Cube should now rotate below!");
        igText("Current Rotation: %.1f degrees", rotation);
        if (igSliderFloat("Cube Y Rotation", &rotation, 0.0f, 360.0f, "%.0f degrees", 0)) {
            // Slider changed - rotation updates immediately
        }
        igEnd();

        // 3D Rendering Setup
        float aspect = (float)screenWidth / (float)screenHeight;
        Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);  // Perspective projection
        rlSetMatrixProjection(proj);

        // Compute view matrix from camera
        Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);

        // Compute model matrix: rotation * translation
        Matrix rot = MatrixRotateY(rotation * DEG2RAD);  // Rotate around Y
        Matrix trans = MatrixTranslate(cubePosition.x, cubePosition.y, cubePosition.z);
        Matrix model = MatrixMultiply(rot, trans);

        // Full model-view matrix (apply model to view)
        Matrix modelView = MatrixMultiply(model, view);

        // Set the full model-view directly (bypass stack)
        rlSetMatrixModelview(modelView);

        // Draw the cube (no push/pop or mult needed)
        DrawCube((Vector3){0.0f, 0.0f, 0.0f});  // At local origin, with model applied above

        rlDrawRenderBatchActive();  // Flush the batch

        // End ImGui frame (record lists)
        igRender();

        // Reset state for ImGui
        glUseProgram(0);

        // Render ImGui (on top, 2D)
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);
    rlglClose();
    glfwDestroyWindow(window);
    glfwTerminate();
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