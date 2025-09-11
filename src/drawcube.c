// drawcube.c
#include "drawcube.h"
#include "raymath.h"
#include <rlgl.h>

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
} Color;

void DrawCube(Vector3 position) {
    float size = 1.0f;

    Vector3 cubeVertices[24];
    cubeVertices[0] = (Vector3){-size, -size, +size};
    cubeVertices[1] = (Vector3){+size, -size, +size};
    cubeVertices[2] = (Vector3){+size, +size, +size};
    cubeVertices[3] = (Vector3){-size, +size, +size};
    cubeVertices[4] = (Vector3){-size, -size, -size};
    cubeVertices[5] = (Vector3){-size, +size, -size};
    cubeVertices[6] = (Vector3){+size, +size, -size};
    cubeVertices[7] = (Vector3){+size, -size, -size};
    cubeVertices[8] = (Vector3){-size, +size, -size};
    cubeVertices[9] = (Vector3){-size, +size, +size};
    cubeVertices[10] = (Vector3){+size, +size, +size};
    cubeVertices[11] = (Vector3){+size, +size, -size};
    cubeVertices[12] = (Vector3){-size, -size, -size};
    cubeVertices[13] = (Vector3){+size, -size, -size};
    cubeVertices[14] = (Vector3){+size, -size, +size};
    cubeVertices[15] = (Vector3){-size, -size, +size};
    cubeVertices[16] = (Vector3){+size, -size, -size};
    cubeVertices[17] = (Vector3){+size, +size, -size};
    cubeVertices[18] = (Vector3){+size, +size, +size};
    cubeVertices[19] = (Vector3){+size, -size, +size};
    cubeVertices[20] = (Vector3){-size, -size, -size};
    cubeVertices[21] = (Vector3){-size, -size, +size};
    cubeVertices[22] = (Vector3){-size, +size, +size};
    cubeVertices[23] = (Vector3){-size, +size, -size};

    int indices[36] = {
        0,1,2, 2,3,0,   // Front
        4,5,6, 6,7,4,   // Back
        8,9,10, 10,11,8, // Top
        12,13,14, 14,15,12, // Bottom
        16,17,18, 18,19,16, // Right
        20,21,22, 22,23,20  // Left
    };

    Color colors[6] = {
        {255,0,0,255},    // Front: Red
        {0,255,0,255},    // Back: Green
        {0,0,255,255},    // Top: Blue
        {255,255,0,255},  // Bottom: Yellow
        {255,0,255,255},  // Right: Magenta
        {0,255,255,255}   // Left: Cyan
    };

    rlTranslatef(position.x, position.y, position.z);

    rlBegin(RL_TRIANGLES);
    for (int face = 0; face < 6; ++face) {
        rlColor4ub(colors[face].r, colors[face].g, colors[face].b, colors[face].a);
        for (int i = 0; i < 6; ++i) {
            int idx = face * 6 + i;
            Vector3 v = cubeVertices[indices[idx]];
            rlVertex3f(v.x, v.y, v.z);
        }
    }
    rlEnd();
}