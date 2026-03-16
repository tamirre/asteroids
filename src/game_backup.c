#include "game.h"
// #include "raylib.h"




void GameUpdate(GameMemory* mem, float dt)
{
    GameState* gs = mem->gameState;

    gs->time += dt;

    if (IsKeyPressed(KEY_R))
        printf("Reload key pressed\n");
}

void GameDraw(GameMemory* mem)
{
    GameState* gs = mem->gameState;

    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("HOT RELOAD ACTIVE", 20, 20, 30, RED);
    DrawText(TextFormat("time: %.2f", gs->time), 20, 60, 20, WHITE);

    EndDrawing();
}



