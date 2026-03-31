#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "chunk system");

    Texture2D texture = LoadTexture("asteroid3.png");
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    // Create render target (FBO)
    RenderTexture2D target = LoadRenderTexture(texture.width, texture.height);

    Shader shader = LoadShader(0, "chunk.fs");

    int timeLoc = GetShaderLocation(shader, "time");
    int explodeLoc = GetShaderLocation(shader, "explode");

    float explode = 0.0f;

    SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		float dt = GetFrameTime();

	   if (IsKeyPressed(KEY_SPACE))
		   explode = 0.0f;

	   explode += dt * 1.5f;   // keeps growing
	   if (explode > 5.0f)
		   explode = 5.0f;

	   SetShaderValue(shader, explodeLoc, &explode, SHADER_UNIFORM_FLOAT);     SetShaderValue(shader, explodeLoc, &explode, SHADER_UNIFORM_FLOAT);

        // --- render texture into FBO ---
        BeginTextureMode(target);
        ClearBackground(BLANK);
        DrawTexture(texture, 0, 0, WHITE);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        BeginShaderMode(shader);

        DrawTextureRec(
            target.texture,
            (Rectangle){0, 0, (float)target.texture.width, -(float)target.texture.height}, // flip Y!
            (Vector2){
                screenWidth/2.0f - texture.width/2.0f,
                screenHeight/2.0f - texture.height/2.0f
            },
            WHITE
        );

        EndShaderMode();

        DrawText("Press SPACE to explode", 10, 10, 20, RAYWHITE);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadTexture(texture);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}
