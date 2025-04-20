// To build on linux: 
// gcc asteroids.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
#define RAYMATH_IMPLEMENTATION
#include "raylib.h"
// #include <stdlib.h>
#include <stdio.h>
// #define ASSETS_IMPLEMENTATION
#include "assets.h"

#define SCREEN_WIDTH (700.0f)
#define SCREEN_HEIGHT (400.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (50)
#define MAX_ENEMIES (40)
#define MAX_STARS (20)
// #define PARALLAX_LAYERS (2)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


typedef enum State
{
    STATE_MAIN_MENU,
    STATE_RUNNING,
    STATE_GAME_OVER,
    STATE_PAUSED,
    STATE_UPGRADE,
} State;

typedef struct Star {
    Vector2 position;
    float size;
    float velocity;
    int imgIndex;
    float alpha;
    Texture2D texture;
} Star;

typedef struct Enemy {
    Vector2 position;
    int health;
    float size;
    float velocity;
    char textureFile[100];
    int type;
    Texture2D texture;
} Enemy;

typedef struct Bullet {
    Vector2 position;
    float velocity;
    float damage;
    Texture2D texture;
} Bullet;

typedef struct GameState {
    // General
    int experience;
    State state;
    float gameTime;
    // Player
    float playerVelocity;
    Vector2 playerPosition;
    int playerHealth;
    bool playerMultishot;
    SpriteAnimation playerAnimation;
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
    float lastEnemyXPosition;
    // Parallax background stars
    Star stars[MAX_STARS];
    int starCount;
    float starTime;
    float starSpawnRate;
    int initStars;
} GameState;

static const GameState defaultGameState = {
    .playerPosition = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
    .playerHealth = 3,
    .shootDelay = 1.0f/4.0f,
    .state = STATE_MAIN_MENU,
    .gameTime = 0.0f,
    .bulletCount = 0,
    .playerVelocity = 200,
    .playerMultishot = false,
    .shootTime = 0.0f,
    .enemyCount = 0,
    .enemySpawnRate = 0.5f,
    .spawnTime = 0.0,
    .experience = 999,
    .starCount = 0,
    .starTime = 0,
    .starSpawnRate = 0.25f,
    .lastEnemyXPosition = 0.0f,
    .initStars = 0,
};

void draw_text_centered(Font font, const char* text, Vector2 pos, float fontSize, float fontSpacing, Color color)
{
	const Vector2 textSize = MeasureTextEx(font, text, fontSize, fontSpacing);
    pos.x -= textSize.x / 2.0f;
    pos.y -= textSize.y / 2.0f;
	DrawTextEx(font, text, (Vector2){pos.x, pos.y}, fontSize, fontSpacing, color);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(240);
    
    Font font = LoadFont("fonts/setback.png");
    int fontSpacing = 1;
    int fontSize = 15;

    GameState gameState = defaultGameState;
    TextureAtlas atlas;
    // Texture2D playerTexture = LoadTexture("assets/rocketNew.png");
    // gameState.playerTexture = playerTexture;
    // gameState.playerAnimation = createSpriteAnimation(playerTexture, 10, (Rectangle[]) {
    //     (Rectangle){0,0,32,62},   (Rectangle){32,0,32,62},  (Rectangle){64,0,32,62},  (Rectangle){96,0,32,62},
    //     (Rectangle){128,0,32,62}, (Rectangle){160,0,32,62}, (Rectangle){192,0,32,62}, (Rectangle){224,0,32,62},
    //     (Rectangle){256,0,32,62}, (Rectangle){288,0,32,62}, (Rectangle){320,0,32,62}, (Rectangle){352,0,32,62},
    //     (Rectangle){384,0,32,62}, (Rectangle){416,0,32,62}, (Rectangle){448,0,32,62}, (Rectangle){480,0,32,62},
    //     (Rectangle){512,0,32,62}, (Rectangle){544,0,32,62}, (Rectangle){576,0,32,62}, (Rectangle){608,0,32,62},        
    // }, 20);
    initTextureAtlas(&atlas);

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

        // Font font = LoadFont("fonts/alagard.png");
        // Font font = GetFontDefault(); 
        

        switch (gameState.state) {
            case STATE_MAIN_MENU:
            {
                Color backgroundColor = ColorFromHSV(259, 1, 0.07);
                ClearBackground(backgroundColor);
                draw_text_centered(font, "Asteroids", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, fontSpacing, WHITE);
                draw_text_centered(font, "<Press enter to play>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 30}, 20, fontSpacing, WHITE);
                draw_text_centered(font, "<WASD to move, space to shoot, p to pause>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 50}, 20,fontSpacing, WHITE);
                draw_text_centered(font, "v0.1", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT - 15}, 15, fontSpacing, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {                    
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
            case STATE_RUNNING:
            {
                Color backgroundColor = ColorFromHSV(258, 1, 0.07);
                ClearBackground(backgroundColor);
                
                // Spawn initial stars for parallax
                {
                    if (gameState.initStars == 0)
                    {
                        while(gameState.starCount < MAX_STARS)
                        {                            
                            int imgIndex = GetRandomValue(1, 2);   
                            // char imgCharBuffer[100] = { 0 };
                            float starXPosition = GetRandomValue(0, SCREEN_WIDTH); 
                            float starYPosition = GetRandomValue(0, SCREEN_HEIGHT); 
                            // sprintf(imgCharBuffer, "assets/star%d.png", imgIndex);
                            Texture2D texture;
                            if (imgIndex == 1)
                            {
                                texture = atlas.starTexture1;
                            } 
                            else
                            {
                                texture = atlas.starTexture2;
                            }

                            Star star = {
                                .position = (Vector2) {starXPosition, starYPosition},
                                .velocity = 30.0f * GetRandomValue(1,2) * imgIndex,
                                .size = 1,
                                .texture = texture,
                                .alpha = 0.5 * imgIndex,
                            };
                            gameState.stars[gameState.starCount++] = star;
                        }
                        gameState.initStars = 1;
                    }
                }
                // Spawn new stars for parallax
                {
                    gameState.starTime += GetFrameTime();
                    if (gameState.starTime > gameState.starSpawnRate) 
                    {
                        gameState.starTime = 0;
                        if (gameState.starCount < MAX_STARS)
                        {
                            int imgIndex = GetRandomValue(1, 2);  
                            // char imgCharBuffer[100] = { 0 }; 
                            Texture2D texture;
                            if (imgIndex == 1)
                            {
                                texture = atlas.starTexture1;
                            } 
                            else if (imgIndex == 2)
                            {
                                texture = atlas.starTexture2;
                            }
                            float starXPosition = GetRandomValue(0, SCREEN_WIDTH); 
                            // sprintf(imgCharBuffer, "assets/star%d.png", imgIndex);
                            Star star = {
                                .position = (Vector2) {starXPosition, 0},
                                .velocity = 30.0f * GetRandomValue(1,2) * imgIndex,
                                .size = 1,
                                .texture = texture,
                                .alpha = 0.5f * imgIndex,
                            };
                            gameState.stars[gameState.starCount++]= star;
                        }
                    }
                }
                // Update Stars
                {
                    for (int starIndex = 0; starIndex < gameState.starCount; starIndex++)
                    {
                        Star* star = &gameState.stars[starIndex];
                        if(!CheckCollisionPointRec(star->position, screenRect))
						{
							// Replace with last star
							*star = gameState.stars[--gameState.starCount];
						}
                        star->position.y += star->velocity * GetFrameTime();
                        // char imgCharBuffer[100] = { 0 };
                        
                        const int texture_x = star->position.x - star->texture.width / 2.0 * star->size;
                        const int texture_y = star->position.y - star->texture.height / 2.0 * star->size;
                        Color starColor = ColorAlpha(WHITE, star->alpha);
                        DrawTextureEx(star->texture, (Vector2) {texture_x, texture_y}, 0.0, star->size, starColor);
                    }
                }
                // Update game time
                gameState.gameTime += GetFrameTime(); 
                
                const int texture_x = gameState.playerPosition.x - 32.0f / 2.0;
                const int texture_y = gameState.playerPosition.y - 62.0f / 2.0;

                if (IsKeyDown(KEY_W)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, 0}, (Vector2) {SCREEN_WIDTH, 0}, 62.0f / 2))
                    {
                        gameState.playerPosition.y -= gameState.playerVelocity * GetFrameTime();
                    }   
                }
                if (IsKeyDown(KEY_S)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, SCREEN_HEIGHT}, (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, 62.0f / 2))
                    {
                        gameState.playerPosition.y += gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_A)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {0, 0}, (Vector2) {0, SCREEN_HEIGHT}, 32.0f / 2))
                    {
                        gameState.playerPosition.x -= gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_D)) {
                    if(!CheckCollisionPointLine(gameState.playerPosition, (Vector2) {SCREEN_WIDTH, 0}, (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, 32.0f / 2))
                    {
                        gameState.playerPosition.x += gameState.playerVelocity * GetFrameTime();
                    }
                }
                if (gameState.shootTime < gameState.shootDelay)
                {
                    gameState.shootTime += GetFrameTime();
                } 
                // Shoot bullets
                while (IsKeyDown(KEY_SPACE) && gameState.shootTime >= gameState.shootDelay) 
                {
                    if (gameState.bulletCount >= MAX_BULLETS)
                    {
                        break;
                    }

                    if (gameState.playerMultishot == true)
                    {
                        Bullet bullet1 = 
                        {
                            .position = gameState.playerPosition,
                            .velocity = 100.0f,
                            .damage = 1.0,
                            .texture = atlas.bulletTexture,
                        };
                        gameState.bullets[gameState.bulletCount++] = bullet1;
                        float texture_x = gameState.playerPosition.x - bullet1.texture.width / 2.0;
                        float texture_y = gameState.playerPosition.y - atlas.playerTexture.height / 2.0 - bullet1.texture.height / 2.0;
                        DrawTexture(bullet1.texture, texture_x, texture_y, WHITE);
                        Bullet bullet2 = 
                        {
                            .position = (Vector2){gameState.playerPosition.x - 40.0f, gameState.playerPosition.y + 0.0f},
                            .velocity = 100.0f,
                            .damage = 1.0,
                            .texture = atlas.bulletTexture,
                        };
                        gameState.bullets[gameState.bulletCount++] = bullet2;
                        texture_x = gameState.playerPosition.x - 40.0f - bullet2.texture.width / 2.0;
                        texture_y = gameState.playerPosition.y - atlas.playerTexture.height / 2.0 - bullet2.texture.height / 2.0;
                        DrawTexture(bullet2.texture, texture_x, texture_y, WHITE);
                        Bullet bullet3 = 
                        {
                            .position = (Vector2){gameState.playerPosition.x + 40.0f, gameState.playerPosition.y + 0.0f},
                            .velocity = 100.0f,
                            .damage = 1.0,
                            .texture = atlas.bulletTexture,
                        };
                        gameState.bullets[gameState.bulletCount++] = bullet3;
                        texture_x = gameState.playerPosition.x + 40.0f - bullet3.texture.width / 2.0;
                        texture_y = gameState.playerPosition.y - atlas.playerTexture.height / 2.0 - bullet3.texture.height / 2.0;
                        DrawTexture(bullet3.texture, texture_x, texture_y, WHITE);
                        gameState.shootTime -= gameState.shootDelay;
                    }
                    else
                    {
                        Bullet bullet =
                            {
                                .position = gameState.playerPosition,
                                .velocity = 100.0f,
                                .damage = 1.0,
                                .texture = atlas.bulletTexture,
                            };
                        gameState.bullets[gameState.bulletCount++] = bullet;
                        gameState.shootTime -= gameState.shootDelay;
                        const int texture_x = gameState.playerPosition.x - bullet.texture.width / 2.0;
                        const int texture_y = gameState.playerPosition.y - atlas.playerTexture.height / 2.0 - bullet.texture.height / 2.0;
                        DrawTexture(bullet.texture, texture_x, texture_y, WHITE);
                    }
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
                        const int texture_x = bullet->position.x - bullet->texture.width / 2.0;
                        const int texture_y = bullet->position.y - atlas.playerTexture.height / 2.0 - bullet->texture.height / 2.0;
                        DrawTexture(bullet->texture, texture_x, texture_y, WHITE);
                    }
                }
                // Update Player
                Rectangle destination = {texture_x, texture_y, 32, 62}; // origin in coordinates and scale
                Vector2 origin = { 0 }; // so it draws from top left of image
                DrawSpriteAnimationPro(atlas.playerAnimation, destination, origin, 0, WHITE);

                // Spawn Enemies
                {
                    gameState.spawnTime += GetFrameTime();
                    if (gameState.spawnTime > gameState.enemySpawnRate && gameState.enemyCount < MAX_ENEMIES) 
                    {
                        float size = GetRandomValue(100.0, 300.0) / 100.0f;
                        float minSpawnDistance = 31.0f * size;  
                        float enemyXPosition = MAX(minSpawnDistance, GetRandomValue(0, SCREEN_WIDTH)); 
                        Enemy enemy = {
                            .position = (Vector2) {enemyXPosition, 0},
                            .health = (int) (size+1.0) * 2.0,
                            .velocity = 35.0,
                            .size = size,
                        };
                        int whichAsteroid = GetRandomValue(1,10);
                        Texture2D enemyTexture;
                        if (whichAsteroid < 4)
                        {
                            enemyTexture = atlas.enemyTexture2;
                            // sprintf(enemy.textureFile, "%s", "assets/asteroid2.png");
                            enemy.type = 2;
                        } 
                        else 
                        {
                            enemyTexture = atlas.enemyTexture1;
                            enemy.type = 1;
                        }
                        enemy.texture = enemyTexture;
                        enemy.position.y -= enemy.texture.height; // to make them come into screen smoothly
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
                            .width = enemy->texture.width * enemy->size,
                            .height = enemy->texture.height * enemy->size,
                            .x = enemy->position.x - enemy->texture.width/2.0f * enemy->size,
                            .y = enemy->position.y - enemy->texture.height/2.0f * enemy->size,
                        };
                        for (int bulletIndex = 0; bulletIndex < gameState.bulletCount; bulletIndex++)
                        {
                            Bullet* bullet = &gameState.bullets[bulletIndex];

                            Rectangle bulletRec = {
                                .width = 2.0f,
                                .height = 7.0f,
                                .x = gameState.bullets[bulletIndex].position.x - 2.0f/2.0,
                                .y = gameState.bullets[bulletIndex].position.y - 7.0f/2.0,
                            };
                            // DrawRectangleRec(bulletRec, GREEN);
                            if(CheckCollisionRecs(enemyRec, bulletRec))
                            {
                                // Replace with bullet with last bullet
                                *bullet = gameState.bullets[--gameState.bulletCount];
                                if (--enemy->health < 1)
                                {
                                    gameState.experience += (int)(enemy->size * 100);
                                    *enemy = gameState.enemies[--gameState.enemyCount];
                                }
                            }
                        }
                        Rectangle playerRec = {
                            .width = 32,
                            .height = 62,
                            .x = gameState.playerPosition.x - 32.0f/2.0,
                            .y = gameState.playerPosition.y - 62.0f/2.0,
                        };
                        // DrawRectangleRec(enemyRec, RED); 
                        // DrawRectangleRec(playerRec, BLUE);
                        Rectangle screenRectExtended = {
                            .width = SCREEN_WIDTH + enemy->texture.width,
                            .height = SCREEN_HEIGHT + enemy->texture.height,
                            .x = 0.0,
                            .y = enemy->position.y, // to make them scroll into screen smoothly
                        };
                        if(!CheckCollisionPointRec(enemy->position, screenRectExtended))
                        {
                            // Replace with last enemy
                            *enemy = gameState.enemies[--gameState.enemyCount];
                        }
                        if(CheckCollisionRecs(enemyRec, playerRec))
                        {
                            *enemy = gameState.enemies[--gameState.enemyCount];
                            if(--gameState.playerHealth < 1) 
                            {
                                gameState.state = STATE_GAME_OVER;
                            }
                        }

                        const int texture_x = enemy->position.x - enemy->texture.width / 2.0 * enemy->size;
                        const int texture_y = enemy->position.y - enemy->texture.height / 2.0 * enemy->size;
                        DrawTextureEx(enemy->texture, (Vector2){texture_x, texture_y}, 0.0, enemy->size, WHITE);
                    }
                }
                // Update player health
                {
                    for (int i = 1; i <= gameState.playerHealth; i++)
                    {
                        const int texture_x = i * 16.0;
                        const int texture_y = atlas.healthTexture.height;
                        DrawTexture(atlas.healthTexture, texture_x, texture_y, WHITE);
                    }
                }
                // Update Score
                {
                    if (gameState.experience > 1000.0 && gameState.state != STATE_UPGRADE)
                    {
                        gameState.state = STATE_UPGRADE;
                        gameState.experience -= 1000.0;
                    }
                    float recPosX = SCREEN_WIDTH - 115.0;
                    float recPosY = 20.0;
                    float recHeight = 30.0;
                    float recWidth = 100.0;
                    DrawRectangle(recPosX, recPosY, gameState.experience / 10.0, recHeight, ColorAlpha(BLUE, 0.5));
                    DrawRectangleLines(recPosX, recPosY, recWidth, recHeight, ColorAlpha(WHITE, 0.5));
                    char experienceText[100] = "XP";
                    Vector2 textSize = MeasureTextEx(font, experienceText, fontSize, fontSpacing);
                    DrawTextEx(font, experienceText, (Vector2){recPosX + recWidth / 2.0 - textSize.x / 2.0, recPosY + recHeight / 2.0 - textSize.y / 2.0}, 20.0, fontSpacing, WHITE);
                }

                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_PAUSED;
                }
                
                break;
            }
            case STATE_UPGRADE:
            {
                ClearBackground(BLACK);
                draw_text_centered(font, "LEVEL UP!", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f - 35.0}, 40, fontSpacing, WHITE);
                draw_text_centered(font, "CHOOSE UPGRADE", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f - 80.0}, 40, fontSpacing, WHITE);
                const int texture_x = atlas.upgradeMultishotTexture.width;
                const int texture_y = atlas.upgradeMultishotTexture.height;
                const int spacing_x = 10;
                DrawTexture(atlas.upgradeMultishotTexture, SCREEN_WIDTH/2.0f - texture_x/2.0f, SCREEN_HEIGHT/2.0f - texture_y/2.0f + 30.0, WHITE);
                DrawTexture(atlas.upgradeDamageTexture, SCREEN_WIDTH/2.0f - texture_x/2.0f - texture_x - spacing_x, SCREEN_HEIGHT/2.0f - texture_y/2.0f + 30.0, WHITE);
                DrawTexture(atlas.upgradeFirerateTexture, SCREEN_WIDTH/2.0f - texture_x/2.0f + texture_x + spacing_x, SCREEN_HEIGHT/2.0f - texture_y/2.0f + 30.0, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {                    
                    gameState.playerMultishot = true;
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
            case STATE_GAME_OVER:
            {
                ClearBackground(BLACK);
                draw_text_centered(font, "GAME OVER", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, fontSpacing, WHITE);
                char scoreText[100] = {0};
                sprintf(scoreText, "Final score: %d", gameState.experience);
                draw_text_centered(font, scoreText, (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 30}, 20, fontSpacing, WHITE);
                draw_text_centered(font, "<Press enter to try again>", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f + 60}, 20, fontSpacing, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    gameState = defaultGameState;
                    initTextureAtlas(&atlas);
                    Texture2D playerTexture = LoadTexture("assets/rocketNew.png");
                    atlas.playerTexture = playerTexture;
                    atlas.playerAnimation = createSpriteAnimation(playerTexture, 10, (Rectangle[]) {
                        (Rectangle){0,0,32,62},   (Rectangle){32,0,32,62},  (Rectangle){64,0,32,62},  (Rectangle){96,0,32,62},
                        (Rectangle){128,0,32,62}, (Rectangle){160,0,32,62}, (Rectangle){192,0,32,62}, (Rectangle){224,0,32,62},
                        (Rectangle){256,0,32,62}, (Rectangle){288,0,32,62}, (Rectangle){320,0,32,62}, (Rectangle){352,0,32,62},
                        (Rectangle){384,0,32,62}, (Rectangle){416,0,32,62}, (Rectangle){448,0,32,62}, (Rectangle){480,0,32,62},
                        (Rectangle){512,0,32,62}, (Rectangle){544,0,32,62}, (Rectangle){576,0,32,62}, (Rectangle){608,0,32,62},        
                    }, 20);
                    gameState.state = STATE_RUNNING;

                }
                break;
            }
            case STATE_PAUSED:
            {
                draw_text_centered(font, "Game is paused...", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, fontSpacing, WHITE);
                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_RUNNING;
                }
                break;
            }
        }
        DrawFPS(10, 40);
        EndDrawing();
    }

    FreeSpriteAnimation(atlas.playerAnimation);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
