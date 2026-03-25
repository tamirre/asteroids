#pragma once

#ifdef _WIN32
#define GAME_API __declspec(dllexport)
#else
#endif

#include "assetsData.h"
#include "txt.h"
#include "audio.h"
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

#define MIN_SCREEN_WIDTH (400.0f)
#define MIN_SCREEN_HEIGHT (225.0f)

#if defined(PLATFORM_WEB)
#define VIRTUAL_WIDTH (1440.0f)
#define VIRTUAL_HEIGHT (810.0f)
#else
#define VIRTUAL_WIDTH (1440.0f)
#define VIRTUAL_HEIGHT (810.0f)
#endif

// #define MIN_SCREEN_WIDTH (940.0f)
// #define MIN_SCREEN_HEIGHT (540.0f)
// #define VIRTUAL_WIDTH (940.0f)
// #define VIRTUAL_HEIGHT (540.0f)
#define WINDOW_TITLE ("Asteroids")
#define MAX_BULLETS (1000)
#define MAX_ASTEROIDS (100)
#define MAX_EXPLOSIONS (20)
#define MAX_STARS (50)
#define MAX_BOOSTS (1)
#define MAX_ENEMIES (3)

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
	Rectangle collider;
	int level;
} Player;

typedef struct Enemy {
    float velocity;
    Vector2 position;
    int health;
    Sprite sprite;
    int size;
    int animationFrames;
    float fireRate;
    float shootTime;
    float damageMulti;
	Rectangle collider;
} Enemy;

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
	bool showDebugInfo;
} Options;

typedef struct GameState {
    // General
    int experience;
	int score;
    State state;
	State lastState;
	float timeScale;
    // Player
    Player player;
	Enemy enemies[MAX_ENEMIES];
	int enemyCount;
	float enemySpawnRate;
	float enemySpawnTime;
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
    // Parallax background stars
    Star stars[MAX_STARS];
    int starCount;
    float starTime;
    float starSpawnRate;
    int initStars;
    Upgrade pickedUpgrade;
	float dt;
	float time;
	UpgradeCard upgradeCards[UPGRADE_COUNT];
    bool shouldExit;
    Rectangle currentCollision;
	bool stateChanged;
} GameState;

typedef struct GameMemory
{
    GameState* gameState;
    Options* options;
    Audio* audio;
    TextureAtlas* atlas;
    SpriteMask* spriteMasks;
    RenderTexture2D* scene;
    RenderTexture2D* litScene;
    Shader* shader;
    Shader* lightShader;
} GameMemory;

#if defined(PLATFORM_WINDOWS)
// Function declarations that are exported from the DLL
GAME_API void InitGame(GameMemory* memory);
GAME_API void UpdateDrawFrame(GameMemory* memory);
GAME_API void Cleanup(GameMemory* memory);
GAME_API void InitAudio(Audio* audio, Options* options);
#endif
typedef void (*GameUpdateFn)(GameMemory*);
typedef void (*GameInitFn)(GameMemory*);
typedef void (*GameCleanupFn)(GameMemory*);
typedef void (*GameInitAudioFn)(Audio*, Options*);

