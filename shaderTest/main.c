#include "raylib.h"
#include <math.h>

int main(void)
{
    const int screenWidth = 1080;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Shader-controlled scaling");
    SetTargetFPS(60);

    Texture2D texture = LoadTexture("upgradeMultiShot.png");
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    Shader shader = LoadShader(0,"shader.glsl");

    int resLoc = GetShaderLocation(shader, "resolution");
    int texSizeLoc = GetShaderLocation(shader, "textureSize");
    int scaleLoc = GetShaderLocation(shader, "scale");

    Vector2 resolution = { screenWidth, screenHeight };
    Vector2 texSize = { texture.width, texture.height };

    SetShaderValue(shader, resLoc, &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_VEC2);

    float time = 0.0f;
    float zoom = 3.5f;

    bool animateScale = true;
    bool shaderEnabled = true;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        time += dt;

        if (IsKeyPressed(KEY_S)) animateScale = !animateScale;
        if (IsKeyPressed(KEY_H)) shaderEnabled = !shaderEnabled;

        if (animateScale)
            zoom = 3.5f + 3.0f * sinf(time * 0.5f);

        SetShaderValue(shader, scaleLoc, &zoom, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(BLACK);

        if (shaderEnabled) BeginShaderMode(shader);

        // Draw fullscreen quad
        DrawTexturePro(
            texture,
            (Rectangle){0,0,texture.width,texture.height},
            (Rectangle){0,0,screenWidth,screenHeight},
            (Vector2){0,0},
            0.0f,
            WHITE
        );

        if (shaderEnabled) EndShaderMode();

        DrawText("S - Toggle Scale Animation", 20, 20, 20, RAYWHITE);
        DrawText("H - Toggle Shader", 20, 50, 20, RAYWHITE);

        EndDrawing();
    }

    UnloadShader(shader);
    UnloadTexture(texture);
    CloseWindow();
    return 0;
}
