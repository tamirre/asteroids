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
#define SCREEN_WIDTH (1080.0f)
#define SCREEN_HEIGHT (720.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (1000)
#define MAX_ASTEROIDS (100)
#define MAX_STARS (50)
#define TARGET_FPS (240)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define GLSL_VERSION 330

#define ASSERT(x) if (!(x)) { printf("Assertion failed on line %d: %s\n", __LINE__, #x); exit(1); }

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

typedef struct Asteroid {
    Vector2 position;
    int health;
    float size;
    float velocity;
    float rotation;
    char textureFile[100];
    int type;
    Sprite sprite;
	Color* pixels;
} Asteroid;

typedef struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float damage;
    Sprite sprite;
    float rotation;
} Bullet;

typedef struct Player {
    float playerVelocity;
    Vector2 playerPosition;
    int playerHealth;
    Sprite sprite;
    int size;
    int animationFrames;
    float invulTime;
    float invulDuration;
    bool playerMultishot;
    float fireRate;
    float shootTime;
    float damageMulti;
} Player;

typedef struct GameState {
    // General
    int experience;
    State state;
    float gameTime;
    // Player
    Player player;
    // Projectiles
    Bullet bullets[MAX_BULLETS];
    int bulletCount;
    // asteroids
    Asteroid asteroids[MAX_ASTEROIDS];
    int asteroidCount;
    float spawnTime;
    float asteroidSpawnRate;
    // float lastasteroidXPosition;
    // Parallax background stars
    Star stars[MAX_STARS];
    int starCount;
    float starTime;
    float starSpawnRate;
    int initStars;
    Upgrade pickedUpgrade;
} GameState;

void cleanup(TextureAtlas atlas, Font font, SpriteMaskCache spriteMasks) {
    FreeSpriteAnimation(atlas.playerAnimation);
    UnloadFont(font);
	UnloadImageColors(spriteMasks.player.pixels);
	UnloadImageColors(spriteMasks.asteroid1.pixels);
	UnloadImageColors(spriteMasks.asteroid2.pixels);
	UnloadImageColors(spriteMasks.asteroid3.pixels);
	UnloadImageColors(spriteMasks.bullet.pixels);
}

void initialize(GameState* gameState, TextureAtlas* atlas) {
    *gameState = (GameState) {
        .state = STATE_MAIN_MENU,
        .gameTime = 0.0f,
        .bulletCount = 0,
        .asteroidCount = 0,
        .asteroidSpawnRate = 0.2f,
        .spawnTime = 0.0,
        .experience = 999,
        .starCount = 0,
        .starTime = 0,
        .starSpawnRate = 0.25f,
        .initStars = 0,
        .pickedUpgrade = UPGRADE_MULTISHOT,
    };

    gameState->player = (Player) {
        .playerVelocity = 200,
        .playerPosition = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
        .playerHealth = 10,
        .playerMultishot = false,
        .sprite = getSprite(SPRITE_PLAYER),
        .size = 2,
        .animationFrames = 5,
        .invulTime = 0.0f,
        .invulDuration = 3.0f,
        .fireRate = 1.0f,
        .shootTime = 1.0f,
        .damageMulti = 1.0f,
    };
}

void DrawUI(GameState gameState, TextureAtlas atlas, Font font, int fontSize, int fontSpacing)
{
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

	DrawFPS(10, 40);
}




bool pixelPerfectCollision(Color* pixel1, Color* pixel2, int width1, int width2, int height1, int height2, Rectangle dst1, Rectangle dst2, Rectangle overlap)
{
    // Quick rejects
    if (overlap.width <= 0 || overlap.height <= 0) return false;

    const float sx1 = (float)width1  / dst1.width;   // screen->src scale for sprite 1
    const float sy1 = (float)height1 / dst1.height;
    const float sx2 = (float)width2  / dst2.width;   // screen->src scale for sprite 2
    const float sy2 = (float)height2 / dst2.height;

    // Iterate overlap in SCREEN space, sample both images in their LOCAL space
    const int ox = (int)floorf(overlap.x);
    const int oy = (int)floorf(overlap.y);
    const int ow = (int)ceilf(overlap.width);
    const int oh = (int)ceilf(overlap.height);

    for (int y = 0; y < oh; ++y) {
        for (int x = 0; x < ow; ++x) {
            // Current point in screen space
            float sx = (float)(ox + x);
            float sy = (float)(oy + y);

            // Map to local coordinates in each source image
            int u1 = (int)((sx - dst1.x) * sx1);
            int v1 = (int)((sy - dst1.y) * sy1);
            int u2 = (int)((sx - dst2.x) * sx2);
            int v2 = (int)((sy - dst2.y) * sy2);
			// DrawPixel((int)sx, (int)sy, (Color){255,0,255,128});

            // Bounds check
            if (u1 < 0 || v1 < 0 || u2 < 0 || v2 < 0) continue;
            if (u1 >= width1 || v1 >= height1) continue;
            if (u2 >= width2 || v2 >= height2) continue;
			Color a = pixel1[v1 * width1 + u1];
            Color b = pixel2[v2 * width2 + u2];

            if (a.a > 0 && b.a > 0) {
                return true;
            }
        }
    }

    return false;
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

	SpriteMaskCache spriteMasks;
    TextureAtlas atlas = initTextureAtlas(&spriteMasks);
	initialize(&gameState, &atlas);
	InitAudioDevice();
	ASSERT(IsAudioDeviceReady());
	Music music = LoadMusicStream("audio/soundtrack.mp3");
	ASSERT(IsMusicValid(music));
	Sound hitFx = LoadSound("audio/hit.wav");
	ASSERT(IsSoundValid(hitFx));
	// Sound gunFx = LoadSound("audio/gun.mp3");
	Sound laserFx = LoadSound("audio/laser.wav");
	ASSERT(IsSoundValid(laserFx));
	PlayMusicStream(music);
	SetMusicVolume(music, 0.15);
	SetSoundVolume(hitFx, 0.5);
	SetSoundVolume(laserFx, 0.5);
	// fprintf(stderr, "GLSL_VERSION: %d\n", GLSL_VERSION);

	// fprintf(stderr, "GLSL_VERSION: %d\n", GLSL_VERSION);
	RenderTexture2D scene = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
	RenderTexture2D litScene = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

	Shader shader = LoadShader(0, TextFormat("shaders/test.glsl", GLSL_VERSION));
	Shader lightShader = LoadShader(0, TextFormat("shaders/light.fs", GLSL_VERSION));
    int uLightPos = GetShaderLocation(lightShader, "lightPos");
    int uLightRadius = GetShaderLocation(lightShader, "lightRadius");
	int uAspect = GetShaderLocation(lightShader, "aspect");
	// Shader shader = LoadShader(0, TextFormat("shaders/test2.glsl", GLSL_VERSION));
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
    float lightRadius = 0.15f;  // normalized radius
	float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	static bool cursorHidden = true;
    while (!WindowShouldClose())
    {
		// BeginDrawing();
		BeginTextureMode(scene);
        // ClearBackground(BLACK);
        
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
				if (IsKeyPressed(KEY_TAB)) // press Tab to toggle cursor
				{
					if (cursorHidden) EnableCursor();
					else DisableCursor();
					cursorHidden = !cursorHidden;
				}

                Color backgroundColor = ColorFromHSV(258, 1, 0.07);
                ClearBackground(backgroundColor);

				UpdateMusicStream(music);
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
                if (gameState.player.shootTime < 1.0f/gameState.player.fireRate)
                {
                    gameState.player.shootTime += GetFrameTime();
                } 
                // Shoot bullets
                while (IsKeyDown(KEY_SPACE) 
                    && gameState.player.shootTime >= 1.0f/gameState.player.fireRate
                    && gameState.bulletCount <= MAX_BULLETS)
                {

					PlaySound(laserFx);
					BeginShaderMode(shader);
                    if (gameState.player.playerMultishot == true && gameState.bulletCount < MAX_BULLETS-3)
                    {
                        float bulletOffset = 0.0f;
                        Bullet bullet1 = 
                        {
                            .position = gameState.player.playerPosition,
                            .velocity = (Vector2){0.0f, 100.0f},
                            .damage = 1.0*gameState.player.damageMulti,
                            .sprite = getSprite(SPRITE_BULLET),
                            .rotation = 0.0f,
                        };
                        bullet1.position.x -= bullet1.sprite.coords.width / 2.0f;
                        bullet1.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet1.sprite.coords.height;
                        gameState.bullets[gameState.bulletCount++] = bullet1;
                        Rectangle bullet1Rec = {
                            .width = bullet1.sprite.coords.width,
                            .height = bullet1.sprite.coords.height,
                            .x = bullet1.position.x,
                            .y = bullet1.position.y,
                        };
						Vector2 texSize = { bullet1Rec.width, bullet1Rec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
                        DrawTexturePro(atlas.textureAtlas, bullet1.sprite.coords, bullet1Rec, (Vector2){0, 0}, bullet1.rotation, WHITE);
                        Bullet bullet2 = 
                        {
                            .position = (Vector2){gameState.player.playerPosition.x - bulletOffset, gameState.player.playerPosition.y + 0.0f},
                            .velocity = (Vector2){sqrt(pow(100.0f,2) - pow(95.0f,2)), 95.0f},
                            .damage = 1.0*gameState.player.damageMulti,
                            .sprite = getSprite(SPRITE_BULLET),
                            .rotation = -15.0f,
                        };
                        bullet2.position.x -= bullet2.sprite.coords.width / 2.0f;
                        bullet2.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet2.sprite.coords.height;
                        gameState.bullets[gameState.bulletCount++] = bullet2;
                        Rectangle bullet2Rec = {
                            .width = bullet2.sprite.coords.width,
                            .height = bullet2.sprite.coords.height,
                            .x = bullet2.position.x,
                            .y = bullet2.position.y,
                        };
						texSize = (Vector2){ bullet2Rec.width, bullet2Rec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// SetShaderValue(lightShader, uLightPos, &bullet2.position, SHADER_UNIFORM_VEC2);
                        DrawTexturePro(atlas.textureAtlas, bullet2.sprite.coords, bullet2Rec, (Vector2){0, 0}, bullet2.rotation, WHITE);
                        Bullet bullet3 = 
                        {
                            .position = (Vector2){gameState.player.playerPosition.x + bulletOffset, gameState.player.playerPosition.y + 0.0f},
                            .velocity = (Vector2){-sqrt(pow(100.0f,2) - pow(95.0f,2)), 95.0f},
                            .damage = 1.0*gameState.player.damageMulti,
                            .sprite = getSprite(SPRITE_BULLET),
                            .rotation = 15.0f,
                        };
                        bullet3.position.x -= bullet3.sprite.coords.width / 2.0f;
                        bullet3.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet3.sprite.coords.height;
                        gameState.bullets[gameState.bulletCount++] = bullet3;
                        Rectangle bullet3Rec = {
                            .width = bullet3.sprite.coords.width,
                            .height = bullet3.sprite.coords.height,
                            .x = bullet3.position.x,
                            .y = bullet3.position.y,
                        };
						texSize = (Vector2){ bullet3Rec.width, bullet3Rec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// SetShaderValue(lightShader, uLightPos, &bullet3.position, SHADER_UNIFORM_VEC2);
                        DrawTexturePro(atlas.textureAtlas, bullet3.sprite.coords, bullet3Rec, (Vector2){0, 0}, bullet3.rotation, WHITE);
                        gameState.player.shootTime -= 1.0f/gameState.player.fireRate;
                    }
                    else
                    {
                        Bullet bullet =
                        {
                            .position = gameState.player.playerPosition,
                            .velocity = (Vector2){0.0f, 100.0f},
                            .damage = 1.0*gameState.player.damageMulti,
                            .sprite = getSprite(SPRITE_BULLET),
                            .rotation = 0.0f,
                        };
                        bullet.position.y -= gameState.player.sprite.coords.height * gameState.player.size / 2.0 + bullet.sprite.coords.height;
                        gameState.bullets[gameState.bulletCount++] = bullet;
                        gameState.player.shootTime -= 1.0f/gameState.player.fireRate;
                        Rectangle bulletRec = {
                            .width = bullet.sprite.coords.width,
                            .height = bullet.sprite.coords.height,
                            .x = bullet.position.x,
                            .y = bullet.position.y,
                        };
						Vector2 texSize = { bulletRec.width, bulletRec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// SetShaderValue(lightShader, uLightPos, &bullet.position, SHADER_UNIFORM_VEC2);
                        DrawTexturePro(atlas.textureAtlas, bullet.sprite.coords, bulletRec, (Vector2){0, 0}, bullet.rotation, WHITE);
                    }
					EndShaderMode();
                }
                // Update Bullets
                {
					BeginShaderMode(shader);
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
                        Rectangle bulletRec = {
                            .width = bullet->sprite.coords.width,
                            .height = bullet->sprite.coords.height,
                            .x = bullet->position.x,
                            .y = bullet->position.y,
                        };
						Vector2 texSize = { bulletRec.width, bulletRec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// SetShaderValue(lightShader, uLightPos, &bullet->position, SHADER_UNIFORM_VEC2);
                        DrawTexturePro(atlas.textureAtlas, bullet->sprite.coords, bulletRec, (Vector2){0, 0}, bullet->rotation, WHITE);
                        // DrawTextureRec(atlas.textureAtlas, bullet->sprite.coords, bullet->position, WHITE);
                    }
					EndShaderMode();
                }
                // Spawn Asteroids
                {
                    gameState.spawnTime += GetFrameTime();
                    if (gameState.spawnTime > gameState.asteroidSpawnRate && gameState.asteroidCount < MAX_ASTEROIDS) 
                    {
                        // int size = (int)GetRandomValue(1, 3);
                        float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
                        // float size = 1.0f;
                        float minSpawnDistance = 50.0f * size;  
                        float asteroidXPosition = MAX(minSpawnDistance, GetRandomValue(0, SCREEN_WIDTH)); 
                        float asteroidVelocity = GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size;
                        Asteroid asteroid = {
                            .position = (Vector2) {asteroidXPosition, 0},
                            .health = (int) (size+1.0) * 2.0,
                            .velocity = asteroidVelocity,
                            .size = size,
                            .rotation = 6.0f * (1 - GetRandomValue(1, 2)),
                        };
                        int whichAsteroid = GetRandomValue(1,10);
                        Sprite asteroidSprite;
                        if (whichAsteroid < 6) {
                            asteroidSprite = getSprite(SPRITE_ASTEROID1);
							asteroid.pixels = spriteMasks.asteroid1.pixels;
                        } else if (whichAsteroid < 9) {
                            asteroidSprite = getSprite(SPRITE_ASTEROID2);
							asteroid.pixels = spriteMasks.asteroid2.pixels;
                        } else {
                            asteroidSprite = getSprite(SPRITE_ASTEROID3);
							asteroid.pixels = spriteMasks.asteroid3.pixels;
                        }
                        asteroid.sprite = asteroidSprite;
                        asteroid.position.y -= asteroid.sprite.coords.height; // to make them come into screen smoothly
                        gameState.spawnTime = 0;
                        gameState.asteroids[gameState.asteroidCount++] = asteroid;
                    }
                }
                // Update asteroids
                {
					BeginShaderMode(shader);
                    for (int asteroidIndex = 0; asteroidIndex < gameState.asteroidCount; asteroidIndex++)
                    {
                        Asteroid* asteroid = &gameState.asteroids[asteroidIndex];
                        asteroid->position.y += asteroid->velocity * GetFrameTime();
                        float width = asteroid->sprite.coords.width * asteroid->size;
                        float height = asteroid->sprite.coords.height * asteroid->size;
                        Rectangle asteroidRec = {
                            .width = width,
                            .height = height, 
                            .x = asteroid->position.x - width,
                            .y = asteroid->position.y - height, 
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
							if(CheckCollisionRecs(asteroidRec, bulletRec))
							{
								Rectangle collisionRec = GetCollisionRec(asteroidRec, bulletRec);
								if (pixelPerfectCollision(spriteMasks.bullet.pixels, asteroid->pixels, 
											bullet->sprite.coords.width, asteroid->sprite.coords.width,
											bullet->sprite.coords.height, asteroid->sprite.coords.height,
											bulletRec, asteroidRec, collisionRec))
								{
									// Replace with bullet with last bullet
									*bullet = gameState.bullets[--gameState.bulletCount];
									if (--asteroid->health < 1)
									{
										gameState.experience += MAX((int)(asteroid->size * 100),1);
										*asteroid = gameState.asteroids[--gameState.asteroidCount];
									}
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
                        // DrawRectangleLines(asteroidRec.x, asteroidRec.y, asteroidRec.width, asteroidRec.height, GREEN);
                        // DrawRectangleLinesEx(playerRec, 1.0, BLUE);
                        Rectangle screenRectExtended = {
                            .width = SCREEN_WIDTH + asteroid->sprite.coords.width,
                            .height = SCREEN_HEIGHT + asteroid->sprite.coords.height,
                            .x = 0.0,
                            .y = asteroid->position.y, // to make them scroll into screen smoothly
                        };
                        if(!CheckCollisionPointRec(asteroid->position, screenRectExtended))
                        {
                            // Replace with last asteroid
                            *asteroid = gameState.asteroids[--gameState.asteroidCount];
                        }
                        if(CheckCollisionRecs(asteroidRec, playerRec) && gameState.player.invulTime <= 0.0f)
                        {
                            Rectangle collisionRec = GetCollisionRec(asteroidRec, playerRec);
                            // DrawRectangleLinesEx(collisionRec, 2.0, RED);
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas.playerAnimation); 
							if (pixelPerfectCollision(spriteMasks.player.pixels, asteroid->pixels, 
										              playerSrc.width, asteroid->sprite.coords.width,
													  gameState.player.sprite.coords.height, asteroid->sprite.coords.height, 
													  playerRec, asteroidRec, collisionRec))
                            {
								PlaySound(hitFx);
                                gameState.player.invulTime = gameState.player.invulDuration;
                                *asteroid = gameState.asteroids[--gameState.asteroidCount];
                                if(--gameState.player.playerHealth < 1) 
                                {
                                    gameState.state = STATE_GAME_OVER;
                                }
                            }
                        }
                        // Check if asteroid is off-screen
                        if (asteroid->position.y > SCREEN_HEIGHT + asteroid->sprite.coords.height * asteroid->size)
                        {
                            *asteroid = gameState.asteroids[--gameState.asteroidCount];
                        }
                    }
                    // Draw asteroid in seperate loop to avoid removing drawing wrong asteroid sprite in 
                    // position of deleted one for one frame causing flickering
                    for (int asteroidIndex = 0; asteroidIndex < gameState.asteroidCount; asteroidIndex++)
                    {
                        Asteroid* asteroid = &gameState.asteroids[asteroidIndex];
                        float width = asteroid->sprite.coords.width * asteroid->size;
                        float height = asteroid->sprite.coords.height * asteroid->size;
                        Rectangle asteroidRec = {
                            .width = width,
                            .height = height, 
                            .x = asteroid->position.x - width,
                            .y = asteroid->position.y - height, 
                        };
                        float rotation = 0.0f;

						Vector2 texSize = { width, height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
                        DrawTexturePro(atlas.textureAtlas, asteroid->sprite.coords, asteroidRec, (Vector2){0, 0}, rotation, WHITE);
                    }
                }
                // Update Player
                const int texture_x = gameState.player.playerPosition.x - gameState.player.sprite.coords.width * gameState.player.size / gameState.player.animationFrames / 2.0;
                const int texture_y = gameState.player.playerPosition.y - gameState.player.sprite.coords.height * gameState.player.size / 2.0;
                Rectangle destination = {texture_x, texture_y, 
                                         gameState.player.sprite.coords.width / gameState.player.animationFrames * gameState.player.size, 
                                         gameState.player.sprite.coords.height * gameState.player.size}; // origin in coordinates and scale
                Vector2 origin = {0, 0}; // so it draws from top left of image
                if (gameState.player.invulTime <= 0.0f) {
                    DrawSpriteAnimationPro(atlas.playerAnimation, destination, origin, 0, WHITE, shader);
                } else {
                    if (((int)(gameState.player.invulTime * 10)) % 2 == 0) {
                        DrawSpriteAnimationPro(atlas.playerAnimation, destination, origin, 0, WHITE, shader);
                    }
                }
				EndShaderMode();

                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_PAUSED;
                }
                
                break;
            }
            case STATE_UPGRADE:
            {
				UpdateMusicStream(music);
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
                if (IsKeyPressed(KEY_LEFT)) {
                    gameState.pickedUpgrade = (Upgrade)((gameState.pickedUpgrade + 1) % UPGRADE_COUNT);
                } else if (IsKeyPressed(KEY_RIGHT)) {
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
                    initialize(&gameState, &atlas);
                    gameState.state = STATE_RUNNING;

                }
                break;
            }
            case STATE_PAUSED:
            {
				PauseMusicStream(music);
                draw_text_centered(font, "Game is paused...", (Vector2){SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}, 40, fontSpacing, WHITE);
                if (IsKeyPressed(KEY_P)) {
                    gameState.state = STATE_RUNNING;
					ResumeMusicStream(music);
                }
                break;
            }
        }
		
		EndTextureMode(); // scene

		{
			// BeginTextureMode(litScene);
			// ClearBackground(BLACK);
			// // BeginBlendMode(BLEND_ADDITIVE);
			// {
			// 	BeginShaderMode(lightShader);
			//
			//
			// 	DrawCircleV(gameState.player.playerPosition, 1, WHITE);
			// 	for (int bulletIndex = 0; bulletIndex < gameState.bulletCount; bulletIndex++)
			// 	{
			// 		Bullet* bullet = &gameState.bullets[bulletIndex];
			// 		Vector2 lightPosNorm = {
			// 			bullet->position.x / SCREEN_WIDTH,
			// 			1.0f - bullet->position.y / SCREEN_HEIGHT
			// 		};
			// 		SetShaderValue(lightShader, uLightPos, &lightPosNorm, SHADER_UNIFORM_VEC2);
			// 		// draw a white circle or sprite — not the scene!
			// 		DrawCircleV(bullet->position, 1, WHITE);
			// 	}
			// 	EndShaderMode();
			// 	// EndBlendMode();
			// }
			// EndTextureMode();
			// --- Build lightmap ---
			BeginTextureMode(litScene);
			ClearBackground(BLACK);
			BeginBlendMode(BLEND_ADDITIVE);

			for (int i = 0; i < gameState.bulletCount; i++)
			{
				Bullet* bullet = &gameState.bullets[i];
				// 	Vector2 lightPosNorm = {
				// 		bullet->position.x / SCREEN_WIDTH,
				// 		1.0f - bullet->position.y / SCREEN_HEIGHT
				// 	};
				// 	SetShaderValue(lightShader, uLightPos, &lightPosNorm, SHADER_UNIFORM_VEC2);
				// draw a light shape: small white circle or texture
				DrawCircleV(bullet->position, 40, WHITE);    // 50 = light radius
			}

			EndBlendMode();
			EndTextureMode();
		}

		SetShaderValueTexture(lightShader, GetShaderLocation(lightShader, "lightTexture"), litScene.texture);
		SetShaderValue(lightShader, uLightRadius, &lightRadius, SHADER_UNIFORM_FLOAT);
		SetShaderValue(lightShader, uAspect, &aspect, SHADER_UNIFORM_FLOAT);

		BeginDrawing();
		// ClearBackground(BLACK);

		BeginShaderMode(lightShader);
		DrawTextureRec(scene.texture, (Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height}, (Vector2){0,0}, WHITE);
		EndShaderMode();

		DrawUI(gameState, atlas, font, fontSize, fontSpacing);
		EndDrawing();

		// BeginDrawing();
		// DrawTextureRec(scene.texture, (Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height}, (Vector2){0,0}, WHITE);
		// EndDrawing();
    }

	UnloadMusicStream(music);
	UnloadSound(hitFx);
	UnloadSound(laserFx);
	CloseAudioDevice();
	UnloadShader(shader);
	UnloadShader(lightShader);
	cleanup(atlas, font, spriteMasks);

    CloseWindow();
    return 0;
}
