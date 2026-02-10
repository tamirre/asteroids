// To build on linux: 
// gcc asteroids.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
#include "assetsData.h"
#include "txt.h"
#define RAYMATH_IMPLEMENTATION
#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include "assetsUtils.h"
#include "localization.h"

#define RAYGUI_IMPLEMENTATION
#include "third_party/raygui/src/raygui.h"
#include "third_party/raygui/styles/dark/style_dark.h"


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MIN_SCREEN_WIDTH (1280.0f)
#define MIN_SCREEN_HEIGHT (720.0f)
#define VIRTUAL_WIDTH (1440.0f)
#define VIRTUAL_HEIGHT (810.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (1000)
#define MAX_ASTEROIDS (100)
#define MAX_STARS (50)
#ifdef PLATFORM_WEB
	#define TARGET_FPS (60)
#else
	#define TARGET_FPS (300)
#endif

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if defined(PLATFORM_WEB)
	#define GLSL_VERSION 300
#else
	#define GLSL_VERSION 330
#endif

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
    float angularVelocity;
    float rotation;
    char textureFile[100];
    int type;
    Sprite sprite;
	Color* pixels;
	Rectangle collider;
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

typedef struct Audio {
    Sound hitFx;
    Sound laserFx;
	Music music;
} Audio;

typedef struct Options {
	float screenWidth;
	float screenHeight;
	float previousWidth;
	float previousHeight;
	bool disableShaders;
	Font font;
	float fontSpacing;
	int maxFontSize;
	int language;
	int lastLanguage;
} Options;

typedef struct GameState {
    // General
    int experience;
    State state;
	float timeScale;
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

static GameState gameState;
static Options options;
static Audio audio; 
static SpriteMask spriteMasks[SPRITE_COUNT];
static TextureAtlas atlas;
static RenderTexture2D scene;
static RenderTexture2D litScene;
static Shader shader;
static Shader lightShader;
static bool editMode = false;

void cleanup(TextureAtlas atlas, Options options, Audio audio, SpriteMask spriteMasks[]) {
    FreeSpriteAnimation(atlas.playerAnimation);
    UnloadFont(options.font);
	for (int i = 0; i < SPRITE_COUNT; i++)
	{
		UnloadImageColors(spriteMasks[i].pixels);
	}
	UnloadMusicStream(audio.music);
	UnloadSound(audio.hitFx);
	UnloadSound(audio.laserFx);
	CloseAudioDevice();
}

void initializeAudio(Audio* audio) {
	InitAudioDevice();
	ASSERT(IsAudioDeviceReady());
	audio->music = LoadMusicStream("audio/soundtrack.mp3");
	ASSERT(IsMusicValid(audio->music));
	audio->hitFx = LoadSound("audio/hit.wav");
	ASSERT(IsSoundValid(audio->hitFx));
	audio->laserFx = LoadSound("audio/laser.wav");
	ASSERT(IsSoundValid(audio->laserFx));
	PlayMusicStream(audio->music);
	SetMusicVolume(audio->music, 0.05);
	SetSoundVolume(audio->hitFx, 0.25);
	SetSoundVolume(audio->laserFx, 0.25);
}

void initializeGameState(GameState* gameState) {
    *gameState = (GameState) {
        .state = STATE_MAIN_MENU,
		.timeScale = 1.0f,
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
        .playerPosition = (Vector2){VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f},
        .playerHealth = 7,
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

void initializeOptions(Options* options) {

	int maxFontSize = 64;
	*options = (Options) {
		.screenWidth = (float)VIRTUAL_WIDTH,
		.screenHeight = (float)VIRTUAL_HEIGHT,
		.disableShaders = true,
		.font = LoadLanguageFont("fonts/UnifontExMono.ttf", maxFontSize, LANG_EN), 
		.fontSpacing = 1.0f,
		.maxFontSize = maxFontSize,
		.language = LANG_EN,
		.lastLanguage = LANG_EN,
	};
	GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
	GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
	GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);

	GuiLoadStyleDark();
	// GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x181818FF);
	// GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x303030FF);
	// GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xE0E0E0FF);
	// GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x505050FF);

	GuiSetFont(options->font);
}


void draw_text_centered(Font font, const char* text, Vector2 pos, int fontSize, int fontSpacing, Color color)
{
	const Vector2 textSize = MeasureTextEx(font, text, fontSize, fontSpacing);
    pos.x -= textSize.x / 2.0f;
    pos.y -= textSize.y / 2.0f;
	DrawTextEx(font, text, (Vector2){pos.x, pos.y}, fontSize, fontSpacing, color);
}

bool pixelPerfectCollision(
    Color* pixel1,
    Color* pixel2,
    int width1, int width2,
    int height1, int height2,
    Rectangle dst1, Rectangle dst2,
    Rectangle overlap,
    float rotationDeg1,
    float rotationDeg2
)
{
    if (overlap.width <= 0 || overlap.height <= 0)
        return false;

    // Screen -> texture scale
    const float sx1 = (float)width1  / dst1.width;
    const float sy1 = (float)height1 / dst1.height;
    const float sx2 = (float)width2  / dst2.width;
    const float sy2 = (float)height2 / dst2.height;

    // Centers (screen space)
    Vector2 center1 = {
        dst1.x + dst1.width  * 0.5f,
        dst1.y + dst1.height * 0.5f
    };
    Vector2 center2 = {
        dst2.x + dst2.width  * 0.5f,
        dst2.y + dst2.height * 0.5f
    };

    // Inverse rotations
    float r1 = -rotationDeg1 * DEG2RAD;
    float r2 = -rotationDeg2 * DEG2RAD;

    float cos1 = cosf(r1), sin1 = sinf(r1);
    float cos2 = cosf(r2), sin2 = sinf(r2);

    int ox = (int)floorf(overlap.x);
    int oy = (int)floorf(overlap.y);
    int ow = (int)ceilf(overlap.width);
    int oh = (int)ceilf(overlap.height);

    for (int y = 0; y < oh; y++) {
        for (int x = 0; x < ow; x++) {
            float sx = ox + x;
            float sy = oy + y;

            /* ---------- sprite 1 ---------- */
            float rx1 = sx - center1.x;
            float ry1 = sy - center1.y;

            float lx1 = cos1 * rx1 - sin1 * ry1;
            float ly1 = sin1 * rx1 + cos1 * ry1;

            lx1 += dst1.width  * 0.5f;
            ly1 += dst1.height * 0.5f;

            int u1 = (int)(lx1 * sx1);
            int v1 = (int)(ly1 * sy1);

            /* ---------- sprite 2 ---------- */
            float rx2 = sx - center2.x;
            float ry2 = sy - center2.y;

            float lx2 = cos2 * rx2 - sin2 * ry2;
            float ly2 = sin2 * rx2 + cos2 * ry2;

            lx2 += dst2.width  * 0.5f;
            ly2 += dst2.height * 0.5f;

            int u2 = (int)(lx2 * sx2);
            int v2 = (int)(ly2 * sy2);

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

void UpdateGame(GameState* gameState, Options* options, TextureAtlas* atlas, SpriteMask spriteMasks[], Audio* audio, float dt)
{
	static bool cursorHidden = true;
	static bool stepMode = false;
	static bool stepOnce = false;

	switch (gameState->state) 
	{
		case STATE_MAIN_MENU:
			{
				if (IsKeyPressed(KEY_ENTER)) {                    
					gameState->state = STATE_RUNNING;
				}
				break;
			}
		case STATE_RUNNING:
			{
				// Step debugging mode
				if (IsKeyPressed(KEY_J)) stepMode = !stepMode;
				if (IsKeyPressed(KEY_K)) stepOnce = true;
				if (IsKeyPressed(KEY_O)) options->disableShaders = !options->disableShaders;
#ifndef PLATFORM_WEB
				if (IsKeyPressed(KEY_F)) 
				{
					ToggleFullscreen();
				}
#endif
				if (IsKeyPressed(KEY_V)) {
					if (IsWindowState(FLAG_VSYNC_HINT))
					{
						ClearWindowState(FLAG_VSYNC_HINT);
					} else {
						SetWindowState(FLAG_VSYNC_HINT);
					}
				}

				if (stepMode && !stepOnce) return;
				stepOnce = false;

				const Rectangle screenRect = {
					.height = options->screenHeight,
					.width = options->screenWidth,
					.x = 0,
					.y = 0
				};
				// Update score
				if (gameState->experience > 1000.0)
				{
					gameState->state = STATE_UPGRADE;
					gameState->experience -= 1000.0;
				}
				if (IsKeyPressed(KEY_H))
				{
					gameState->timeScale -= 0.1f;
				}
				if (IsKeyPressed(KEY_L)) 
				{
					gameState->timeScale += 0.1f;
				}
				if (IsKeyPressed(KEY_TAB)) // press Tab to toggle cursor
				{
					if (cursorHidden) EnableCursor();
					else DisableCursor();
					cursorHidden = !cursorHidden;
				}

				UpdateMusicStream(audio->music);
				// Spawn initial stars for parallax
				{
					if (gameState->initStars == 0)
					{
						while(gameState->starCount < MAX_STARS)
						{                            
							int imgIndex = GetRandomValue(1, 2);   
							float starXPosition = GetRandomValue(0, options->screenWidth); 
							float starYPosition = GetRandomValue(0, options->screenHeight); 
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
							gameState->stars[gameState->starCount++] = star;
						}
						gameState->initStars = 1;
					}
				}
				// Spawn new stars for parallax
				{
					gameState->starTime += dt;
					if (gameState->starTime > gameState->starSpawnRate) 
					{
						gameState->starTime = 0;
						if (gameState->starCount < MAX_STARS)
						{
							int imgIndex = GetRandomValue(1, 2);  
							Sprite starSprite = getSprite(SPRITE_STAR1);
							if (imgIndex == 2)
							{
								starSprite = getSprite(SPRITE_STAR2);
							}
							float starXPosition = GetRandomValue(0, options->screenWidth); 
							Star star = {
								.position = (Vector2) {starXPosition, 0},
								.velocity = 30.0f * GetRandomValue(1,2) * imgIndex,
								.size = 1,
								.alpha = 0.5f * imgIndex,
								.sprite = starSprite,
							};
							gameState->stars[gameState->starCount++]= star;
						}
					}
				}
                {
                    for (int starIndex = 0; starIndex < gameState->starCount; starIndex++)
                    {
                        Star* star = &gameState->stars[starIndex];
                        if(!CheckCollisionPointRec(star->position, screenRect))
						{
							// Replace with last star
							*star = gameState->stars[--gameState->starCount];
						}
                        star->position.y += star->velocity * dt;
                    }
                }
				if (gameState->player.invulTime < 0.0f)
				{
					gameState->player.invulTime = 0.0f;
				} else {
					gameState->player.invulTime -= dt;
				}

				if (IsKeyDown(KEY_W)) {
					if(!CheckCollisionPointLine(gameState->player.playerPosition, (Vector2) {0, 0}, (Vector2) {options->screenWidth, 0}, gameState->player.sprite.coords.width/gameState->player.animationFrames / 2))
					{
						gameState->player.playerPosition.y -= gameState->player.playerVelocity * dt;
					}   
				}
				if (IsKeyDown(KEY_S)) {
					if(!CheckCollisionPointLine(gameState->player.playerPosition, (Vector2) {0, options->screenHeight}, (Vector2) {options->screenWidth, options->screenHeight}, gameState->player.sprite.coords.height / 2))
					{
						gameState->player.playerPosition.y += gameState->player.playerVelocity * dt;
					}
				}
				if (IsKeyDown(KEY_A)) {
					if(!CheckCollisionPointLine(gameState->player.playerPosition, (Vector2) {0, 0}, (Vector2) {0, options->screenHeight}, gameState->player.sprite.coords.width/gameState->player.animationFrames / 2))
					{
						gameState->player.playerPosition.x -= gameState->player.playerVelocity * dt;
					}
				}
				if (IsKeyDown(KEY_D)) {
					if(!CheckCollisionPointLine(gameState->player.playerPosition, 
								(Vector2) {options->screenWidth, 0},
								(Vector2) {options->screenWidth, options->screenHeight}, 
								gameState->player.sprite.coords.width/gameState->player.animationFrames / 2))
					{
						gameState->player.playerPosition.x += gameState->player.playerVelocity * dt;
					}
				}
				if (gameState->player.shootTime < 1.0f/gameState->player.fireRate)
				{
					gameState->player.shootTime += dt;
				} 
				// Shoot bullets
				while (IsKeyDown(KEY_SPACE) 
						&& gameState->player.shootTime >= 1.0f/gameState->player.fireRate
						&& gameState->bulletCount <= MAX_BULLETS)
				{

					PlaySound(audio->laserFx);
					if (gameState->player.playerMultishot == true && gameState->bulletCount < MAX_BULLETS-3)
					{
						float bulletOffset = 0.0f;
						Bullet bullet1 = 
						{
							.position = gameState->player.playerPosition,
							.velocity = (Vector2){0.0f, 500.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = 0.0f,
						};
						bullet1.position.x -= bullet1.sprite.coords.width / 2.0f;
						bullet1.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 + bullet1.sprite.coords.height;
						gameState->bullets[gameState->bulletCount++] = bullet1;
						Bullet bullet2 = 
						{
							.position = (Vector2){gameState->player.playerPosition.x - bulletOffset, gameState->player.playerPosition.y + 0.0f},
							.velocity = (Vector2){sqrt(pow(500.0f,2) - pow(450.0f,2)), 450.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = -15.0f,
						};
						bullet2.position.x -= bullet2.sprite.coords.width / 2.0f;
						bullet2.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 + bullet2.sprite.coords.height;
						gameState->bullets[gameState->bulletCount++] = bullet2;
						Bullet bullet3 = 
						{
							.position = (Vector2){gameState->player.playerPosition.x + bulletOffset, gameState->player.playerPosition.y + 0.0f},
							.velocity = (Vector2){-sqrt(pow(500.0f,2) - pow(450.0f,2)), 450.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = 15.0f,
						};
						bullet3.position.x -= bullet3.sprite.coords.width / 2.0f;
						bullet3.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 + bullet3.sprite.coords.height;
						gameState->bullets[gameState->bulletCount++] = bullet3;

						gameState->player.shootTime -= 1.0f/gameState->player.fireRate;
					}
					else
					{
						Bullet bullet =
						{
							.position = gameState->player.playerPosition,
							.velocity = (Vector2){0.0f, 500.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = 0.0f,
						};
						bullet.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 + bullet.sprite.coords.height;
						gameState->bullets[gameState->bulletCount++] = bullet;
						gameState->player.shootTime -= 1.0f/gameState->player.fireRate;
					}
				}
				// Update Bullets
				{
                    for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
                    {
                        Bullet* bullet = &gameState->bullets[bulletIndex];
                        if(!CheckCollisionPointRec(bullet->position, screenRect))
						{
							// Replace with last projectile
							*bullet = gameState->bullets[--gameState->bulletCount];
						}
                        bullet->position.x -= bullet->velocity.x * dt;
                        bullet->position.y -= bullet->velocity.y * dt;
					}
				}
				// Spawn Asteroids
				{
					gameState->spawnTime += dt;
					if (gameState->spawnTime > gameState->asteroidSpawnRate && gameState->asteroidCount < MAX_ASTEROIDS) 
					{
						// int size = (int)GetRandomValue(1, 3);
						float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
						// float size = 1.0f;
						float minSpawnDistance = 50.0f * size;  
						float asteroidXPosition = MAX(minSpawnDistance, GetRandomValue(0, options->screenWidth)); 
						Asteroid asteroid = {
							.position = (Vector2) {asteroidXPosition, 0},
							.health = (int) (size+1.0) * 2.0,
							.velocity = GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size,
							.angularVelocity = GetRandomValue(-40.0f, 40.0f),
							.size = size,
							.rotation = GetRandomValue(0.0f, 360.0f),
						};
						int whichAsteroid = GetRandomValue(1,10);
						Sprite asteroidSprite;
						if (whichAsteroid < 6) {
							asteroidSprite = getSprite(SPRITE_ASTEROID1);
							asteroid.pixels = spriteMasks[SPRITE_ASTEROID1].pixels;
						} else if (whichAsteroid < 9) {
							asteroidSprite = getSprite(SPRITE_ASTEROID2);
							asteroid.pixels = spriteMasks[SPRITE_ASTEROID2].pixels;
						} else {
							asteroidSprite = getSprite(SPRITE_ASTEROID3);
							asteroid.pixels = spriteMasks[SPRITE_ASTEROID3].pixels;
						}
						asteroid.sprite = asteroidSprite;
						asteroid.position.y -= asteroid.sprite.coords.height; // to make them come into screen smoothly
						gameState->spawnTime = 0;
						gameState->asteroids[gameState->asteroidCount++] = asteroid;
					}
				}
				// Update asteroids
				{
					for (int asteroidIndex = 0; asteroidIndex < gameState->asteroidCount; asteroidIndex++)
					{
						Asteroid* asteroid = &gameState->asteroids[asteroidIndex];
						asteroid->position.y += asteroid->velocity * dt;
						asteroid->rotation   += asteroid->angularVelocity * dt;
						float width  = asteroid->sprite.coords.width * asteroid->size;
						float height = asteroid->sprite.coords.height * asteroid->size;
						asteroid->collider = (Rectangle) {
							.x = asteroid->position.x - width / 2.0f,
							.y = asteroid->position.y - height / 2.0f, 
							.width  = width,
							.height = height, 
						};

						for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
						{
							Bullet* bullet = &gameState->bullets[bulletIndex];
							Rectangle bulletRec = {
								.width = 2.0f,
								.height = 7.0f,
								.x = gameState->bullets[bulletIndex].position.x,
								.y = gameState->bullets[bulletIndex].position.y,
							};
							// DrawRectangleLines(bulletRec.x, bulletRec.y, bulletRec.width, bulletRec.height, GREEN);
							if(CheckCollisionRecs(asteroid->collider, bulletRec))
							{
								Rectangle collisionRec = GetCollisionRec(asteroid->collider, bulletRec);
								if (pixelPerfectCollision(spriteMasks[SPRITE_BULLET].pixels, asteroid->pixels, 
											bullet->sprite.coords.width, asteroid->sprite.coords.width,
											bullet->sprite.coords.height, asteroid->sprite.coords.height,
											bulletRec, asteroid->collider, collisionRec, bullet->rotation, asteroid->rotation))
								{
									// Replace with bullet with last bullet
									*bullet = gameState->bullets[--gameState->bulletCount];
									if (--asteroid->health < 1)
									{
										gameState->experience += MAX((int)(asteroid->size * 100),1);
										*asteroid = gameState->asteroids[--gameState->asteroidCount];
									}
								}
							}
						}
						float playerWidth = gameState->player.sprite.coords.width/gameState->player.animationFrames * gameState->player.size;
						float playerHeight = gameState->player.sprite.coords.height * gameState->player.size;
						Rectangle playerRec = {
							.width = playerWidth,
							.height =  playerHeight,
							.x = gameState->player.playerPosition.x - playerWidth/2.0f,
							.y = gameState->player.playerPosition.y - playerHeight/2.0f,
						};
						// DrawRectangleLinesEx(playerRec, 1.0, BLUE);
						Rectangle screenRectExtended = {
							.width = options->screenWidth + asteroid->sprite.coords.width,
							.height = options->screenHeight + asteroid->sprite.coords.height,
							.x = 0.0,
							.y = asteroid->position.y, // to make them scroll into screen smoothly
						};
						if(!CheckCollisionPointRec(asteroid->position, screenRectExtended))
						{
							// Replace with last asteroid
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
						if(CheckCollisionRecs(asteroid->collider, playerRec) && gameState->player.invulTime <= 0.0f)
						{
							Rectangle collisionRec = GetCollisionRec(asteroid->collider, playerRec);
							// DrawRectangleLinesEx(collisionRec, 2.0, RED);
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas->playerAnimation); 
							if (pixelPerfectCollision(spriteMasks[SPRITE_PLAYER].pixels, asteroid->pixels, 
										playerSrc.width, asteroid->sprite.coords.width,
										gameState->player.sprite.coords.height, asteroid->sprite.coords.height, 
										playerRec, asteroid->collider, collisionRec, 0.0f, asteroid->rotation))
							{
								PlaySound(audio->hitFx);
								gameState->player.invulTime = gameState->player.invulDuration;
								*asteroid = gameState->asteroids[--gameState->asteroidCount];
								if(--gameState->player.playerHealth < 1) 
								{
									gameState->state = STATE_GAME_OVER;
								}
							}
						}
						// Check if asteroid is off-screen
						if (asteroid->position.y > options->screenHeight + asteroid->sprite.coords.height * asteroid->size)
						{
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
					}
				}

				if (IsKeyPressed(KEY_ESCAPE)) {
					gameState->state = STATE_PAUSED;
				}
				break;
			}
		case STATE_UPGRADE:
			{
				UpdateMusicStream(audio->music);
				if (IsKeyPressed(KEY_LEFT)) {
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade + 1) % UPGRADE_COUNT);
				} else if (IsKeyPressed(KEY_RIGHT)) {
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade - 1 + UPGRADE_COUNT) % UPGRADE_COUNT);
				}
				if (IsKeyPressed(KEY_ENTER)) {                    
					if (gameState->pickedUpgrade == UPGRADE_MULTISHOT) {
						gameState->player.playerMultishot = true;
					} else if (gameState->pickedUpgrade == UPGRADE_DAMAGE) {
						gameState->player.damageMulti += 0.2f;
					} else if (gameState->pickedUpgrade == UPGRADE_FIRERATE) {
						gameState->player.fireRate += 2.0f;
					}
					gameState->state = STATE_RUNNING;
				}
				break;
			}
		case STATE_GAME_OVER:
			{
				if (IsKeyPressed(KEY_ENTER)) {
					initializeGameState(gameState);
					initializeOptions(options);
					gameState->state = STATE_RUNNING;
					// gameState->player.playerPosition = (Vector2){options->screenWidth / 2.0f, options->screenHeight / 2.0f};
				}
				break;
			}
		case STATE_PAUSED:
			{
				PauseMusicStream(audio->music);
				if (IsKeyPressed(KEY_ESCAPE)) {
					gameState->state = STATE_RUNNING;
					ResumeMusicStream(audio->music);
				}
				break;
			}
	}
}

Rectangle GetScaledViewport(int winW, int winH)
{
    float scale = fminf(
        (float)winW / VIRTUAL_WIDTH,
        (float)winH / VIRTUAL_HEIGHT
    );

    float w = VIRTUAL_WIDTH  * scale;
    float h = VIRTUAL_HEIGHT * scale;

    return (Rectangle){
        (winW - w) / 2.0f,
        (winH - h) / 2.0f,
        w,
        h
    };
}

void HandleResize(float* previousWidth, float* previousHeight)
{
	int width  = GetRenderWidth();
	int height = GetRenderHeight();
	if (width != *previousWidth || height != *previousHeight) 
	{
		float aspect = (float)VIRTUAL_WIDTH / VIRTUAL_HEIGHT;
		height = (int)(width / aspect); // adjust height to maintain ratio

		if (width < MIN_SCREEN_WIDTH || height < MIN_SCREEN_HEIGHT) {
			width  = width  < MIN_SCREEN_WIDTH ? MIN_SCREEN_WIDTH : width;
			height = height < MIN_SCREEN_HEIGHT ? MIN_SCREEN_HEIGHT: height;
		}
		SetWindowSize(width, height);
	}
}

void DrawScene(GameState* gameState, Options* options, TextureAtlas* atlas, RenderTexture2D* scene, Shader shader)
{
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
	BeginTextureMode(*scene);

	const Rectangle screenRect = {
		.height = options->screenHeight,
		.width = options->screenWidth,
		.x = 0,
		.y = 0
	};

	switch (gameState->state) {
		case STATE_MAIN_MENU:
			{
				break;
			}
		case STATE_RUNNING:
			{
				Color backgroundColor = ColorFromHSV(258, 1, 0.07);
				ClearBackground(backgroundColor);

				// Draw Stars
				{
					for (int starIndex = 0; starIndex < gameState->starCount; starIndex++)
					{
						Star* star = &gameState->stars[starIndex];
						if(!CheckCollisionPointRec(star->position, screenRect))
						{
							// Replace with last star
							*star = gameState->stars[--gameState->starCount];
						}

						const int texture_x = star->position.x - star->sprite.coords.width / 2.0 * star->size;
						const int texture_y = star->position.y - star->sprite.coords.height / 2.0 * star->size;
						Color starColor = ColorAlpha(WHITE, star->alpha);
						DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_STAR1).coords, (Vector2) {texture_x, texture_y}, starColor);
					}
				}

				// if(!options->disableShaders) BeginShaderMode(shader);
				// Draw Bullets
				{
					for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
					{
						Bullet* bullet = &gameState->bullets[bulletIndex];
						Rectangle bulletRec = {
							.width = bullet->sprite.coords.width,
							.height = bullet->sprite.coords.height,
							.x = bullet->position.x,
							.y = bullet->position.y,
						};
						Vector2 texSize = { bulletRec.width, bulletRec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// SetShaderValue(lightShader, uLightPos, &bullet->position, SHADER_UNIFORM_VEC2);
						DrawTexturePro(atlas->textureAtlas, bullet->sprite.coords, bulletRec, (Vector2){0, 0}, bullet->rotation, WHITE);
						// DrawTextureRec(atlas->textureAtlas, bullet->sprite.coords, bullet->position, WHITE);
					}
				}
				// Draw asteroids
				{
					for (int asteroidIndex = 0; asteroidIndex < gameState->asteroidCount; asteroidIndex++)
					{
						Asteroid* asteroid = &gameState->asteroids[asteroidIndex];
						float width = asteroid->sprite.coords.width * asteroid->size;
						float height = asteroid->sprite.coords.height * asteroid->size;
						Rectangle asteroidDrawRect = {
							.x = asteroid->position.x,
							.y = asteroid->position.y, 
							.width = width,
							.height = height, 
						};

						Vector2 texSize = { width, height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						DrawTexturePro(atlas->textureAtlas, asteroid->sprite.coords, asteroidDrawRect, (Vector2){asteroid->collider.width/2.0f, asteroid->collider.height/2.0f}, asteroid->rotation, WHITE);
						// DrawRectangleLines(asteroid->collider.x, asteroid->collider.y, asteroid->collider.width, asteroid->collider.height, GREEN);
					}
				}
				// Draw Player
				const int texture_x = gameState->player.playerPosition.x - gameState->player.sprite.coords.width * gameState->player.size / gameState->player.animationFrames / 2.0;
				const int texture_y = gameState->player.playerPosition.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0;
				Rectangle destination = {texture_x, texture_y, 
					gameState->player.sprite.coords.width / gameState->player.animationFrames * gameState->player.size, 
					gameState->player.sprite.coords.height * gameState->player.size}; // origin in coordinates and scale
				Vector2 origin = {0, 0}; // so it draws from top left of image
				if (gameState->player.invulTime <= 0.0f) {
					DrawSpriteAnimationPro(atlas->playerAnimation, destination, origin, 0, WHITE, shader);
				} else {
					if (((int)(gameState->player.invulTime * 10)) % 2 == 0) {
						DrawSpriteAnimationPro(atlas->playerAnimation, destination, origin, 0, WHITE, shader);
					}
				}
				// DrawRectangleLines(destination.x, destination.y, destination.width, destination.height, RED);
				// if(!options->disableShaders) EndShaderMode();
				break;
			}
		case STATE_UPGRADE:
			{
				break;
			}
		case STATE_GAME_OVER:
			{
				break;
			}
		case STATE_PAUSED:
			{
				break;
			}
	}
	EndTextureMode(); // scene
}

void DrawLightmap(GameState* gameState, Options* options, RenderTexture2D* litScene, Shader lightShader)
{
	int uLightPos = GetShaderLocation(lightShader, "lightPos");
	int uLightRadius = GetShaderLocation(lightShader, "lightRadius");
	int uAspect = GetShaderLocation(lightShader, "aspect");
	float lightRadius = 0.1f;  // normalized radius
	float aspect = (float)options->screenWidth / (float)options->screenHeight;
	SetShaderValueTexture(lightShader, GetShaderLocation(lightShader, "lightTexture"), litScene->texture);
	SetShaderValue(lightShader, uLightRadius, &lightRadius, SHADER_UNIFORM_FLOAT);
	SetShaderValue(lightShader, uAspect, &aspect, SHADER_UNIFORM_FLOAT);
	// --- Build lightmap ---
	int uLightCount = GetShaderLocation(lightShader, "lightCount");
	BeginTextureMode(*litScene);
	ClearBackground(BLACK);
	BeginBlendMode(BLEND_ADDITIVE);
	// Prepare arrays
	Vector2 lights[128];
	int lc = 0;

	for (int i = 0; i < gameState->bulletCount; i++) {
		// convert pixel -> normalized UV (0–1)
		lights[lc].x = gameState->bullets[i].position.x / (float)options->screenWidth;
		lights[lc].y = 1.0f - gameState->bullets[i].position.y / (float)options->screenHeight;
		lc++;
	}

	lights[lc].x = gameState->player.playerPosition.x / (float)options->screenWidth;
	lights[lc].y = 1.0f - gameState->player.playerPosition.y / (float)options->screenHeight;
	lc++;
	// Upload array
	SetShaderValue(lightShader, uLightCount, &lc, SHADER_UNIFORM_INT);
	SetShaderValueV(lightShader, uLightPos, lights, SHADER_UNIFORM_VEC2, lc);

	EndBlendMode();
	EndTextureMode();
}

void DrawUI(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	Rectangle dst = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	switch (gameState->state) {
		case STATE_RUNNING:
			{
				// Draw player health
				for (int i = 1; i <= gameState->player.playerHealth; i++)
				{
					const int texture_x = i * 16;
					const int texture_y = getSprite(SPRITE_HEART).coords.height;
					DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
				}
				// Draw Score
				float recPosX = dst.width * 0.9;
				float recPosY = dst.height * 0.05;
				float recHeight = 30.0f;
				float recWidth = 100.0f;
				DrawRectangle(recPosX, recPosY, gameState->experience / 10.0f, recHeight, ColorAlpha(BLUE, 0.5));
				DrawRectangleLines(recPosX, recPosY, recWidth, recHeight, ColorAlpha(WHITE, 0.5));
				// char experienceText[100] = "XP";
				Vector2 textSize = MeasureTextEx(options->font, T(TXT_EXPERIENCE), 20.0f, GetDefaultSpacing(20.0f));
				DrawTextEx(options->font, T(TXT_EXPERIENCE), (Vector2){recPosX + recWidth / 2.0f - textSize.x / 2.0f, recPosY + recHeight / 2.0f - textSize.y / 2.0f}, 20.0f, GetDefaultSpacing(20.0f), WHITE);
				break;
			}
		case STATE_MAIN_MENU:
			{
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				draw_text_centered(options->font, T(TXT_GAME_TITLE), (Vector2){dst.width/2.0f, dst.height/2.0f}, 40, GetDefaultSpacing(40), WHITE);
				draw_text_centered(options->font, T(TXT_PRESS_TO_PLAY), (Vector2){dst.width/2.0f, dst.height/2.0f + 30}, 20, GetDefaultSpacing(20), WHITE);
				draw_text_centered(options->font, T(TXT_INSTRUCTIONS), (Vector2){dst.width/2.0f, dst.height/2.0f + 50}, 20, GetDefaultSpacing(20), WHITE);
				draw_text_centered(options->font, "v0.1", (Vector2){dst.width/2.0f, dst.height - 15}, 15, GetDefaultSpacing(15),  WHITE);

				char testBuffer[2048] = {0};
				TWrap(testBuffer, 2048, options->font, "This is a test of the text wrapping function.", 50.0, 20.0, GetDefaultSpacing(20));
				draw_text_centered(options->font, testBuffer, (Vector2){dst.width/2.0f, dst.height - 100}, 15, GetDefaultSpacing(15),  WHITE);

				TWrap(testBuffer, 2048, options->font, "This is a test of the text wrapping function. This should also be aligned to the center", 50.0, 15.0, GetDefaultSpacing(15));
				DrawTextWrapped(options->font, testBuffer,
								(Vector2){50,60},
								50.0f,
								15, GetDefaultSpacing(15),
								ALIGN_CENTER,
								WHITE);

				TWrap(testBuffer, 2048, options->font, "This is a test of the text wrapping function. This should also be aligned to the right", 50.0, 20.0, GetDefaultSpacing(20));
				DrawTextWrapped(options->font, testBuffer,
								(Vector2){150,60},
								50.0f,
								20, GetDefaultSpacing(20),
								ALIGN_RIGHT,
								WHITE);
				break;
			}
		case STATE_GAME_OVER:
			{
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				draw_text_centered(options->font, T(TXT_GAME_OVER), (Vector2){dst.width/2.0f, dst.height/2.0f}, 40, options->fontSpacing, WHITE);
				char scoreText[100] = {0};
				sprintf(scoreText, "Final score: %d", gameState->experience);
				draw_text_centered(options->font, scoreText, (Vector2){dst.width/2.0f, dst.height/2.0f + 30.0f}, 20.0f, options->fontSpacing, WHITE);
				draw_text_centered(options->font, T(TXT_TRY_AGAIN), (Vector2){dst.width/2.0f, dst.height/2.0f + 60.0f}, 20.0f, options->fontSpacing, WHITE);
				break;
			}
		case STATE_PAUSED:
			{
				// Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				// ClearBackground(backgroundColor);
				// Draw player health
				for (int i = 1; i <= gameState->player.playerHealth; i++)
				{
					const int texture_x = i * 16;
					const int texture_y = getSprite(SPRITE_HEART).coords.height;
					DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
				}
				// Draw Score
				float recPosX = dst.width * 0.9;
				float recPosY = dst.height * 0.05;
				float recHeight = 30.0f;
				float recWidth = 100.0f;
				DrawRectangle(recPosX, recPosY, gameState->experience / 10.0f, recHeight, ColorAlpha(BLUE, 0.5));
				DrawRectangleLines(recPosX, recPosY, recWidth, recHeight, ColorAlpha(WHITE, 0.5));
				// char experienceText[100] = "XP";
				Vector2 textSize = MeasureTextEx(options->font, T(TXT_EXPERIENCE), 20.0f, GetDefaultSpacing(20.0f));
				DrawTextEx(options->font, T(TXT_EXPERIENCE), (Vector2){recPosX + recWidth / 2.0f - textSize.x / 2.0f, recPosY + recHeight / 2.0f - textSize.y / 2.0f}, 20.0f, GetDefaultSpacing(20.0f), WHITE);
				draw_text_centered(options->font, T(TXT_GAME_PAUSED), (Vector2){dst.width/2.0f, dst.height/2.0f}, 40, options->fontSpacing, WHITE);
				// Draw a window box
				float boxWidth = 500.0f;
				float boxHeight = 500.0f;
				GuiWindowBox((Rectangle){ options->screenWidth/4-boxWidth/2, options->screenHeight/2-boxHeight/2, boxWidth, boxHeight }, "Settings");
				// Language label
				float labelWidth = 160.0f;
				float labelHeight = 100.0f;
				GuiLabel((Rectangle){ options->screenWidth/4-labelWidth/2-boxWidth/4, options->screenHeight/2-labelHeight/2-boxHeight/4, labelWidth, labelHeight}, "Language:");
				// Build semicolon-separated strings
				// Note: the label string must list all items separated by ';'
				const char *langItems = "English;German;Chinese";
				// Draw dropdown box
				float dropdownWidth = 240.0f;
				float dropdownHeight = 60.0f;
				if (GuiDropdownBox((Rectangle){ options->screenWidth/4-dropdownWidth/2+boxWidth/4, 
												options->screenHeight/2-dropdownHeight/2-boxHeight/4,
												labelWidth, labelHeight}, langItems, 
												&options->language, editMode))
				{
					// Toggled edit mode when clicked
					editMode = !editMode;
				}
				if (options->language != options->lastLanguage)
				{
					switch (options->language)
					{
						case 0: LocSetLanguage(LANG_EN); break;
						case 1: LocSetLanguage(LANG_DE); break;
						case 2: LocSetLanguage(LANG_ZH); break;
					}

					UnloadFont(options->font);
					options->font = LoadLanguageFont("fonts/UnifontExMono.ttf", options->maxFontSize, options->language);
					GuiSetFont(options->font);
					options->lastLanguage = options->language;
				}
				break;
			}
		case STATE_UPGRADE:
			{
				// Draw player health
				for (int i = 1; i <= gameState->player.playerHealth; i++)
				{
					const int texture_x = i * 16;
					const int texture_y = getSprite(SPRITE_HEART).coords.height;
					DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
				}
				// Draw Score
				float recPosX = dst.width * 0.9;
				float recPosY = dst.height * 0.05;
				float recHeight = 30.0f;
				float recWidth = 100.0f;
				DrawRectangle(recPosX, recPosY, gameState->experience / 10.0f, recHeight, ColorAlpha(BLUE, 0.5));
				DrawRectangleLines(recPosX, recPosY, recWidth, recHeight, ColorAlpha(WHITE, 0.5));
				// char experienceText[100] = "XP";
				Vector2 textSize = MeasureTextEx(options->font, T(TXT_EXPERIENCE), 20.0f, GetDefaultSpacing(20.0f));
				DrawTextEx(options->font, T(TXT_EXPERIENCE), (Vector2){recPosX + recWidth / 2.0f - textSize.x / 2.0f, recPosY + recHeight / 2.0f - textSize.y / 2.0f}, 20.0f, GetDefaultSpacing(20.0f), WHITE);

				draw_text_centered(options->font, T(TXT_LEVEL_UP), (Vector2){dst.width/2.0f, dst.height/2.0f - 80.0f}, 40, options->fontSpacing, WHITE);
				draw_text_centered(options->font, T(TXT_CHOOSE_UPGRADE), (Vector2){dst.width/2.0f, dst.height/2.0f - 35.0f}, 40, options->fontSpacing, WHITE);
				const int width = getSprite(SPRITE_UPGRADEMULTISHOT).coords.width*3.0f;
				const int height = getSprite(SPRITE_UPGRADEMULTISHOT).coords.height*3.0f;                
				const int pos_x = dst.width/2.0f - width/2.0f;
				const int pos_y = dst.height/2.0f - height/2.0f + 90.0f;
				const int spacing_x = 240;
				Rectangle upgradeRect = {
					.width = width,
					.height = height,
					.x = pos_x,
					.y = pos_y,
				};
				switch (gameState->pickedUpgrade)
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
				DrawTexturePro(atlas->textureAtlas, getSprite(SPRITE_UPGRADEMULTISHOT).coords, upgradeRect, (Vector2){0, 0}, 0.0f, WHITE); 
				upgradeRect.x -= spacing_x;
				DrawTexturePro(atlas->textureAtlas, getSprite(SPRITE_UPGRADEDAMAGE).coords, upgradeRect, (Vector2){0, 0}, 0.0f, WHITE); 
				upgradeRect.x += 2.0f * spacing_x;
				DrawTexturePro(atlas->textureAtlas, getSprite(SPRITE_UPGRADEFIRERATE).coords, upgradeRect, (Vector2){0, 0}, 0.0f, WHITE); 
			}
	}
	DrawFPS(10, 40);
}

void DrawComposite(RenderTexture2D* scene, Options* options, RenderTexture2D* litScene, GameState* gameState, Shader lightShader)
{
    Rectangle dst = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
    Rectangle src = {
        0, 0,
        (float)scene->texture.width,
        -(float)scene->texture.height
    };

    if (!options->disableShaders) BeginShaderMode(lightShader);
    DrawTexturePro(scene->texture, src, dst, (Vector2){0, 0}, 0, WHITE);
    if (!options->disableShaders) EndShaderMode();
}

void UpdateDrawFrame()
{
	float dt = GetFrameTime() * gameState.timeScale;
	HandleResize(&options.previousWidth, &options.previousHeight);
	UpdateGame(&gameState, &options, &atlas, spriteMasks, &audio, dt);
	DrawLightmap(&gameState, &options, &litScene, lightShader);
	DrawScene(&gameState, &options, &atlas, &scene, shader);

	BeginDrawing();
	{
		ClearBackground(BLACK);
		DrawComposite(&scene, &options, &litScene, &gameState, lightShader);
		DrawUI(&gameState, &options, &atlas);
	}
	EndDrawing();
}

int main() {
    SetTargetFPS(TARGET_FPS);

	LocSetLanguage(LANG_EN);
	// LocSetLanguage(LANG_DE);
	// LocSetLanguage(LANG_ZH);

	ConfigFlags configFlags = FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_BORDERLESS_WINDOWED_MODE;
	SetConfigFlags(configFlags);
	gameState.player.playerPosition = (Vector2){options.screenWidth / 2.0f, options.screenHeight / 2.0f};

	InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, WINDOW_TITLE);
	SetWindowMinSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	// Disable Exit on ESC
    SetExitKey(KEY_NULL);
	initializeOptions(&options);
	initializeGameState(&gameState);

    atlas = initTextureAtlas(spriteMasks);
	initializeAudio(&audio);

	scene = LoadRenderTexture(options.screenWidth, options.screenHeight);
	litScene = LoadRenderTexture(options.screenWidth, options.screenHeight);

#ifdef PLATFORM_WEB
	shader = LoadShader(0, TextFormat("shaders/test_web.glsl", GLSL_VERSION));
	lightShader = LoadShader(0, TextFormat("shaders/light_web.fs", GLSL_VERSION));
#else
	shader = LoadShader(0, TextFormat("shaders/test.glsl", GLSL_VERSION));
	lightShader = LoadShader(0, TextFormat("shaders/light.fs", GLSL_VERSION));
#endif

	options.previousWidth  = VIRTUAL_WIDTH;
	options.previousHeight = VIRTUAL_HEIGHT;
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, TARGET_FPS, 1);
#else
    while (!WindowShouldClose())
    { 
		UpdateDrawFrame();
    }
#endif
	UnloadShader(shader);
	UnloadShader(lightShader);
	cleanup(atlas, options, audio, spriteMasks);

    CloseWindow();
    return 0;
}

