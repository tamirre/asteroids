#include "raylib.h"
#include <math.h>

int main(void)
{
    const int screenWidth = 1000;
    const int screenHeight = 700;

    InitWindow(screenWidth, screenHeight, "raylib sprite scaling test");
    SetTargetFPS(60);

    Texture2D texture = LoadTexture("upgradeMultiShot.png");

    // Let the shader control sampling
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    Shader shader = LoadShader(0, "shader.glsl");

    int texSizeLoc = GetShaderLocation(shader, "textureSize");
    Vector2 texSize = { texture.width, texture.height };
    SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_VEC2);

    float time = 0.0f;
    float scale = 3.5f;
    float rotation = 0.0f;

    bool animateScale = true;
    bool animateRotation = true;
    bool shaderOn = true;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        time += dt;

        // toggles
        if (IsKeyPressed(KEY_S)) animateScale = !animateScale;
        if (IsKeyPressed(KEY_R)) animateRotation = !animateRotation;
        if (IsKeyPressed(KEY_H)) shaderOn = !shaderOn;

        // manual scale
        if (IsKeyPressed(KEY_J)) scale += 0.1f;
        if (IsKeyPressed(KEY_K)) scale -= 0.1f;

        // animated scale
        if (animateScale)
            scale = 3.5f + 2.0f * sinf(time);

        // animated rotation
        if (animateRotation)
            rotation = time * 45.0f;   // 45° per second

        Vector2 pos = { screenWidth / 2.0f, screenHeight / 2.0f };

        Rectangle src = { 0, 0, texture.width, texture.height };

        Rectangle dst = {
            pos.x,
            pos.y,
            texture.width * scale,
            texture.height * scale
        };

        Vector2 origin = {
            dst.width / 2,
            dst.height / 2
        };

        BeginDrawing();
        ClearBackground(BLACK);

        if (shaderOn) BeginShaderMode(shader);

        DrawTexturePro(texture, src, dst, origin, rotation, WHITE);

        if (shaderOn) EndShaderMode();

        // Controls
        DrawText("S - toggle scale animation", 20, 20, 20, RAYWHITE);
        DrawText("R - toggle rotation", 20, 50, 20, RAYWHITE);
        DrawText("J/K - adjust scale", 20, 80, 20, RAYWHITE);
        DrawText("H - toggle shader", 20, 110, 20, RAYWHITE);

        // Status indicators
        if (shaderOn)
            DrawText("SHADER: ON", 20, 150, 24, GREEN);
        else
            DrawText("SHADER: OFF", 20, 150, 24, RED);

        if (animateRotation)
            DrawText("ROTATION: ON", 20, 180, 24, GREEN);
        else
            DrawText("ROTATION: OFF", 20, 180, 24, RED);

        EndDrawing();
    }

    UnloadShader(shader);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}
