#define RAYMATH_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#define SCREEN_WIDTH (700.0f)
#define SCREEN_HEIGHT (400.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (100)
#define MAX_ENEMIES (100)

typedef enum State
{
    STATE_MAIN_MENU,
    STATE_RUNNING,
    STATE_GAME_OVER,
    STATE_PAUSED,
} State;

typedef struct Enemy {
    Vector2 position;
    int health;
    float size;
    float velocity;
} Enemy;

typedef struct Bullet {
    Vector2 position;
    float velocity;
    float damage;
} Bullet;

typedef struct GameState {
    // General
    int score;
    State state;
    float gameTime;
    // Player
    float playerVelocity;
    Vector2 playerPosition;
    int playerHealth;
    // Projectiles
    Bullet bullets[MAX_BULLETS];
    int bulletCount;
    float shootDelay;
    float shootTime;
    // Enemies
    Enemy enemies[MAX_ENEMIES];
    int enemyCount;
    float spawnTime;
    float enemySpawnRate;
} GameState;

static const GameState defaultGameState = {
    .playerPosition = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
    .playerHealth = 3,
    .state = STATE_MAIN_MENU,
    .shootDelay = 1.0f/4.0f,
    .gameTime = 0.0f,
    .bulletCount = 0,
    .playerVelocity = 200,
    .shootTime = 0.0f,
    .enemyCount = 0,
    .enemySpawnRate = 0.5f,
    .spawnTime = 0.0,
    .score = 0,
};

void draw_text_centered(const char* text, Vector2 pos, float fontSize, Color color)
{
	const Vector2 textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1);
    pos.x -= textSize.x / 2.0f;
    pos.y -= textSize.y / 2.0f;
	DrawText(text, pos.x, pos.y, fontSize, color);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(60);
    
    GameState gameState = defaultGameState;
    
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        
        const Rectangle screenRect = {
            .height = SCREEN_HEIGHT,
            .width = SCREEN_WIDTH,
            .x = 0,
            .y = 0
        };

        switch (gameState.state) {
            case STATE_MAIN_MENU:
            {
                draw_text_centered("Asteroids", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, WHITE);
                draw_text_centered("<Press enter to play>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 30}, 20, WHITE);
                draw_text_centered("<WASD to move, space to shoot, p to pause>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 50}, 20, WHITE);
                draw_text_centered("v0.1", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT - 15}, 15, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {                    
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
            case STATE_RUNNING:
            {
                gameState.gameTime += GetFrameTime(); 
                               
                Texture2D texture = LoadTexture("assets/rocket.png");
                const int texture_x = gameState.playerPosition.x - texture.width / 2;
                const int texture_y = gameState.playerPosition.y - texture.height / 2;
                DrawTextureEx(texture, (Vector2) {texture_x, texture_y}, 0.0, 1.0, WHITE);

                if (IsKeyDown(KEY_W)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, 0}, (Vector2) {SCREEN_WIDTH, 0}, texture.height / 2))
                    {
                        gameState.playerPosition.y -= gameState.playerVelocity * GetFrameTime();
                    }   
                }
                if (IsKeyDown(KEY_S)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, SCREEN_HEIGHT}, (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, texture.height / 2))
                    {
                        gameState.playerPosition.y += gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_A)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, 0}, (Vector2) {0, SCREEN_HEIGHT}, texture.width / 2))
                    {
                        gameState.playerPosition.x -= gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_D)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {SCREEN_WIDTH, 0}, (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, texture.width / 2))
                    {
                        gameState.playerPosition.x += gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (gameState.shootTime < gameState.shootDelay)
                {
                    gameState.shootTime += GetFrameTime();
                } 
                while (IsKeyDown(KEY_SPACE) && gameState.shootTime >= gameState.shootDelay) 
                {
                    if (gameState.bulletCount >= MAX_BULLETS)
                    {
                        break;
                    }  

                    Bullet bullet = 
                    {
                        .position = gameState.playerPosition,
                        .velocity = 100.0f,
                        .damage = 1.0,
                    };
                    gameState.bullets[gameState.bulletCount++] = bullet;
                    gameState.shootTime -= gameState.shootDelay;
                    Texture2D texture = LoadTexture("assets/bullet.png");
                    const int texture_x = gameState.playerPosition.x - texture.width / 2;
                    const int texture_y = gameState.playerPosition.y - texture.height * 2 - 2;
                    DrawTexture(texture, texture_x, texture_y, WHITE);
                }
                // Update Bullets
                {
                    for (int bulletIndex = 0; bulletIndex < gameState.bulletCount; bulletIndex++)
                    {
 
                        Bullet* bullet = &gameState.bullets[bulletIndex];
                        if(!CheckCollisionPointRec(bullet->position, screenRect))
						{
							// Replace with last projectile
							*bullet = gameState.bullets[--gameState.bulletCount];
						}
                        bullet->position.y -= bullet->velocity * GetFrameTime();
                        Texture2D texture = LoadTexture("assets/bullet.png");
                        const int texture_x = bullet->position.x - texture.width / 2;
                        const int texture_y = bullet->position.y - texture.height / 2;
                        DrawTexture(texture, texture_x, texture_y, WHITE);
                    }
                        
                }
                // Spawn Enemies
                {
                    gameState.spawnTime += GetFrameTime();
                    if (gameState.spawnTime > gameState.enemySpawnRate) 
                    {
                        if (gameState.enemyCount >= MAX_ENEMIES)
                        {
                            break;
                        }
                        float size = GetRandomValue(100.0, 300.0) / 100.0f;   
                        float enemyXPosition = GetRandomValue(0, SCREEN_WIDTH); 
                        Enemy enemy = {
                            .position = (Vector2) {enemyXPosition, 0},
                            .health = (int) size * 2,
                            .velocity = 50.0,
                            .size = size,
                        };
                        gameState.spawnTime = 0;
                        gameState.enemies[gameState.enemyCount++] = enemy;
                    }
                }
                // Update Enemies
                {
                    for (int enemyIndex = 0; enemyIndex < gameState.enemyCount; enemyIndex++)
                    {
                        Enemy* enemy = &gameState.enemies[enemyIndex];
                        enemy->position.y += enemy->velocity * GetFrameTime();
                        Rectangle enemyRec = {
                            .width = 31.0f * enemy->size,
                            .height = 28.0f * enemy->size,
                            .x = enemy->position.x - 31.0f/2.0f * enemy->size,
                            .y = enemy->position.y - 28.0f/2.0f * enemy->size,
                        };
                        for (int bulletIndex = 0; bulletIndex < gameState.bulletCount; bulletIndex++)
                        {
                            Bullet* bullet = &gameState.bullets[bulletIndex];

                            Rectangle bulletRec = {
                                .width = 2.0f,
                                .height = 7.0f,
                                .x = gameState.bullets[bulletIndex].position.x - 2.0f/2,
                                .y = gameState.bullets[bulletIndex].position.y - 7.0f/2,
                            };
                            // DrawRectangleRec(bulletRec, GREEN);
                            if(CheckCollisionRecs(enemyRec, bulletRec))
                            {
                                // Replace with bullet with last bullet
                                *bullet = gameState.bullets[--gameState.bulletCount];
                                if (--enemy->health < 1)
                                {
                                    gameState.score += (int)(enemy->size * 100);
                                    *enemy = gameState.enemies[--gameState.enemyCount];
                                }
                            }
                        }
                        Rectangle playerRec = {
                                .width = 26,
                                .height = 27,
                                .x = gameState.playerPosition.x - 26/2,
                                .y = gameState.playerPosition.y - 27/2,
                        };

                        // DrawRectangleRec(enemyRec, RED); 
                        // DrawRectangleRec(playerRec, BLUE);
                        if(CheckCollisionRecs(enemyRec, playerRec))
                        {
                            *enemy = gameState.enemies[--gameState.enemyCount];
                            if(--gameState.playerHealth < 1) 
                            {
                                gameState.state = STATE_GAME_OVER;
                            }
                        }
                        Texture2D texture = LoadTexture("assets/asteroid.png");
                        const int texture_x = enemy->position.x - texture.width / 2 * enemy->size;
                        const int texture_y = enemy->position.y - texture.height / 2 * enemy->size;
                        DrawTextureEx(texture, (Vector2) {texture_x, texture_y}, 0.0, enemy->size, WHITE);
                    }
                    for (int i = 1; i <= gameState.playerHealth; i++)
                    {
                        Texture2D texture = LoadTexture("assets/heart.png");
                        const int texture_x = i * 16.0;
                        const int texture_y = texture.height;
                        DrawTexture(texture, texture_x, texture_y, WHITE);
                    }
                }
                // Update Score
                {
                    char scoreText[100] = { 0 };
                    int fontSize = 15;
				    sprintf(scoreText, "Score: %d", gameState.score);
                    const Vector2 textSize = MeasureTextEx(GetFontDefault(), scoreText, fontSize, 1);
                    DrawText(scoreText, SCREEN_WIDTH - textSize.x - 20.0, 20.0, fontSize, WHITE);
                }

                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_PAUSED;
                }
                break;
            }
            case STATE_GAME_OVER:
            {
                ClearBackground(BLACK);
                draw_text_centered("GAME OVER", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, WHITE);
                char scoreText[100] = {0};
                sprintf(scoreText, "Final score: %d", gameState.score);
                draw_text_centered(scoreText, (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 30}, 20, WHITE);
                draw_text_centered("<Press enter to try again>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 60}, 20, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    gameState = defaultGameState;
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
            case STATE_PAUSED:
            {
                draw_text_centered("Game is paused...", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, WHITE);
                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    printf("%f", gameState.playerPosition.x);
    return 0;
}
