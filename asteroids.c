// To build on linux: 
// gcc asteroids.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
#define RAYMATH_IMPLEMENTATION
#include "raylib.h"
// #include <stdlib.h>
#include <stdio.h>
#include <math.h>
// #define ASSETS_IMPLEMENTATION
#include "assets.h"

// #define SCREEN_WIDTH (700.0f)
// #define SCREEN_HEIGHT (400.0f)
#define SCREEN_WIDTH (1400.0f)
#define SCREEN_HEIGHT (700.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (1000)
#define MAX_ENEMIES (40)
#define MAX_STARS (50)
#define TARGET_FPS (240)

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

typedef enum Upgrade
{
    UPGRADE_MULTISHOT,
    UPGRADE_DAMAGE,
    UPGRADE_FIRERATE,
    UPGRADE_COUNT,
} Upgrade;

typedef struct Star {
    Vector2 position;
    float size;
    float velocity;
    int imgIndex;
    float alpha;
    Sprite sprite;
} Star;

typedef struct Enemy {
    Vector2 position;
    int health;
    float size;
    float velocity;
    float rotation;
    char textureFile[100];
    int type;
    Sprite sprite;
} Enemy;

typedef struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float damage;
    // Texture2D texture;
    Sprite sprite;
} Bullet;

typedef struct Player {
    float playerVelocity;
    Vector2 playerPosition;
    int playerHealth;
    Sprite sprite;
    float size;
    int animationFrames;
    float invulTime;
    float invulDuration;
    bool playerMultishot;
    float fireRate;
    float damageMulti;
} Player;

typedef struct GameState {
    // General
    int experience;
    State state;
    float gameTime;
    // Player
    Player player;
    // float playerVelocity;
    // Vector2 playerPosition;
    // int playerHealth;
    // bool playerMultishot;
    // Projectiles
    Bullet bullets[MAX_BULLETS];
    int bulletCount;
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
    Upgrade pickedUpgrade;
} GameState;

void initialize(GameState* gameState) {
    *gameState = (GameState) {
        .state = STATE_MAIN_MENU,
        .gameTime = 0.0f,
        .bulletCount = 0,
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
        .pickedUpgrade = UPGRADE_MULTISHOT,
    };

    gameState->player = (Player) {
        .playerVelocity = 200,
        .playerPosition = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
        .playerHealth = 3,
        .playerMultishot = false,
        .sprite = getSprite(SPRITE_PLAYER),
        .size = 2,
        .animationFrames = 5,
        .invulTime = 0.0f,
        .invulDuration = 3.0f,
        .fireRate = 1.0f,
        .damageMulti = 1.0f,
    };
}

void draw_text_centered(Font font, const char* text, Vector2 pos, float fontSize, float fontSpacing, Color color)
{
	const Vector2 textSize = MeasureTextEx(font, text, fontSize, fontSpacing);
    pos.x -= textSize.x / 2.0f;
    pos.y -= textSize.y / 2.0f;
	DrawTextEx(font, text, (Vector2){pos.x, pos.y}, fontSize, fontSpacing, color);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    
    // Font font = LoadFont("fonts/setback.png");
    Font font = LoadFont("fonts/jupiter_crash.png");
    // Font font = LoadFont("fonts/mecha.png");
    int fontSpacing = 1;
    int fontSize = 15;

    GameState gameState;
    initialize(&gameState);

    // gameState.bullets[0].position = (Vector2){0, 0};
    TextureAtlas atlas = initTextureAtlas();

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
                            float starXPosition = GetRandomValue(0, SCREEN_WIDTH); 
                            float starYPosition = GetRandomValue(0, SCREEN_HEIGHT); 
                            Sprite sprite;
                            if (imgIndex == 1)
                            {
                                sprite = getSprite(SPRITE_STAR1);
                            } 
                            else
                            {
                                sprite = getSprite(SPRITE_STAR2);
                            }

                            Star star = {
                                .position = (Vector2) {starXPosition, starYPosition},
                                .velocity = 30.0f * GetRandomValue(1,2) * imgIndex,
                                .size = 1,
                                .sprite = sprite,
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
                            Sprite starSprite;
                            if (imgIndex == 1)
                            {
                                starSprite = getSprite(SPRITE_STAR1);
                            } 
                            else if (imgIndex == 2)
                            {
                                starSprite = getSprite(SPRITE_STAR2);
                            }
                            float starXPosition = GetRandomValue(0, SCREEN_WIDTH); 
                            Star star = {
                                .position = (Vector2) {starXPosition, 0},
                                .velocity = 30.0f * GetRandomValue(1,2) * imgIndex,
                                .size = 1,
                                .alpha = 0.5f * imgIndex,
                                .sprite = starSprite,
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
                        
                        const int texture_x = star->position.x - star->sprite.coords.width / 2.0 * star->size;
                        const int texture_y = star->position.y - star->sprite.coords.height / 2.0 * star->size;
                        Color starColor = ColorAlpha(WHITE, star->alpha);
                        DrawTextureRec(atlas.textureAtlas, getSprite(SPRITE_STAR1).coords, (Vector2) {texture_x, texture_y}, starColor);
                    }
                }
                // Update game time
                gameState.gameTime += GetFrameTime(); 
                if (gameState.player.invulTime < 0.0f)
                {
                    gameState.player.invulTime = 0.0f;
                } else {
                    gameState.player.invulTime -= GetFrameTime();
                }

                if (IsKeyDown(KEY_W)) {
                    if(!CheckCollisionPointLine(gameState.player.playerPosition, (Vector2) {0, 0}, (Vector2) {SCREEN_WIDTH, 0}, gameState.player.sprite.coords.width/gameState.player.animationFrames / 2))
                    {
                        gameState.player.playerPosition.y -= gameState.player.playerVelocity * GetFrameTime();
                    }   
                }
                if (IsKeyDown(KEY_S)) {
                    if(!CheckCollisionPointLine(gameState.player.playerPosition, (Vector2) {0, SCREEN_HEIGHT}, (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, gameState.player.sprite.coords.height / 2))
                    {
                        gameState.player.playerPosition.y += gameState.player.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_A)) {
                    if(!CheckCollisionPointLine(gameState.player.playerPosition, (Vector2) {0, 0}, (Vector2) {0, SCREEN_HEIGHT}, gameState.player.sprite.coords.width/gameState.player.animationFrames / 2))
                    {
                        gameState.player.playerPosition.x -= gameState.player.playerVelocity * GetFrameTime();
                    }
                }
                if (IsKeyDown(KEY_D)) {
                    if(!CheckCollisionPointLine(gameState.player.playerPosition, 
                                                (Vector2) {SCREEN_WIDTH, 0},
                                                (Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT}, 
                                                gameState.player.sprite.coords.width/gameState.player.animationFrames / 2))
                    {
                        gameState.player.playerPosition.x += gameState.player.playerVelocity * GetFrameTime();
                    }
                }
                if (gameState.shootTime < 1.0f/gameState.player.fireRate)
                {
                    gameState.shootTime += GetFrameTime();
                } 
                // Shoot bullets
                while (IsKeyDown(KEY_SPACE) 
                    && gameState.shootTime >= 1.0f/gameState.player.fireRate
                    && gameState.bulletCount <= MAX_BULLETS)
                {
                    // if (gameState.bulletCount >= MAX_BULLETS)
                    // {
                    //     printf("DEBUG: Max bullets reached\n");
                    //     break;
                        // continue;
                    // } else {
                        if (gameState.player.playerMultishot == true && gameState.bulletCount < MAX_BULLETS-3)
                        {
                            float bulletOffset = 0.0f;
                            Bullet bullet1 = 
                            {
                                .position = gameState.player.playerPosition,
                                .velocity = (Vector2){0.0f, 100.0f},
                                .damage = 1.0*gameState.player.damageMulti,
                                .sprite = getSprite(SPRITE_BULLET),
                            };
                            bullet1.position.x -= bullet1.sprite.coords.width / 2.0f;
                            bullet1.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet1.sprite.coords.height;
                            gameState.bullets[gameState.bulletCount++] = bullet1;
                            float texture_x = gameState.player.playerPosition.x - bullet1.sprite.coords.width / 2.0;
                            float texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0 - bullet1.sprite.coords.height / 2.0;
                            DrawTextureRec(atlas.textureAtlas, bullet1.sprite.coords, (Vector2){texture_x, texture_y}, WHITE);
                            Bullet bullet2 = 
                            {
                                .position = (Vector2){gameState.player.playerPosition.x - bulletOffset, gameState.player.playerPosition.y + 0.0f},
                                .velocity = (Vector2){sqrt(pow(100.0f,2) - pow(95.0f,2)), 95.0f},
                                .damage = 1.0*gameState.player.damageMulti,
                                .sprite = getSprite(SPRITE_BULLET),
                            };
                            bullet2.position.x -= bullet2.sprite.coords.width / 2.0f;
                            bullet2.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet2.sprite.coords.height;
                            gameState.bullets[gameState.bulletCount++] = bullet2;
                            texture_x = gameState.player.playerPosition.x - bulletOffset - bullet2.sprite.coords.width / 2.0;
                            texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0 - bullet2.sprite.coords.height / 2.0;
                            DrawTextureRec(atlas.textureAtlas, bullet2.sprite.coords, (Vector2){texture_x, texture_y}, WHITE);
                            Bullet bullet3 = 
                            {
                                .position = (Vector2){gameState.player.playerPosition.x + bulletOffset, gameState.player.playerPosition.y + 0.0f},
                                .velocity = (Vector2){-sqrt(pow(100.0f,2) - pow(95.0f,2)), 95.0f},
                                .damage = 1.0*gameState.player.damageMulti,
                                .sprite = getSprite(SPRITE_BULLET),
                            };
                            bullet3.position.x -= bullet3.sprite.coords.width / 2.0f;
                            bullet3.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet3.sprite.coords.height;
                            gameState.bullets[gameState.bulletCount++] = bullet3;
                            texture_x = gameState.player.playerPosition.x + bulletOffset - bullet3.sprite.coords.width / 2.0;
                            texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0 - bullet3.sprite.coords.height / 2.0;
                            DrawTextureRec(atlas.textureAtlas, bullet3.sprite.coords, (Vector2){texture_x, texture_y}, WHITE);
                            gameState.shootTime -= 1.0f/gameState.player.fireRate;
                        }
                        else
                        {
                            Bullet bullet =
                            {
                                .position = gameState.player.playerPosition,
                                .velocity = (Vector2){0.0f, 100.0f},
                                .damage = 1.0*gameState.player.damageMulti,
                                .sprite = getSprite(SPRITE_BULLET),
                            };
                            // bullet.position.x -= bullet.sprite.coords.width / 2.0;
                            bullet.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet.sprite.coords.height;
                            gameState.bullets[gameState.bulletCount++] = bullet;
                            gameState.shootTime -= 1.0f/gameState.player.fireRate;
                            // const float texture_x = gameState.player.playerPosition.x - bullet.sprite.coords.width / 2.0;
                            // const float texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0 - bullet.sprite.coords.height / 2.0;

                            // DrawTextureRec(atlas.textureAtlas, bullet.sprite.coords, (Vector2){texture_x, texture_y}, WHITE);
                            DrawTextureRec(atlas.textureAtlas, bullet.sprite.coords, bullet.position, WHITE);
                        }
                    // }
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
                        bullet->position.x -= bullet->velocity.x * GetFrameTime();
                        bullet->position.y -= bullet->velocity.y * GetFrameTime();
                        DrawTextureRec(atlas.textureAtlas, bullet->sprite.coords, bullet->position, WHITE);
                    }
                }
                // Spawn Enemies
                {
                    gameState.spawnTime += GetFrameTime();
                    if (gameState.spawnTime > gameState.enemySpawnRate && gameState.enemyCount < MAX_ENEMIES) 
                    {
                        float size = GetRandomValue(50.0, 200.0) / 100.0f;
                        float minSpawnDistance = 50.0f * size;  
                        float enemyXPosition = MAX(minSpawnDistance, GetRandomValue(0, SCREEN_WIDTH)); 
                        float enemyVelocity = GetRandomValue(30.0f, 65.0f) * 2.0f / (size);
                        Enemy enemy = {
                            .position = (Vector2) {enemyXPosition, 0},
                            .health = (int) (size+1.0) * 2.0,
                            .velocity = enemyVelocity,
                            .size = size,
                            .rotation = 6.0f * (1 - GetRandomValue(1, 2)),
                        };
                        int whichAsteroid = GetRandomValue(1,10);
                        Sprite asteroidSprite;
                        if (whichAsteroid < 3) {
                            asteroidSprite = getSprite(SPRITE_ASTEROID1);
                        } else if (whichAsteroid < 6) {
                            asteroidSprite = getSprite(SPRITE_ASTEROID2);
                        } else {
                            asteroidSprite = getSprite(SPRITE_ASTEROID3);
                            enemy.size = enemy.size / 2.0f;
                        }
                        enemy.sprite = asteroidSprite;
                        enemy.position.y -= enemy.sprite.coords.height; // to make them come into screen smoothly
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
                        float width = enemy->sprite.coords.width * enemy->size;
                        float height = enemy->sprite.coords.height * enemy->size;
                        Rectangle enemyRec = {
                            .width = width,
                            .height = height, 
                            .x = enemy->position.x - width,
                            .y = enemy->position.y - height, 
                        };

                        for (int bulletIndex = 0; bulletIndex < gameState.bulletCount; bulletIndex++)
                        {
                            Bullet* bullet = &gameState.bullets[bulletIndex];
                            Rectangle bulletRec = {
                                .width = 2.0f,
                                .height = 7.0f,
                                .x = gameState.bullets[bulletIndex].position.x,
                                .y = gameState.bullets[bulletIndex].position.y,
                            };
                            // DrawRectangleLines(bulletRec.x, bulletRec.y, bulletRec.width, bulletRec.height, GREEN);
                            if(CheckCollisionRecs(enemyRec, bulletRec))
                            {
                                // Replace with bullet with last bullet
                                *bullet = gameState.bullets[--gameState.bulletCount];
                                if (--enemy->health < 1)
                                {
                                    gameState.experience += MAX((int)(enemy->size * 100),1);
                                    *enemy = gameState.enemies[--gameState.enemyCount];
                                }
                            }
                        }
                        float playerWidth = gameState.player.sprite.coords.width/gameState.player.animationFrames * gameState.player.size;
                        float playerHeight = gameState.player.sprite.coords.height * gameState.player.size;
                        Rectangle playerRec = {
                            .width = playerWidth,
                            .height =  playerHeight,
                            .x = gameState.player.playerPosition.x - playerWidth/2.0f,
                            .y = gameState.player.playerPosition.y - playerHeight/2.0f,
                        };
                        // DrawRectangleLines(enemyRec.x, enemyRec.y, enemyRec.width, enemyRec.height, RED);
                        // DrawRectangleLinesEx(playerRec, 1.0, BLUE);
                        Rectangle screenRectExtended = {
                            .width = SCREEN_WIDTH + enemy->sprite.coords.width,
                            .height = SCREEN_HEIGHT + enemy->sprite.coords.height,
                            .x = 0.0,
                            .y = enemy->position.y, // to make them scroll into screen smoothly
                        };
                        if(!CheckCollisionPointRec(enemy->position, screenRectExtended))
                        {
                            // Replace with last enemy
                            *enemy = gameState.enemies[--gameState.enemyCount];
                        }
                        if(CheckCollisionRecs(enemyRec, playerRec) && gameState.player.invulTime <= 0.0f)
                        {
                            gameState.player.invulTime = gameState.player.invulDuration;
                            *enemy = gameState.enemies[--gameState.enemyCount];
                            if(--gameState.player.playerHealth < 1) 
                            {
                                gameState.state = STATE_GAME_OVER;
                            }
                        }
                        // Check if asteroid is off-screen
                        if (enemy->position.y > SCREEN_HEIGHT + enemy->sprite.coords.height * enemy->size)
                        {
                            *enemy = gameState.enemies[--gameState.enemyCount];
                        }
                    }
                    // Draw enemy in seperate loop to avoid removing drawing wrong enemy sprite in 
                    // position of deleted one for one frame causing flickering
                    for (int enemyIndex = 0; enemyIndex < gameState.enemyCount; enemyIndex++)
                    {
                        Enemy* enemy = &gameState.enemies[enemyIndex];
                        float width = enemy->sprite.coords.width * enemy->size;
                        float height = enemy->sprite.coords.height * enemy->size;
                        Rectangle enemyRec = {
                            .width = width,
                            .height = height, 
                            .x = enemy->position.x - width,
                            .y = enemy->position.y - height, 
                        };
                        float rotation = 0.0f;
                        // DrawTexturePro(atlas.textureAtlas, enemy->sprite.coords, enemyRec, (Vector2){enemy->sprite.coords.width/2.0f, enemy->sprite.coords.height/2.0f}, rotation, WHITE);
                        DrawTexturePro(atlas.textureAtlas, enemy->sprite.coords, enemyRec, (Vector2){0, 0}, rotation, WHITE);
                    }
                }
                // Update Player
                const int texture_x = gameState.player.playerPosition.x - gameState.player.sprite.coords.width * gameState.player.size / gameState.player.animationFrames / 2.0;
                const int texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0;
                // Rectangle destination = {texture_x, texture_y, gameState.player.sprite.coords.width/gameState.player.animationFrames*gameState.player.size, gameState.player.sprite.coords.height*gameState.player.size}; // origin in coordinates and scale
                Rectangle destination = {texture_x, texture_y, 
                                         gameState.player.sprite.coords.width / gameState.player.animationFrames * gameState.player.size, 
                                         gameState.player.sprite.coords.height * gameState.player.size}; // origin in coordinates and scale
                Vector2 origin = {0, 0}; // so it draws from top left of image
                if (gameState.player.invulTime <= 0.0f) {
                    DrawSpriteAnimationPro(atlas.playerAnimation, destination, origin, 0, WHITE);
                } else {
                    if (((int)(gameState.player.invulTime * 10)) % 2 == 0) {
                        DrawSpriteAnimationPro(atlas.playerAnimation, destination, origin, 0, WHITE);
                    }
                }
                // Update player health
                {
                    for (int i = 1; i <= gameState.player.playerHealth; i++)
                    {
                        const int texture_x = i * 16;
                        const int texture_y = getSprite(SPRITE_HEART).coords.height;
                        DrawTextureRec(atlas.textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
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
                draw_text_centered(font, "LEVEL UP!", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f - 80.0}, 40, fontSpacing, WHITE);
                draw_text_centered(font, "CHOOSE UPGRADE", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f - 35.0}, 40, fontSpacing, WHITE);
                const int width = getSprite(SPRITE_MULTISHOT_UPGRADE).coords.width;
                const int height = getSprite(SPRITE_MULTISHOT_UPGRADE).coords.height;                
                const int pos_x = SCREEN_WIDTH/2.0f - width/2.0f;
                const int pos_y = SCREEN_HEIGHT/2.0f - height/2.0f + 30.0;
                const int spacing_x = 80;
                switch (gameState.pickedUpgrade)
                {
                    case UPGRADE_MULTISHOT:
                    {
                        DrawRectangle(pos_x-5, pos_y-5, width+10, height+10, ColorAlpha(GREEN, 0.5));
                        break;
                    }
                    case UPGRADE_DAMAGE:
                    {
                        DrawRectangle(pos_x - spacing_x - 5, pos_y-5, width+10, height+10, ColorAlpha(GREEN, 0.5));
                        break;
                    }
                    case UPGRADE_FIRERATE:
                    {
                        DrawRectangle(pos_x + spacing_x - 5, pos_y-5, width+10, height+10, ColorAlpha(GREEN, 0.5));
                        break;
                    }
                    case UPGRADE_COUNT:
                    {
                        break;
                    }
                }
                DrawTextureRec(atlas.textureAtlas, getSprite(SPRITE_MULTISHOT_UPGRADE).coords, (Vector2){pos_x, pos_y}, WHITE);
                DrawTextureRec(atlas.textureAtlas, getSprite(SPRITE_DAMAGE_UPGRADE).coords, (Vector2){pos_x - spacing_x, pos_y}, WHITE);
                DrawTextureRec(atlas.textureAtlas, getSprite(SPRITE_FIRERATE_UPGRADE).coords, (Vector2){pos_x + spacing_x, pos_y}, WHITE);
                if (IsKeyPressed(KEY_A)) {
                    gameState.pickedUpgrade = (Upgrade)((gameState.pickedUpgrade + 1) % UPGRADE_COUNT);
                } else if (IsKeyPressed(KEY_D)) {
                    gameState.pickedUpgrade = (Upgrade)((gameState.pickedUpgrade - 1 + UPGRADE_COUNT) % UPGRADE_COUNT);
                }
                if (IsKeyPressed(KEY_ENTER)) {                    
                    if (gameState.pickedUpgrade == UPGRADE_MULTISHOT) {
                        gameState.player.playerMultishot = true;
                    } else if (gameState.pickedUpgrade == UPGRADE_DAMAGE) {
                        gameState.player.damageMulti += 0.2f;
                    } else if (gameState.pickedUpgrade == UPGRADE_FIRERATE) {
                        gameState.player.fireRate += 2.0f;
                    }
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
                    initialize(&gameState);
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
