// To build on linux: 
// gcc asteroids.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
#include <stdio.h>
#include <math.h>

#include "assetsData.h"
#include "txt.h"
#include "assetsUtils.h"
#include "localization.h"

#include "raymath.h"
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "third_party/include/raygui.h"
#include "third_party/include/style_dark.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MIN_SCREEN_WIDTH (1440.0f)
#define MIN_SCREEN_HEIGHT (810.0f)
#define VIRTUAL_WIDTH (1440.0f)
#define VIRTUAL_HEIGHT (810.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (1000)
#define MAX_ASTEROIDS (100)
#define MAX_EXPLOSIONS (20)
#define MAX_STARS (50)
#define MAX_BOOSTS (2)
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
    UPGRADE_DAMAGE,
	UPGRADE_MULTISHOT,
    UPGRADE_FIRERATE,
    UPGRADE_COUNT,
} Upgrade;

typedef struct UpgradeCard {
	Rectangle rect;
	float animationTime;
} UpgradeCard;

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
    Vector2 velocity;
    float angularVelocity;
    float rotation;
    char textureFile[100];
    Sprite sprite;
	Rectangle collider;
} Asteroid;

typedef struct Boost {
	Vector2 position;
	Vector2 velocity;
	float size;
	float rotation;
	float angularVelocity;
	Sprite sprite;
	Rectangle collider;
} Boost;

typedef struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float damage;
    Sprite sprite;
    float rotation;
	float size;
	Rectangle collider;
} Bullet;

typedef struct Explosion {
    Vector2 position;
	Vector2 velocity;
    float startTime;
    bool active;
} Explosion;

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
	bool shieldEnabled;
	float shieldTime;
} Player;

typedef struct Audio {
    Sound hitFx;
    Sound blastFx;
	Sound cardFx;
	Sound explosionFx;
	Sound shieldFx;
	Sound langEn;
	Sound langDe;
	Sound langZh;
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
	bool languageEditMode;
	bool languageChanged;
	bool disableCursor;
	Vector2 lastMousePos;
	float musicVolume;
	float fxVolume;
	bool musicVolumeChanged;
	bool fxVolumeChanged;
} Options;

typedef struct GameState {
    // General
    int experience;
    State state;
	State lastState;
	float timeScale;
    // Player
    Player player;
    // Projectiles
    Bullet bullets[MAX_BULLETS];
    int bulletCount;
	Explosion explosions[MAX_EXPLOSIONS];
	int explosionCount;
    // asteroids
    Asteroid asteroids[MAX_ASTEROIDS];
    int asteroidCount;
	float spawnTime;
	float asteroidSpawnRate;
	Boost boosts[MAX_BOOSTS];
	int boostCount;
	float boostSpawnTime;
	float boostSpawnRate;
    // float lastasteroidXPosition;
    // Parallax background stars
    Star stars[MAX_STARS];
    int starCount;
    float starTime;
    float starSpawnRate;
    int initStars;
    Upgrade pickedUpgrade;
	float dt;
	float time;
	// float upgradeSelectAnim[UPGRADE_COUNT];
	UpgradeCard upgradeCards[UPGRADE_COUNT];
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
static bool shouldExit = false;

void cleanup(TextureAtlas atlas, Options options, Audio audio, SpriteMask spriteMasks[]) {
	UnloadShader(shader);
	UnloadShader(lightShader);
	for (int i = 0; i < ANIMATION_COUNT; i++)
	{
		FreeSpriteAnimation(atlas.animations[i]);
	}

    UnloadFont(options.font);
	for (int i = 0; i < SPRITE_COUNT; i++)
	{
		UnloadImageColors(spriteMasks[i].pixels);
	}
	UnloadMusicStream(audio.music);
	UnloadSound(audio.hitFx);
	UnloadSound(audio.blastFx);
	UnloadSound(audio.cardFx);
	UnloadSound(audio.shieldFx);
	UnloadSound(audio.langEn);
	UnloadSound(audio.langDe);
	UnloadSound(audio.langZh);
	UnloadSound(audio.explosionFx);
	CloseAudioDevice();
}

void setFxVolume(Audio* audio, float volume) {
	SetSoundVolume(audio->hitFx, volume);
	SetSoundVolume(audio->blastFx, volume*0.5f);
	SetSoundVolume(audio->explosionFx, volume*0.5f);
	SetSoundVolume(audio->shieldFx, volume);
	SetSoundVolume(audio->cardFx, volume);
	SetSoundVolume(audio->langEn, volume);
	SetSoundVolume(audio->langDe, volume);
	SetSoundVolume(audio->langZh, volume);
}

void initializeAudio(Audio* audio, Options* options) {
	InitAudioDevice();
	ASSERT(IsAudioDeviceReady());
	audio->music = LoadMusicStream("audio/soundtrack.mp3");
	ASSERT(IsMusicValid(audio->music));
	audio->hitFx = LoadSound("audio/hit.wav");
	ASSERT(IsSoundValid(audio->hitFx));
	audio->blastFx = LoadSound("audio/laserPowerGunshot.wav");
	ASSERT(IsSoundValid(audio->blastFx));
	audio->cardFx = LoadSound("audio/cardSelect.mp3");
	ASSERT(IsSoundValid(audio->cardFx));
	audio->langEn = LoadSound("audio/america.mp3");
	ASSERT(IsSoundValid(audio->langEn));
	audio->langDe = LoadSound("audio/erika.mp3");
	ASSERT(IsSoundValid(audio->langDe));
	audio->langZh = LoadSound("audio/bingchilling.mp3");
	ASSERT(IsSoundValid(audio->langZh));
	audio->explosionFx = LoadSound("audio/explosionBlast.wav");
	ASSERT(IsSoundValid(audio->explosionFx));
	audio->shieldFx = LoadSound("audio/shield.wav");
	ASSERT(IsSoundValid(audio->shieldFx));
	PlayMusicStream(audio->music);
	SetMusicVolume(audio->music, options->musicVolume);
	setFxVolume(audio, options->fxVolume);
}

void initializeGameState(GameState* gameState) {
    *gameState = (GameState) {
        .state = STATE_MAIN_MENU,
		.lastState = STATE_MAIN_MENU,
		.timeScale = 1.0f,
        .bulletCount = 0,
        .asteroidCount = 0,
        .asteroidSpawnRate = 0.2f,
        .boostCount = 0,
        .boostSpawnTime = 0.0f,
        .boostSpawnRate = 1.0f,
        .spawnTime = 0.0,
        .experience = 1000,
        .starCount = 0,
        .starTime = 0,
        .starSpawnRate = 0.25f,
        .initStars = 0,
        .pickedUpgrade = UPGRADE_MULTISHOT,
		.dt = 0.0f,
		.time = 0.0f,
    };

	for (int i = 0; i < UPGRADE_COUNT; i++)
	{
		gameState->upgradeCards[i].animationTime = 0.0f;
		gameState->upgradeCards[i].rect = (Rectangle){0};
	}

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
        .damageMulti = 1.5f,
		.shieldEnabled = false,
		.shieldTime = 5.25f,
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
		.languageChanged = false,
		.disableCursor = false,
		.lastMousePos = (Vector2){0,0},
		.musicVolume = 0.05f,
		.fxVolume = 0.25f,
		.musicVolumeChanged = false,
		.fxVolumeChanged = false,
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


void draw_text_centered(Font font, const char* text, Vector2 pos, int fontSize, Color color)
{
	float fontSpacing = GetDefaultSpacing(fontSize);
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

void HandleResize(Options* options)
{
	float* previousWidth = &options->previousWidth;
	float* previousHeight = &options->previousHeight;
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
		options->screenWidth = width;
		options->screenHeight = height;
		// SetWindowSize(width, height);
	}
}

void UpdateGame(GameState* gameState, Options* options, TextureAtlas* atlas, SpriteMask spriteMasks[], Audio* audio, float dt)
{
	static bool cursorHidden = true;
	static bool stepMode = false;
	static bool stepOnce = false;
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());

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
				float viewportScale = viewport.width / VIRTUAL_WIDTH;
				if (!IsMusicStreamPlaying(audio->music)) ResumeMusicStream(audio->music);
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
					.height = viewport.height,
					.width = viewport.width,
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
				// Update Player
				if (gameState->player.invulTime < 0.0f)
				{
					gameState->player.invulTime = 0.0f;
				} else {
					gameState->player.invulTime -= dt;
				}

				// float screenWidth = VIRTUAL_WIDTH;
				// float screenHeight = VIRTUAL_HEIGHT;
				if (IsKeyDown(KEY_W)) {
					if(!CheckCollisionPointLine((Vector2) {gameState->player.playerPosition.x * viewportScale, gameState->player.playerPosition.y * viewportScale},
								                (Vector2) {0, 0}, 
												(Vector2) {viewport.width, 0},
												gameState->player.sprite.coords.height * viewportScale))
					{
						gameState->player.playerPosition.y -= gameState->player.playerVelocity * dt;
					}   
				}
				if (IsKeyDown(KEY_S)) {
					if(!CheckCollisionPointLine((Vector2) {gameState->player.playerPosition.x * viewportScale, gameState->player.playerPosition.y * viewportScale},
								                (Vector2) {0, viewport.height},
												(Vector2) {viewport.width, viewport.height},
												gameState->player.sprite.coords.height * viewportScale))
					{
						gameState->player.playerPosition.y += gameState->player.playerVelocity * dt;
					}
				}
				if (IsKeyDown(KEY_A)) {
					if(!CheckCollisionPointLine((Vector2) {gameState->player.playerPosition.x * viewportScale, gameState->player.playerPosition.y * viewportScale},
								                (Vector2) {0, 0}, 
												(Vector2) {0, viewport.height},
												gameState->player.sprite.coords.width / gameState->player.animationFrames * viewportScale))
					{
						gameState->player.playerPosition.x -= gameState->player.playerVelocity * dt;
					}
				}
				if (IsKeyDown(KEY_D)) {
					if(!CheckCollisionPointLine((Vector2) {gameState->player.playerPosition.x * viewportScale, gameState->player.playerPosition.y * viewportScale},
												(Vector2) {viewport.width, 0},
												(Vector2) {viewport.width, viewport.height},
												gameState->player.sprite.coords.width / gameState->player.animationFrames * viewportScale))
					{
						gameState->player.playerPosition.x += gameState->player.playerVelocity * dt;
					}
				}
				if (gameState->player.shootTime < 1.0f/gameState->player.fireRate)
				{
					gameState->player.shootTime += dt;
				} 
				if (gameState->player.shieldEnabled && gameState->player.shieldTime > 0.0f)
				{
					gameState->player.shieldTime -= dt;
				}
				if (gameState->player.shieldTime < 0.0f)
				{
					gameState->player.shieldEnabled = false;
					gameState->player.shieldTime = 5.25f;
				}
				// Shoot bullets
				while (IsKeyDown(KEY_SPACE) 
						&& gameState->player.shootTime >= 1.0f/gameState->player.fireRate
						&& gameState->bulletCount <= MAX_BULLETS)
				{

					PlaySound(audio->blastFx);
					float bulletSize = 0.5f;
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
							.size = bulletSize,
						};
						// bullet1.position.x -= bullet1.sprite.coords.width * bullet1.size / bullet1.sprite.numFrames / 2.0f;
						bullet1.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 - bullet1.sprite.coords.height * bullet1.size / 2.0;
						gameState->bullets[gameState->bulletCount++] = bullet1;
						Bullet bullet2 = 
						{
							.position = (Vector2){gameState->player.playerPosition.x - bulletOffset, gameState->player.playerPosition.y + 0.0f},
							.velocity = (Vector2){sqrt(pow(500.0f,2) - pow(450.0f,2)), 450.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = -15.0f,
							.size = bulletSize,
						};
						// bullet2.position.x -= bullet2.sprite.coords.width * bullet2.size / bullet2.sprite.numFrames / 2.0f;
						bullet2.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 - bullet2.sprite.coords.height * bullet2.size / 2.0;
						gameState->bullets[gameState->bulletCount++] = bullet2;
						Bullet bullet3 = 
						{
							.position = (Vector2){gameState->player.playerPosition.x + bulletOffset, gameState->player.playerPosition.y + 0.0f},
							.velocity = (Vector2){-sqrt(pow(500.0f,2) - pow(450.0f,2)), 450.0f},
							.damage = 1.0*gameState->player.damageMulti,
							.sprite = getSprite(SPRITE_BULLET),
							.rotation = 15.0f,
							.size = bulletSize,
						};
						// bullet3.position.x -= bullet3.sprite.coords.width * bullet3.size / bullet3.sprite.numFrames / 2.0f;
						bullet3.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 - bullet3.sprite.coords.height * bullet3.size / 2.0;
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
							.size = bulletSize,
						};
						bullet.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0 - bullet.sprite.coords.height * bullet.size / 2.0;
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
						// float size = 1.0f;
						float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
						float minSpawnDistance = 50.0f * size;  
						float asteroidXPosition = MAX(minSpawnDistance, GetRandomValue(0, viewport.width));
						Asteroid asteroid =
						{
							.position = (Vector2) {asteroidXPosition, 0},
							.health = (int) (size + 1.0) * 2.0,
							.velocity = (Vector2) {0, GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size},
							.angularVelocity = GetRandomValue(-40.0f, 40.0f),
							.size = size,
						};
						int whichAsteroid = GetRandomValue(1,10);
						Sprite asteroidSprite;
						if (whichAsteroid < 6) {
							asteroidSprite = getSprite(SPRITE_ASTEROID1);
						} else if (whichAsteroid < 9) {
							asteroidSprite = getSprite(SPRITE_ASTEROID2);
						} else {
							asteroidSprite = getSprite(SPRITE_ASTEROID3);
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
						asteroid->position.y += asteroid->velocity.y * dt;
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
							if(CheckCollisionRecs(asteroid->collider, bullet->collider))
							{
								Rectangle collisionRec = GetCollisionRec(asteroid->collider, bullet->collider);
								Rectangle bulletSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_BULLET]]);
								if (pixelPerfectCollision(spriteMasks[SPRITE_BULLET].pixels, spriteMasks[asteroid->sprite.spriteID].pixels, 
											bulletSrc.width, asteroid->sprite.coords.width,
											bulletSrc.height, asteroid->sprite.coords.height,
											bullet->collider, asteroid->collider, collisionRec, bullet->rotation, asteroid->rotation))
								{

									Explosion* explosion = NULL;
									if (gameState->explosionCount < MAX_EXPLOSIONS)
									{
										PlaySound(audio->explosionFx);
										explosion = &gameState->explosions[gameState->explosionCount++];
										explosion->position = bullet->position;
										explosion->position.y -= bulletSrc.height/2.0f;
										explosion->velocity = asteroid->velocity;
										explosion->startTime = GetTime();
										explosion->active = true;
									}
									// Replace with bullet with last bullet
									*bullet = gameState->bullets[--gameState->bulletCount];
									asteroid->health -= bullet->damage;
									if (asteroid->health < 1)
									{
										gameState->experience += MAX((int)(asteroid->size * 100),1);
										*asteroid = gameState->asteroids[--gameState->asteroidCount];
										if(explosion != NULL)
										{
											explosion->velocity.y = 0.0f;
										}
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
							.width = viewport.width + asteroid->sprite.coords.width,
							.height = viewport.height + asteroid->sprite.coords.height,
							.x = 0.0,
							.y = asteroid->position.y, // to make them scroll into screen smoothly
						};
						if(!CheckCollisionPointRec(asteroid->position, screenRectExtended))
						{
							// Replace with last asteroid
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
						if(CheckCollisionRecs(asteroid->collider, playerRec) && 
								gameState->player.invulTime <= 0.0f &&
								gameState->player.shieldEnabled == false)
						{
							Rectangle collisionRec = GetCollisionRec(asteroid->collider, playerRec);
							// DrawRectangleLinesEx(collisionRec, 2.0, RED);
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_PLAYER]]);
							if (pixelPerfectCollision(spriteMasks[SPRITE_PLAYER].pixels, spriteMasks[asteroid->sprite.spriteID].pixels, 
										playerSrc.width, asteroid->sprite.coords.width,
										playerSrc.height, asteroid->sprite.coords.height, 
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
						if (asteroid->position.y > viewport.height + asteroid->sprite.coords.height * asteroid->size)
						{
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
					}
				}
				// Update explosions
				for (int explosionIndex = 0; explosionIndex < gameState->explosionCount; explosionIndex++)
				{
					Explosion* explosion = &gameState->explosions[explosionIndex];
					if (explosion->active)
					{
						explosion->position.y += explosion->velocity.y * dt;
					}
				}

				// Spawn boosts
				{
					gameState->boostSpawnTime += dt;
					if (gameState->boostSpawnTime > gameState->boostSpawnRate && gameState->boostCount < MAX_BOOSTS)
					{
						// int size = (int)GetRandomValue(1, 3);
						float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
						// float size = 1.0f;
						float minSpawnDistance = 50.0f * size;  
						float boostXPosition = MAX(minSpawnDistance, GetRandomValue(0, viewport.width));
						Boost boost = {
							.position = (Vector2) {boostXPosition, 0},
							.velocity = (Vector2) {0, GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size},
							// .angularVelocity = GetRandomValue(-40.0f, 40.0f),
							.angularVelocity = 0.0f,
							.size = 2.0f,
							// .rotation = GetRandomValue(0.0f, 360.0f),
							.rotation = 0.0f,
						};
						Sprite boostSprite;
						boostSprite = getSprite(SPRITE_SCRAPMETAL);
						boost.sprite = boostSprite;
						boost.position.y -= boost.sprite.coords.height; // to make them come into screen smoothly
						gameState->boostSpawnTime = 0;
						gameState->boosts[gameState->boostCount++] = boost;
					}
				}
				// Update boosts
				{
					for (int boostIndex = 0; boostIndex < gameState->boostCount; boostIndex++)
					{
						Boost* boost = &gameState->boosts[boostIndex];
						boost->position.y += boost->velocity.y * dt;
						boost->rotation += boost->angularVelocity * dt;
						float width  = boost->sprite.coords.width / boost->sprite.numFrames * boost->size;
						float height = boost->sprite.coords.height * boost->size;
						boost->collider = (Rectangle) {
							.x = boost->position.x - width / 2.0f,
							.y = boost->position.y - height / 2.0f,
							.width = width,
							.height = height, 
						};
						// Check if boost is off-screen
						if (boost->position.y > viewport.height + boost->sprite.coords.height * boost->size)
						{
							*boost = gameState->boosts[--gameState->boostCount];
						}
						float playerWidth = gameState->player.sprite.coords.width/gameState->player.animationFrames * gameState->player.size;
						float playerHeight = gameState->player.sprite.coords.height * gameState->player.size;
						Rectangle playerRec = {
							.width = playerWidth,
							.height =  playerHeight,
							.x = gameState->player.playerPosition.x - playerWidth/2.0f,
							.y = gameState->player.playerPosition.y - playerHeight/2.0f,
						};
						if(CheckCollisionRecs(boost->collider, playerRec))
						{
							Rectangle collisionRec = GetCollisionRec(boost->collider, playerRec);
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_PLAYER]]);
							Rectangle boostSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_SCRAPMETAL]]);
							if (pixelPerfectCollision(spriteMasks[SPRITE_PLAYER].pixels, spriteMasks[boost->sprite.spriteID].pixels, 
										playerSrc.width, boostSrc.width,
										playerSrc.height, boostSrc.height,
										playerRec, boost->collider, collisionRec, 0.0f, boost->rotation))
							{
								*boost = gameState->boosts[--gameState->boostCount];
								gameState->player.shieldEnabled = true;
								PlaySound(audio->shieldFx);
							}
						}
					}
				}
				if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
					gameState->state = STATE_PAUSED;
					gameState->lastState = STATE_RUNNING;
				}
				break;
			}
		case STATE_UPGRADE:
			{
				if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
					gameState->state = STATE_PAUSED;
					gameState->lastState = STATE_UPGRADE;
				}
				UpdateMusicStream(audio->music);

				Vector2 mousePos = GetMousePosition();
				bool clickedUpgrade = false;
				int lastUpgrade = gameState->pickedUpgrade; 
				if (IsKeyPressed(KEY_LEFT)) 
				{
					PlaySound(audio->cardFx);
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade - 1 + UPGRADE_COUNT) % UPGRADE_COUNT);
				} 
				else if (IsKeyPressed(KEY_RIGHT)) 
				{
					PlaySound(audio->cardFx);
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade + 1) % UPGRADE_COUNT);
				}
				if (mousePos.x == options->lastMousePos.x || mousePos.y == options->lastMousePos.y)
				{
					options->disableCursor = true;
				} 
				else 
				{
					options->disableCursor = false;
				}
				options->lastMousePos = mousePos;
				for (int i = 0; i < UPGRADE_COUNT; i++) 
				{
					if (CheckCollisionPointRec(mousePos, gameState->upgradeCards[i].rect) && !options->disableCursor)
					{
						gameState->pickedUpgrade = i; 
						if (lastUpgrade != gameState->pickedUpgrade) 
						{
							PlaySound(audio->cardFx);
						}
						if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
						{
							clickedUpgrade = true;
						}
						break;
					}
				}
				if (IsKeyPressed(KEY_ENTER) || clickedUpgrade) {
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
				}
				break;
			}
		case STATE_PAUSED:
			{
				PauseMusicStream(audio->music);
				if (options->languageChanged)
				{
					options->languageChanged = false;
					switch (options->language)
					{
						case LANG_EN: PlaySound(audio->langEn); break;
						case LANG_DE: PlaySound(audio->langDe); break;
						case LANG_ZH: PlaySound(audio->langZh); break;
					}
				}
				if (options->musicVolumeChanged) 
				{
					SetMusicVolume(audio->music, options->musicVolume);
					options->musicVolumeChanged = false;
				}
				if (options->fxVolumeChanged) 
				{
					setFxVolume(audio, options->fxVolume);
					options->fxVolumeChanged = false;
				}
				if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) 
				{
					gameState->state = gameState->lastState;
				}
				break;
			}
	}
}


void DrawScene(GameState* gameState, Options* options, TextureAtlas* atlas, RenderTexture2D* scene, Shader shader)
{
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
	BeginTextureMode(*scene);

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
						const int texture_x = bullet->position.x - bullet->sprite.coords.width * bullet->size / getSprite(SPRITE_BULLET).numFrames / 2.0;
						const int texture_y = bullet->position.y - bullet->sprite.coords.height * bullet->size / 2.0;
						const float width = bullet->sprite.coords.width / getSprite(SPRITE_BULLET).numFrames * bullet->size;
						const float height = bullet->sprite.coords.height * bullet->size;
						Rectangle bulletRec = {
							.width = width,
							.height = height,
							.x = texture_x,
							.y = texture_y,
						};
						bullet->collider = bulletRec;
						Vector2 texSize = { bulletRec.width, bulletRec.height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// Change the frame per second speed of animation
						// atlas->animations[SpriteToAnimation[SPRITE_BULLET]].framesPerSecond = 14;
						DrawSpriteAnimationPro(atlas->textureAtlas, atlas->animations[SpriteToAnimation[SPRITE_BULLET]], bulletRec, (Vector2){0, 0}, bullet->rotation, WHITE, shader);
						// DrawRectangleLines(bulletRec.x, bulletRec.y, bulletRec.width, bulletRec.height, RED);
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
						DrawTexturePro(atlas->textureAtlas, asteroid->sprite.coords, asteroidDrawRect, 
								(Vector2){asteroid->collider.width/2.0f, asteroid->collider.height/2.0f}, asteroid->rotation, WHITE);
						// DrawRectangleLines(asteroid->collider.x, asteroid->collider.y, 
						// 		asteroid->collider.width, asteroid->collider.height, GREEN);
						// DrawRectangleLines(asteroidDrawRect.x, asteroidDrawRect.y,
						// 		asteroidDrawRect.width, asteroidDrawRect.height, GREEN);
					}
				}
				// Draw boosts
				{
					for (int boostIndex = 0; boostIndex < gameState->boostCount; boostIndex++)
					{
						Boost* boost = &gameState->boosts[boostIndex];
						float width = boost->sprite.coords.width / boost->sprite.numFrames * boost->size;
						float height = boost->sprite.coords.height * boost->size;
						Rectangle boostDrawRect = {
							.x = boost->position.x,
							.y = boost->position.y, 
							.width = width,
							.height = height, 
						};
						Vector2 texSize = { boost->collider.width, boost->collider.height };
						Vector2 pivot = { boost->collider.width / 2.0f, boost->collider.height / 2.0f };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// Rectangle destination = {texture_x, texture_y, width, height}; // origin in coordinates and scale
						DrawSpriteAnimationPro(atlas->textureAtlas, atlas->animations[SpriteToAnimation[SPRITE_SCRAPMETAL]], boostDrawRect, pivot, boost->rotation, WHITE, shader);
						// DrawRectangleLines(boostDrawRect.x, boostDrawRect.y, boostDrawRect.width, boostDrawRect.height, RED);
						// DrawRectangleLines(boost->collider.x, boost->collider.y, boost->collider.width, boost->collider.height, GREEN);
					}
				}

				// Draw explosions
				{
					Sprite sprite = getSprite(SPRITE_EXPLOSION);
					atlas->animations[SpriteToAnimation[SPRITE_EXPLOSION]].framesPerSecond = 14;
					float width  = sprite.coords.width / sprite.numFrames;
					float height = sprite.coords.height;
					for (int i = 0; i < gameState->explosionCount; i++)
					{
						Explosion* explosion = &gameState->explosions[i];
						if (!explosion->active) 
						{
							// Replace with last explosion
							*explosion = gameState->explosions[--gameState->explosionCount];
							continue;
						}

						Rectangle dest = {
							explosion->position.x - width/2,
							explosion->position.y - height/2,
							width,
							height
						};

						Vector2 texSize = { width, height };
						SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						bool finished = DrawSpriteAnimationOnce(
								atlas->textureAtlas,
								atlas->animations[SpriteToAnimation[SPRITE_EXPLOSION]],
								dest,
								(Vector2){0, 0},
								0.0f,
								WHITE,
								shader,
								explosion->startTime
								);

						if (finished)
							explosion->active = false;
					}
				}
				// Draw Player
				const int texture_x = gameState->player.playerPosition.x - gameState->player.sprite.coords.width * gameState->player.size / gameState->player.animationFrames / 2.0;
				const int texture_y = gameState->player.playerPosition.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0;
				Rectangle playerDestination = {texture_x, texture_y, 
					gameState->player.sprite.coords.width / gameState->player.animationFrames * gameState->player.size, 
					gameState->player.sprite.coords.height * gameState->player.size}; // origin in coordinates and scale
				Vector2 origin = {0, 0}; // so it draws from top left of image
				if (gameState->player.invulTime <= 0.0f) {
					DrawSpriteAnimationPro(atlas->textureAtlas, atlas->animations[SpriteToAnimation[SPRITE_PLAYER]], playerDestination, origin, 0, WHITE, shader);
				} else {
					if (((int)(gameState->player.invulTime * 10)) % 2 == 0) {
						DrawSpriteAnimationPro(atlas->textureAtlas, atlas->animations[SpriteToAnimation[SPRITE_PLAYER]], playerDestination, origin, 0, WHITE, shader);
					}
				}
				// DrawCircleV(gameState->player.playerPosition, 8.0f, GREEN);
				// DrawRectangleLines(playerDestination.x, playerDestination.y, playerDestination.width, playerDestination.height, BLUE);
				
				// Draw shield
				if (gameState->player.shieldEnabled)
				{
					atlas->animations[SpriteToAnimation[SPRITE_SHIELD]].framesPerSecond = 14;
					Vector2 texSize = { getSprite(SpriteToAnimation[SPRITE_SHIELD]).coords.width / getSprite(SpriteToAnimation[SPRITE_SHIELD]).numFrames, 
										getSprite(SpriteToAnimation[SPRITE_SHIELD]).coords.height };
					SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
					DrawSpriteAnimationPro(atlas->textureAtlas, atlas->animations[SpriteToAnimation[SPRITE_SHIELD]], playerDestination, origin, 0, WHITE, shader);
				}
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
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	int uLightPos = GetShaderLocation(lightShader, "lightPos");
	int uLightRadius = GetShaderLocation(lightShader, "lightRadius");
	int uAspect = GetShaderLocation(lightShader, "aspect");
	float lightRadius = 0.1f;  // normalized radius
	float aspect = (float)viewport.width / (float)viewport.height;
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
		lights[lc].x = gameState->bullets[i].position.x / (float)viewport.width;
		lights[lc].y = 1.0f - gameState->bullets[i].position.y / (float)viewport.height;
		lc++;
	}

	lights[lc].x = gameState->player.playerPosition.x / (float)viewport.width;
	lights[lc].y = 1.0f - gameState->player.playerPosition.y / (float)viewport.height;
	lc++;
	// Upload array
	SetShaderValue(lightShader, uLightCount, &lc, SHADER_UNIFORM_INT);
	SetShaderValueV(lightShader, uLightPos, lights, SHADER_UNIFORM_VEC2, lc);

	EndBlendMode();
	EndTextureMode();
}

void DrawHealthBar(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	// Draw player health
	for (int i = 1; i <= gameState->player.playerHealth; i++)
	{
		const int texture_x = i * 16;
		const int texture_y = getSprite(SPRITE_HEART).coords.height;
		DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
	}
}

void DrawScore(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	Rectangle dst = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
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
}

float EaseOutBack(float t)
{
    // t must be 0 → 1
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;

    float x = t - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

void DrawUpgrades(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	Rectangle dst = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	draw_text_centered(options->font, T(TXT_LEVEL_UP), (Vector2){dst.width/2.0f, dst.height/2.0f - 80.0f}, 40, WHITE);
	draw_text_centered(options->font, T(TXT_CHOOSE_UPGRADE), (Vector2){dst.width/2.0f, dst.height/2.0f - 35.0f}, 40, WHITE);
	float scaling = 3.0f;
	const int width  = getSprite(SPRITE_UPGRADEMULTISHOT).coords.width;
	const int height = getSprite(SPRITE_UPGRADEMULTISHOT).coords.height;
	const int pos_x  = dst.width/2 - width/2;
	const int pos_y  = dst.height/2 - height/2 + 130;
	const int spacing_x = 240;

	const int upgradeToSprite[UPGRADE_COUNT] = {
		[UPGRADE_MULTISHOT] = SPRITE_UPGRADEMULTISHOT,
		[UPGRADE_DAMAGE]    = SPRITE_UPGRADEDAMAGE,
		[UPGRADE_FIRERATE]  = SPRITE_UPGRADEFIRERATE,
	};
	const int upgradeToText[UPGRADE_COUNT] = {
		[UPGRADE_MULTISHOT] = TXT_UPGRADE_MULTISHOT,
		[UPGRADE_DAMAGE]    = TXT_UPGRADE_DAMAGE,
		[UPGRADE_FIRERATE]  = TXT_UPGRADE_FIRERATE,
	};
	char upgradeBuffer[2048] = {0};
	const int fontSize = 4.0f;
	const float rotScal  = 3.0f;
	const float timeScal = 0.1f;
	float pulseScaling = 0.0f;
	float animSpeed = 2.0f;
	float yOff = 0.0f;
	float rotation = 0.0f;

	for (int i = 0; i < UPGRADE_COUNT; i++) {
		Rectangle upgradeRect = 
		{
			.width  = width,
			.height = height,
			.x = pos_x + (i - 1) * spacing_x + width / 2.0f, // i-1 to center the middle upgrade
			.y = pos_y + height / 2.0f,
		};

		float* anim = &gameState->upgradeCards[i].animationTime;

		// Animate scaling and rotation
		if (gameState->pickedUpgrade == i) {
			pulseScaling = 0.10f * sinf(gameState->time * 2.2f);
			rotation = rotScal * cosf(gameState->time / timeScal);
			*anim += gameState->dt * animSpeed;
		} else {
			pulseScaling = 0.0f;
			rotation = 0.0f;
			*anim -= gameState->dt * animSpeed;
		}
		scaling = 3.0f + pulseScaling;
		*anim = Clamp(*anim, 0.0f, 1.0f);
		yOff = -40.0f * EaseOutBack(*anim);
		upgradeRect.y += yOff;

		// Apply scaling
		upgradeRect.width  *= scaling;
		upgradeRect.height *= scaling;

		// Pivot for rotation
		Vector2 pivot = { upgradeRect.width / 2.0f, upgradeRect.height / 2.0f };

		// Store rect for collision detection with mouse
		gameState->upgradeCards[i].rect = (Rectangle){
			.x = upgradeRect.x - pivot.x,
			.y = upgradeRect.y - pivot.y,
			.width = upgradeRect.width,
			.height = upgradeRect.height,
		};
		// Note: Debug draw
		// DrawRectangleLines(gameState->upgradeCards[i].rect.x, gameState->upgradeCards[i].rect.y, gameState->upgradeCards[i].rect.width, gameState->upgradeCards[i].rect.height, RED);

		// Text offsets
		Vector2 cardCenter = { upgradeRect.x + upgradeRect.width*0.5f, upgradeRect.y + upgradeRect.height*0.5f };
		Vector2 textOffset = { 8.0f * scaling, 45.0f * scaling };
		Vector2 textPos    = { upgradeRect.x + textOffset.x, upgradeRect.y + textOffset.y };

		// Draw the upgrade sprite
		DrawTexturePro(atlas->textureAtlas, getSprite(upgradeToSprite[i]).coords, 
				       upgradeRect, pivot, rotation, WHITE);

		// Draw the upgrade text
		DrawTextWrapped(options->font, T(upgradeToText[i]), 
				        upgradeBuffer, 2048,
						(Vector2){textPos.x - textOffset.x, textPos.y - textOffset.y},
						32.0f*scaling, 
						fontSize, 
						ALIGN_LEFT,
						rotation,
						scaling,
						(Vector2){cardCenter.x - textPos.x, cardCenter.y - textPos.y},
						WHITE);

	}
}

void DrawPauseMenu(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	draw_text_centered(options->font, T(TXT_GAME_PAUSED), (Vector2){viewport.width/2.0f, viewport.height/5.0f}, 40, WHITE);
	// Draw a window box
	const float boxWidth = 475.0f;
	const float boxHeight = 350.0f;
	const float boxPosX = options->screenWidth/2;
	const float boxPosY = options->screenHeight/2;
	GuiWindowBox((Rectangle){boxPosX-boxWidth/2,
			                 boxPosY-boxHeight/2, 
							 boxWidth, boxHeight }, T(TXT_SETTINGS));
	// Language label
	const float labelWidth = 160.0f;
	const float labelHeight = 100.0f;
	const float buttonWidth = 100;
	const float buttonHeight = 50;
	const float sliderWidth = 160;
	const float sliderHeight = 20;
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6, 
			labelWidth, labelHeight}, T(TXT_LANGUAGE));
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6+1*boxHeight/12, 
			labelWidth, labelHeight}, T(TXT_MUSICVOLUME));
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6+2*boxHeight/12, 
			labelWidth, labelHeight}, T(TXT_FXVOLUME));
	if (GuiButton((Rectangle){ boxPosX+boxWidth/4-buttonWidth/2, 
							   boxPosY-buttonHeight/4+boxHeight/6, 
							   buttonWidth, buttonHeight }, T(TXT_QUIT))) 
	{
		shouldExit = true;
	}
	// int GuiSliderBar(Rectangle bounds, const char *textLeft, const char *textRight, float *value, float minValue, float maxValue)
	if (!options->languageEditMode) 
	{
		if (GuiSliderBar((Rectangle){ boxPosX-sliderWidth/2+boxWidth/6, 
					boxPosY-sliderHeight/2-boxHeight/6+1*boxHeight/12, 
					sliderWidth, sliderHeight }, "0%", "100%", &options->musicVolume, 0.0f, 0.5f)) 
		{
			options->musicVolumeChanged = true;
		}
		// int GuiSliderBar(Rectangle bounds, const char *textLeft, const char *textRight, float *value, float minValue, float maxValue)
		if (GuiSliderBar((Rectangle){ boxPosX-sliderWidth/2+boxWidth/6, 
					boxPosY-sliderHeight/2-boxHeight/6+2*boxHeight/12, 
					sliderWidth, sliderHeight }, "0%", "100%", &options->fxVolume, 0.0f, 0.5f)) 
		{
			options->fxVolumeChanged = true;
		}
	}
	if (GuiButton((Rectangle){ boxPosX-boxWidth/4-buttonWidth/2, 
							   boxPosY-buttonHeight/4+boxHeight/6, 
							   buttonWidth, buttonHeight }, T(TXT_CONTINUE))) 
	{
		gameState->state = gameState->lastState;
	}
	// Draw dropdown box
	const float dropdownWidth = 160.0f;
	const float dropdownHeight = 20.0f;
	// Note: the label string must list all items separated by ';'
	char langItems[256];
	snprintf(langItems, sizeof(langItems), "%s;%s;%s", T(TXT_ENGLISH), T(TXT_GERMAN), T(TXT_CHINESE));
	if (GuiDropdownBox((Rectangle){ boxPosX-dropdownWidth/2+boxWidth/6, 
									boxPosY-dropdownHeight/2-boxHeight/6,
									dropdownWidth, dropdownHeight}, 
									langItems, &options->language, options->languageEditMode))
	{
		// Toggled edit mode when clicked
		options->languageEditMode = !options->languageEditMode;
	}
	if (options->language != options->lastLanguage)
	{
		options->languageChanged = true;
		switch (options->language)
		{
			case LANG_EN: LocSetLanguage(LANG_EN); break;
			case LANG_DE: LocSetLanguage(LANG_DE); break;
			case LANG_ZH: LocSetLanguage(LANG_ZH); break;
		}
		UnloadFont(options->font);
		options->font = LoadLanguageFont("fonts/UnifontExMono.ttf", options->maxFontSize, options->language);
		GuiSetFont(options->font);
		options->lastLanguage = options->language;
	}
}

void DrawUI(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	switch (gameState->state) {
		case STATE_RUNNING:
			{
				DrawHealthBar(gameState, options, atlas);
				DrawScore(gameState, options, atlas);
				float scale = viewport.width/VIRTUAL_WIDTH;
				const int texture_x = gameState->player.playerPosition.x * scale;
				const int texture_y = (gameState->player.playerPosition.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0) * scale;
				// DrawCircleV(gameState->player.playerPosition, 5.0f, RED);
				// DrawCircleV((Vector2){texture_x, texture_y}, 5.0f, RED);
				if (gameState->player.shieldEnabled)
				{
					char shieldText[100] = {0};
					const float textSize = 18.0f;
					sprintf(shieldText, "%.2f", gameState->player.shieldTime);
					Vector2 position = (Vector2) {texture_x, texture_y};
					position.y -= textSize;
					position.x -= MeasureTextEx(options->font, shieldText, textSize, GetDefaultSpacing(textSize)).x / 2.0f;
					// DrawCircleV(position, 10.0f, WHITE);
					if (gameState->player.shieldTime > 2.0f)
					{
						DrawTextEx(options->font, shieldText, position, textSize, 0, WHITE);
					}
					else
					{
						DrawTextEx(options->font, shieldText, position, textSize, 0, RED);
					}

				}
				break;
			}
		case STATE_MAIN_MENU:
			{
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				draw_text_centered(options->font, T(TXT_GAME_TITLE), (Vector2){viewport.width/2.0f, viewport.height/2.0f}, 40, WHITE);
				draw_text_centered(options->font, T(TXT_PRESS_TO_PLAY), (Vector2){viewport.width/2.0f, viewport.height/2.0f + 30}, 20, WHITE);
				draw_text_centered(options->font, T(TXT_INSTRUCTIONS), (Vector2){viewport.width/2.0f, viewport.height/2.0f + 50}, 20, WHITE);
				draw_text_centered(options->font, "v0.1", (Vector2){viewport.width/2.0f, viewport.height - 15}, 15, WHITE);

				// char testBuffer[2048] = {0};
				// TWrap(testBuffer, 2048, options->font, "This is a test of the text wrapping function.", 50.0, 20.0);
				// draw_text_centered(options->font, testBuffer, (Vector2){viewport.width/2.0f, viewport.height - 100}, 15, WHITE);
				//
				// DrawTextWrapped(options->font, "This is a test of the text wrapping function. This should also be aligned to the center", testBuffer, 2048,
				// 				(Vector2){50,60},
				// 				50.0f,
				// 				15, 
				// 				ALIGN_CENTER,
				// 				WHITE);
				//
				// DrawTextWrapped(options->font, "This is a test of the text wrapping function. This should also be aligned to the right", testBuffer, 2048,
				// 				(Vector2){150,60},
				// 				50.0f,
				// 				20,
				// 				ALIGN_RIGHT,
				// 				WHITE);
				break;
			}
		case STATE_GAME_OVER:
			{
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				draw_text_centered(options->font, T(TXT_GAME_OVER), (Vector2){viewport.width/2.0f, viewport.height/2.0f}, 40, WHITE);
				char scoreText[100] = {0};
				sprintf(scoreText, "Final score: %d", gameState->experience);
				draw_text_centered(options->font, scoreText, (Vector2){viewport.width/2.0f, viewport.height/2.0f + 30.0f}, 20.0f, WHITE);
				draw_text_centered(options->font, T(TXT_TRY_AGAIN), (Vector2){viewport.width/2.0f, viewport.height/2.0f + 60.0f}, 20.0f, WHITE);
				break;
			}
		case STATE_UPGRADE:
			{
				DrawHealthBar(gameState, options, atlas);
				DrawScore(gameState, options, atlas);
				DrawUpgrades(gameState, options, atlas);
				break;
			}
		case STATE_PAUSED:
			{
				DrawHealthBar(gameState, options, atlas);
				DrawScore(gameState, options, atlas);
				if (gameState->lastState == STATE_RUNNING) 
				{
				}
				else if (gameState->lastState == STATE_UPGRADE) 
				{
					DrawUpgrades(gameState, options, atlas);
				}
				DrawPauseMenu(gameState, options, atlas);
				break;
			}
	}
	DrawFPS(10, 40);
}

void DrawComposite(RenderTexture2D* scene, Options* options, RenderTexture2D* litScene, GameState* gameState, Shader lightShader)
{
    Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
    Rectangle src = {
        0, 0,
        (float)scene->texture.width,
        -(float)scene->texture.height
    };

    if (!options->disableShaders) BeginShaderMode(lightShader);
    DrawTexturePro(scene->texture, src, viewport, (Vector2){0, 0}, 0, WHITE);
    if (!options->disableShaders) EndShaderMode();
}

void UpdateDrawFrame()
{
	gameState.dt = GetFrameTime() * gameState.timeScale;
	gameState.time += gameState.dt;
	HandleResize(&options);
	UpdateGame(&gameState, &options, &atlas, spriteMasks, &audio, gameState.dt);
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

	ConfigFlags configFlags = FLAG_WINDOW_RESIZABLE | 
		                      FLAG_VSYNC_HINT | 
							  FLAG_BORDERLESS_WINDOWED_MODE;
							  // FLAG_WINDOW_TOPMOST | 
							  // FLAG_FULLSCREEN_MODE |
							  // FLAG_WINDOW_UNDECORATED;
	SetConfigFlags(configFlags);
	gameState.player.playerPosition = (Vector2){options.screenWidth / 2.0f, options.screenHeight / 2.0f};

	InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, WINDOW_TITLE);
	SetWindowMinSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	
	// Disable Exit on ESC
    SetExitKey(KEY_NULL);

	initializeOptions(&options);
	initializeGameState(&gameState);
	initializeAudio(&audio, &options);

    atlas = initTextureAtlas(spriteMasks);
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
    while (!WindowShouldClose() && !shouldExit)
    { 
		UpdateDrawFrame();
    }
#endif
	// UnloadShader(shader);
	// UnloadShader(lightShader);
	cleanup(atlas, options, audio, spriteMasks);

    CloseWindow();
    return 0;
}

