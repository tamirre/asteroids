#include "game.h"
#include <raylib.h>

void Cleanup(GameMemory* gameMemory) 
{
	UnloadShader(*gameMemory->shader);
	UnloadShader(*gameMemory->lightShader);
	UnloadShader(*gameMemory->explosionShader);
	for (int i = 0; i < ANIMATION_COUNT; i++)
	{
		FreeSpriteAnimation(gameMemory->atlas->animations[i]);
	}
    UnloadFont(gameMemory->options->font);
    UnloadFont(gameMemory->options->titleFont);
	for (int i = 0; i < SPRITE_COUNT; i++)
	{
		UnloadImageColors(gameMemory->spriteMasks[i].pixels);
	}
	for (int i = 0; i < MUSIC_COUNT; i++)
	{
		UnloadMusicStream(gameMemory->audio->music[i]);
	}
	for (int i = 0; i < SOUND_COUNT; i++)
	{
		UnloadSound(gameMemory->audio->sounds[i]);
	}
	// De-Initialization of gif recording
    //--------------------------------------------------------------------------------------
    // If still recording a GIF on close window, just finish
	if(gameMemory->gameState->gifRecorder.recording)
	{
		GifRecordStop(&gameMemory->gameState->gifRecorder);
	}
	CloseAudioDevice();
}

void LoopSoundtrack(Music* music) 
{
	// Hard coded loop for now 
	const float sampleRate = 44100;
	if (GetMusicTimePlayed(*music) > 1556879.0f/sampleRate)
	{
		SeekMusicStream(*music, 519288.0f/sampleRate);
	}
	UpdateMusicStream(*music);
}

void SetFxVolume(Audio* audio, float volume) 
{
	for (int i = 0; i < SOUND_COUNT; i++)
	{
		SetSoundVolume(audio->sounds[i], volume);
	}
}

void InitializeAudio(Audio* audio, Options* options) 
{
	InitAudioDevice();
	ASSERT(IsAudioDeviceReady());
	for (int i = 0; i < MUSIC_COUNT; i++)
	{
		audio->music[i] = LoadMusicStream(music_files[i]);
		ASSERT(IsMusicValid(audio->music[i]));
	}
	for (int i = 0; i < SOUND_COUNT; i++)
	{
		audio->sounds[i] = LoadSound(sound_files[i]);
		ASSERT(IsSoundValid(audio->sounds[i]));
	}
	PlayMusicStream(audio->music[audio->currentSongtrackID]);
	SetMusicVolume(audio->music[audio->currentSongtrackID], options->musicVolume);
	SetFxVolume(audio, options->fxVolume);
}

void InitializeGameState(GameState* gameState) 
{
    *gameState = (GameState) {
		.experience = 0,
		.score = 0,
        .state = STATE_MAIN_MENU,
		.lastState = STATE_MAIN_MENU,
		.timeScale = 1.0f,
		.player = {0},
		.enemies = {0},
		.enemyCount = 0,
		.enemySpawnRate = 30.0f,
		.enemySpawnTime = 25.0f,
		.bullets = {0},
        .bulletCount = 0,
		.explosions = {0},
		.explosionCount = 0,
		.asteroids = {0},
        .asteroidCount = 0,
		.spawnTime = 0.0,
        .asteroidSpawnRate = 0.2f,
        .boostCount = 0,
        .boostSpawnTime = 0.0f,
        .boostSpawnRate = 10.0f,
		.stars = {0},
        .starCount = 0,
        .starTime = 0,
        .starSpawnRate = 0.25f,
        .initStars = 0,
        .pickedUpgrade = UPGRADE_MULTISHOT,
		.maxPlayerBullets = 7,
		.dt = 0.0f,
		.time = 0.0f,
		.upgradeCards = {0},
		.shouldExit = false,
		.currentCollision = {0},
		.stateChanged = true,
    };

	MsfGifState gifState = { 0 };
	gameState->gifRecorder = (GifRecorder){ 
		.gifState = &gifState,
		.recording = false,
		.frameCounter = 0,
	};

	for (int i = 0; i < UPGRADE_COUNT; i++)
	{
		gameState->upgradeCards[i].animationTime = 0.0f;
		gameState->upgradeCards[i].rect = (Rectangle){0};
	}

    gameState->player = (Player) {
        .velocity = 350,
        .position = (Vector2){VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f},
        .health = 7,
        .bulletCount = 1,
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
		.level = 1,
		.collider = (Rectangle){0,0,0,0},
    };
}

void InitializeOptions(Options* options) 
{
	const int maxFontSize = 64;
	const int maxTitleFontSize = 100;
	*options = (Options) {
		.screenWidth = (float)VIRTUAL_WIDTH,
		.screenHeight = (float)VIRTUAL_HEIGHT,
		.previousWidth = (float)VIRTUAL_WIDTH,
		.previousHeight = (float)VIRTUAL_HEIGHT,
		.disableShaders = true,
		.font = LoadLanguageFont("./assets/fonts/UnifontExMono.ttf", maxFontSize, LANG_EN), 
		.titleFont = LoadLanguageFont("./assets/fonts/Ethnocentric-Regular.otf", maxTitleFontSize, LANG_EN), 
		.fontSpacing = 1.0f,
		.maxFontSize = maxFontSize,
		.language = LANG_EN,
		.lastLanguage = LANG_EN,
		.languageEditMode = false,
		.languageChanged = false,
		.disableCursor = false,
		.lastMousePos = (Vector2){0,0},
		.musicVolume = 0.15f,
		.fxVolume = 0.15f,
		.musicVolumeChanged = false,
		.fxVolumeChanged = false,
		.showDebugInfo = false,
	};
	SetTextureFilter(options->font.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(options->titleFont.texture, TEXTURE_FILTER_BILINEAR);
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

void InitGame(GameMemory* gameMemory)
{
    SetTargetFPS(TARGET_FPS);

	// set default language
	LocSetLanguage(LANG_EN);
	// LocSetLanguage(LANG_DE);
	// LocSetLanguage(LANG_ZH);

	ConfigFlags configFlags = FLAG_WINDOW_RESIZABLE | 
		                      FLAG_VSYNC_HINT | 
							  // FLAG_WINDOW_TOPMOST | 
							  // FLAG_FULLSCREEN_MODE |
							  // FLAG_WINDOW_UNDECORATED |
							  FLAG_BORDERLESS_WINDOWED_MODE;
	SetConfigFlags(configFlags);

	InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, WINDOW_TITLE);
	SetWindowMinSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	
	// Disable Exit on ESC
    SetExitKey(KEY_NULL);

	// Options options = {0};
	GameState gameState = {0};
	Audio audio = {0};
	InitializeOptions(gameMemory->options);
	InitializeGameState(gameMemory->gameState);
	InitializeAudio(gameMemory->audio, gameMemory->options);
	// gameMemory->options = &options;
	RenderTexture2D scene = LoadRenderTexture(gameMemory->options->screenWidth, gameMemory->options->screenHeight);
	RenderTexture2D litScene = LoadRenderTexture(gameMemory->options->screenWidth, gameMemory->options->screenHeight);
	SpriteMask spriteMasks[SPRITE_COUNT];
    TextureAtlas atlas = initTextureAtlas(spriteMasks);
	gameMemory->scene = &scene;
	gameMemory->litScene = &litScene;

	// Write the initialized state to gameMemory
	gameMemory->atlas = &atlas;
	gameMemory->spriteMasks = spriteMasks;
	gameMemory->options->previousWidth  = VIRTUAL_WIDTH;
	gameMemory->options->previousHeight = VIRTUAL_HEIGHT;
#ifndef PLATFORM_WEB
	*gameMemory->shader = LoadShader(0, TextFormat("./src/shaders/default.glsl", GLSL_VERSION));
	*gameMemory->lightShader = LoadShader(0, TextFormat("./src/shaders/light.fs", GLSL_VERSION));
	*gameMemory->explosionShader = LoadShader(0, TextFormat("./src/shaders/explode.glsl", GLSL_VERSION));
#endif
	printf("InitGame done!\n");
}

void loadSaveState(GameMemory* gameMemory)
{
	FILE* file = fopen("save.dat", "rb");
	if (file == NULL)
	{
		printf("Error: could not open savestate file\n");
		return;
	}
	fread(gameMemory->gameState, sizeof(GameState), 1, file);
	fclose(file);
}

void writeSaveState(GameMemory* gameMemory)
{
	FILE* file = fopen("save.dat", "wb");
	if (file == NULL)
	{
		printf("Error opening file\n");
		return;
	}
	fwrite(gameMemory->gameState, sizeof(GameState), 1, file);
	fclose(file);
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

void UpdateGame(GameMemory* gameMemory)
{
	GameState* gameState = gameMemory->gameState;
	Options* options = gameMemory->options;
	TextureAtlas* atlas = gameMemory->atlas;
	SpriteMask* spriteMasks = gameMemory->spriteMasks;
	// SpriteMask spriteMasks[SPRITE_COUNT] = gameMemory->spriteMasks;
	Audio* audio = gameMemory->audio;

	// static bool cursorHidden = true;
	static bool stepMode = false;
	static bool stepOnce = false;
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());

	// gameState->stateChanged = false;
	switch (gameState->state) 
	{
		case STATE_MAIN_MENU:
			{
				if (IsKeyPressed(KEY_ENTER)) {                    
					gameState->state = STATE_RUNNING;
					gameState->stateChanged = true;
				}
				break;
			}
		case STATE_RUNNING:
			{
				float viewportScale = viewport.width / VIRTUAL_WIDTH;
				const Rectangle screenRect = {
					.height = VIRTUAL_HEIGHT,
					.width = VIRTUAL_WIDTH,
					.x = 0,
					.y = 0
				};
				if (!IsMusicStreamPlaying(audio->music[audio->currentSongtrackID])) 
				{
					ResumeMusicStream(audio->music[audio->currentSongtrackID]);
				}
				LoopSoundtrack(&audio->music[audio->currentSongtrackID]);
				ResumeSound(audio->sounds[SOUND_SHIELD]);
				// Input keys
				{
					// Step debugging mode
					if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
						gameState->state = STATE_PAUSED;
						gameState->lastState = STATE_RUNNING;
						gameState->stateChanged = true;
					}
					if (IsKeyPressed(KEY_J)) stepMode = !stepMode;
					if (IsKeyPressed(KEY_K)) stepOnce = true;
					if (IsKeyPressed(KEY_H))
					{
						gameState->timeScale -= 0.1f;
					}
					if (IsKeyPressed(KEY_L)) 
					{
						gameState->timeScale += 0.1f;
					}
					if (IsKeyPressed(KEY_O)) options->disableShaders = !options->disableShaders;
#ifndef PLATFORM_WEB
					if (IsKeyPressed(KEY_F)) 
					{
						ToggleFullscreen();
					}
					if (IsKeyPressed(KEY_F2)) 
					{
						writeSaveState(gameMemory);
					}
					if (IsKeyPressed(KEY_T)) 
					{
						loadSaveState(gameMemory);
					}
					if (IsKeyPressed(KEY_TAB)) // press Tab to toggle cursor
					{
						// if (cursorHidden) EnableCursor();
						// else DisableCursor();
						// cursorHidden = !cursorHidden;
					}
					if (IsKeyPressed(KEY_V)) {
						if (IsWindowState(FLAG_VSYNC_HINT))
						{
							ClearWindowState(FLAG_VSYNC_HINT);
						} else {
							SetWindowState(FLAG_VSYNC_HINT);
						}
					}
					// Start-Stop GIF recording on CTRL+R
					if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R))
					{
						if (gameState->gifRecorder.recording)
						{
							GifRecordStop(&gameState->gifRecorder);
						}
						else
						{
							GifRecordStart(&gameState->gifRecorder);
						}
					}
					// Take Screenshot
					if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_M))
					{
						ScreenShot();
					}
#endif
					if (stepMode && !stepOnce) return;
					stepOnce = false;
				}
				// Update score
				{
					const float requiredExperience = 500.0;
					if (gameState->experience > requiredExperience * gameState->player.level * 1.5f)
					{
						gameState->state = STATE_UPGRADE;
						gameState->stateChanged = true;
						gameState->experience -= requiredExperience * gameState->player.level * 1.5f;
						gameState->player.level++;
					}
				}
				// Spawn stars for parallax
				{
					if (gameState->initStars == 0)
					{
						while(gameState->starCount < MAX_STARS)
						{                            
							int imgIndex = GetRandomValue(1, 2);   
							// float starXPosition = GetRandomValue(0, options->screenWidth); 
							// float starYPosition = GetRandomValue(0, options->screenHeight); 
							float starXPosition = GetRandomValue(0, VIRTUAL_WIDTH);
							float starYPosition = GetRandomValue(0, VIRTUAL_HEIGHT); 
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
					gameState->starTime += gameState->dt;
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
							float starXPosition = GetRandomValue(0, VIRTUAL_WIDTH);
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
				// Update stars
                {
                    for (int starIndex = 0; starIndex < gameState->starCount; starIndex++)
                    {
                        Star* star = &gameState->stars[starIndex];
                        star->position.y += star->velocity * gameState->dt;
                        if(!CheckCollisionPointRec(star->position, screenRect))
						{
							// Replace with last star
							if (gameState->starCount > 0)
							{
								*star = gameState->stars[--gameState->starCount];
							} else {
								gameState->starCount = 0;
							}
						}
                    }
                }
				// Update Player
				// Player movement
				{
					if (IsKeyDown(KEY_W)) {
						if(!CheckCollisionPointLine((Vector2) {gameState->player.position.x * viewportScale, gameState->player.position.y * viewportScale},
									(Vector2) {0, 0}, 
									(Vector2) {viewport.width, 0},
									gameState->player.sprite.coords.height * viewportScale))
						{
							gameState->player.position.y -= gameState->player.velocity * gameState->dt;
						}   
					}
					if (IsKeyDown(KEY_S)) {
						if(!CheckCollisionPointLine((Vector2) {gameState->player.position.x * viewportScale, gameState->player.position.y * viewportScale},
									(Vector2) {0, viewport.height},
									(Vector2) {viewport.width, viewport.height},
									gameState->player.sprite.coords.height * viewportScale))
						{
							gameState->player.position.y += gameState->player.velocity * gameState->dt;
						}
					}
					if (IsKeyDown(KEY_A)) {
						if(!CheckCollisionPointLine((Vector2) {gameState->player.position.x * viewportScale, gameState->player.position.y * viewportScale},
									(Vector2) {0, 0}, 
									(Vector2) {0, viewport.height},
									gameState->player.sprite.coords.width / gameState->player.animationFrames * viewportScale))
						{
							gameState->player.position.x -= gameState->player.velocity * gameState->dt;
						}
					}
					if (IsKeyDown(KEY_D)) {
						if(!CheckCollisionPointLine((Vector2) {gameState->player.position.x * viewportScale, gameState->player.position.y * viewportScale},
									(Vector2) {viewport.width, 0},
									(Vector2) {viewport.width, viewport.height},
									gameState->player.sprite.coords.width / gameState->player.animationFrames * viewportScale))
						{
							gameState->player.position.x += gameState->player.velocity * gameState->dt;
						}
					}

					if (gameState->player.invulTime < 0.0f) 
					{
						gameState->player.invulTime = 0.0f;
					} 
					else 
					{
						gameState->player.invulTime -= gameState->dt;
					}
					if (gameState->player.shootTime < 1.0f/gameState->player.fireRate)
					{
						gameState->player.shootTime += gameState->dt;
					} 
					if (gameState->player.shieldEnabled && gameState->player.shieldTime > 0.0f)
					{
						gameState->player.shieldTime -= gameState->dt;
					}
					if (gameState->player.shieldTime < 0.0f)
					{
						gameState->player.shieldEnabled = false;
						gameState->player.shieldTime = 5.25f;
					}
					const float playerWidth = gameState->player.sprite.coords.width/gameState->player.animationFrames * gameState->player.size;
					const float playerHeight = gameState->player.sprite.coords.height * gameState->player.size;
					gameState->player.collider = (Rectangle) {
						.width = playerWidth,
							.height =  playerHeight,
							.x = gameState->player.position.x - playerWidth/2.0f,
							.y = gameState->player.position.y - playerHeight/2.0f,
					};
				}
				// Shoot bullets (player)
				if (IsKeyDown(KEY_SPACE) 
					&& gameState->player.shootTime >= 1.0f/gameState->player.fireRate
					&& gameState->bulletCount <= MAX_BULLETS)
				{
					const float random = (1.0f - (float)GetRandomValue(0, 2))/10.0f;
					SetSoundPitch(audio->sounds[SOUND_GUN], 1.0f + random);
					PlaySound(audio->sounds[SOUND_GUN]);
					if (gameState->player.bulletCount > 0 && gameState->bulletCount < MAX_BULLETS - gameState->player.bulletCount)
					{
						int count = gameState->player.bulletCount;
						float spreadAngle = 30.0f; // total spread in degrees 
						float startAngle = -spreadAngle / 2.0f;
						float bulletSize = gameState->player.damageMulti/2.0f;
						for (int i = 0; i < count; i++)
						{
							float t = (count == 1) ? 0.5f : (float)i / (count - 1);
							float angleDeg = startAngle + t * spreadAngle;
							float angleRad = -angleDeg * DEG2RAD;

							float speed = 500.0f;

							Vector2 velocity = {
								sinf(angleRad) * speed,
								cosf(angleRad) * speed
							};

							Bullet bullet = {
								.position = gameState->player.position,
								.velocity = velocity,
								.damage = 1.5f * gameState->player.damageMulti,
								.sprite = getSprite(SPRITE_BULLET),
								.rotation = angleDeg,
								.size = bulletSize * 0.75f,
								.owner = &gameState->player,
							};

							// Adjust Y so bullet spawns at top of player
							bullet.position.y -= gameState->player.sprite.coords.height * gameState->player.size / 2.0f
								               - bullet.sprite.coords.height * bullet.size / 2.0f;

							gameState->bullets[gameState->bulletCount++] = bullet;
						}

						gameState->player.shootTime -= 1.0f / gameState->player.fireRate;
					}
				}
				// Spawn enemies
				{
					gameState->enemySpawnTime += gameState->dt;
					if (gameState->enemySpawnTime > gameState->enemySpawnRate && gameState->enemyCount < MAX_ENEMIES) 
					{
						// printf("Spawning enemy\n");
						float size = 2.0;
						Vector2 velocity = (Vector2){0.1f + (float)GetRandomValue(3, 10)/20.0f,0};
						float enemyXPosition = GetRandomValue(0            +getSprite(SPRITE_ENEMY).coords.width/2.0f, 
								                              VIRTUAL_WIDTH-getSprite(SPRITE_ENEMY).coords.width/2.0f);
						Enemy enemy =
						{
							.velocity = velocity,
							.position = (Vector2){enemyXPosition, 70},
							.phase = 0.0f,
							.health = 20,
							.size = size,
							.sprite = getSprite(SPRITE_ENEMY),
							.shootTime = 0.0f,
							.fireRate = 1.0f,
							.bulletCount = 3,
						};
						enemy.collider = (Rectangle){
							.x = enemy.position.x - enemy.sprite.coords.width*enemy.size/2.0f,
							.y = enemy.position.y - enemy.sprite.coords.height*enemy.size/2.0f,
							.width = enemy.sprite.coords.width*enemy.size,
							.height = enemy.sprite.coords.height*enemy.size,
						};
						gameState->enemies[gameState->enemyCount++] = enemy;
						gameState->enemySpawnTime = 0.0f;
					}
				}
				// Update enemies
				{
					for (int enemyIndex = 0; enemyIndex < gameState->enemyCount; enemyIndex++)
					{
						Enemy* enemy = &gameState->enemies[enemyIndex];
						enemy->phase += enemy->velocity.x * gameState->dt;
						enemy->position.x = 0.5 * enemy->sprite.coords.width*enemy->size + (VIRTUAL_WIDTH - enemy->sprite.coords.width*enemy->size) * 0.5f * (1.0f + sinf(enemy->phase));
						enemy->collider = (Rectangle){
							.x = enemy->position.x - enemy->sprite.coords.width*enemy->size/2.0f,
							.y = enemy->position.y - enemy->sprite.coords.height*enemy->size/2.0f,
							.width = enemy->sprite.coords.width*enemy->size,
							.height = enemy->sprite.coords.height*enemy->size,
						};
					}
					for (int enemyIndex = 0; enemyIndex < gameState->enemyCount; enemyIndex++)
					{
						Enemy* enemy = &gameState->enemies[enemyIndex];
						// Collision enemy bullet
						for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
						{
							Bullet* bullet = &gameState->bullets[bulletIndex];
							if(bullet->owner == &gameState->player)
							{
								if(CheckCollisionRecs(enemy->collider, bullet->collider))
								{
									Rectangle collisionRec = GetCollisionRec(enemy->collider, bullet->collider);
									Rectangle bulletSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_BULLET]]);
									if (pixelPerfectCollision(spriteMasks[SPRITE_BULLET].pixels, spriteMasks[enemy->sprite.spriteID].pixels, 
												bulletSrc.width, enemy->sprite.coords.width,
												bulletSrc.height, enemy->sprite.coords.height,
												bullet->collider, enemy->collider, collisionRec, bullet->rotation, 0))
									{

										Explosion* explosion = NULL;
										if (gameState->explosionCount < MAX_EXPLOSIONS)
										{
											PlaySound(audio->sounds[SOUND_EXPLOSIONBLAST]);
											explosion = &gameState->explosions[gameState->explosionCount++];
											explosion->position = bullet->position;
											explosion->position.y -= bulletSrc.height/2.0f;
											explosion->velocity = enemy->velocity;
											explosion->startTime = GetTime();
											explosion->active = true;
										}
										enemy->health -= bullet->damage;
										// Replace bullet with last bullet
										if (gameState->bulletCount > 0)
										{
											*bullet = gameState->bullets[--gameState->bulletCount];
										} else {
											gameState->bulletCount = 0;
										}
										if (enemy->health <= 0.0f)
										{
											gameState->experience += 200;
											gameState->score += 20 * MAX((int)(enemy->size * 100),1);
											if (gameState->enemyCount > 0)
											{
												*enemy = gameState->enemies[--gameState->enemyCount];
											} else {
												gameState->enemyCount = 0;
											}
											if(explosion != NULL)
											{
												explosion->velocity.x = 0.0f;
											}
										}
									}
								}
							}
						}
					}

					for (int enemyIndex = 0; enemyIndex < gameState->enemyCount; enemyIndex++)
					{
						Enemy* enemy = &gameState->enemies[enemyIndex];
						// Shoot bullets (enemies)
						enemy->shootTime += gameState->dt;
						if (enemy->shootTime >= 1.0f/enemy->fireRate && gameState->bulletCount <= MAX_BULLETS)
						{
							const float random = (1.0f - (float)GetRandomValue(0, 2))/10.0f;
							SetSoundPitch(audio->sounds[SOUND_GUN], 1.0f + random);
							PlaySound(audio->sounds[SOUND_GUN]);
							float bulletSize = 0.5f;
							if (enemy->bulletCount > 0 && gameState->bulletCount < MAX_BULLETS - enemy->bulletCount)
							{
								int count = enemy->bulletCount;
								float spreadAngle = 30.0f; // total spread in degrees (adjust as needed)
								float startAngle = -spreadAngle / 2.0f;
								for (int i = 0; i < count; i++)
								{
									float t = (count == 1) ? 0.5f : (float)i / (count - 1);
									float angleDeg = startAngle + t * spreadAngle;
									float angleRad = angleDeg * DEG2RAD;
									float speed = 500.0f;

									Vector2 velocity = { sinf(angleRad) * speed, -cosf(angleRad) * speed };

									Bullet bullet = {
										.position = enemy->position,
										.velocity = velocity,
										.damage = 1.0f * enemy->damageMulti,
										.sprite = getSprite(SPRITE_BULLET),
										.rotation = angleDeg,
										.size = bulletSize,
										.owner = &enemy,
									};

									// Adjust Y so bullet spawns at top of player
									bullet.position.y -= enemy->sprite.coords.height * enemy->size / 2.0f
									                   - bullet.sprite.coords.height * bullet.size / 2.0f;

									gameState->bullets[gameState->bulletCount++] = bullet;
								}
								enemy->shootTime -= 1.0f / enemy->fireRate;
							}
						}
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
							if (gameState->bulletCount > 0)
							{
								*bullet = gameState->bullets[--gameState->bulletCount];
							} else {
								gameState->bulletCount = 0;
							}
						}
                        bullet->position.x -= bullet->velocity.x * gameState->dt;
                        bullet->position.y -= bullet->velocity.y * gameState->dt;
						const int texture_x = bullet->position.x - bullet->sprite.coords.width * bullet->size / getSprite(SPRITE_BULLET).numFrames / 2.0;
						const int texture_y = bullet->position.y - bullet->sprite.coords.height * bullet->size / 2.0;
						const float width = bullet->sprite.coords.width / getSprite(SPRITE_BULLET).numFrames * bullet->size;
						const float height = bullet->sprite.coords.height * bullet->size;
						bullet->collider = (Rectangle) {
							.width = width,
								.height = height,
								.x = texture_x,
								.y = texture_y,
						};
					}
				}
				// Collision player bullet
				for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
				{
					Bullet* bullet = &gameState->bullets[bulletIndex];
					if(CheckCollisionRecs(gameState->player.collider, bullet->collider) 
						&& bullet->owner != &gameState->player 
						&& gameState->player.invulTime <= 0.0f 
						&& gameState->player.shieldEnabled == false)
					{
						Rectangle collisionRec = GetCollisionRec(gameState->player.collider, bullet->collider);
						gameState->currentCollision = collisionRec;
						Rectangle bulletSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_BULLET]]);
						Rectangle playerSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_PLAYER]]);
						if (pixelPerfectCollision(spriteMasks[SPRITE_BULLET].pixels, spriteMasks[SPRITE_PLAYER].pixels, 
									bulletSrc.width, playerSrc.width,
									bulletSrc.height, playerSrc.height,
									bullet->collider, gameState->player.collider, collisionRec, bullet->rotation, 0))
						{

							Explosion* explosion = NULL;
							if (gameState->explosionCount < MAX_EXPLOSIONS)
							{
								PlaySound(audio->sounds[SOUND_EXPLOSIONBLAST]);
								explosion = &gameState->explosions[gameState->explosionCount++];
								explosion->position = bullet->position;
								explosion->position.y -= bulletSrc.height/2.0f;
								explosion->velocity.y = gameState->player.velocity;
								explosion->startTime = GetTime();
								explosion->active = true;
							}
							PlaySound(audio->sounds[SOUND_HIT]);
							// Replace with bullet with last bullet
							if (gameState->bulletCount > 0)
							{
								*bullet = gameState->bullets[--gameState->bulletCount];
							} else {
								gameState->bulletCount = 0;
							}
							gameState->player.invulTime = gameState->player.invulDuration;
							if (--gameState->player.health < 1)
							{
								gameState->state = STATE_GAME_OVER;
								gameState->stateChanged = true;
							}
						}
					}
				}
				// Spawn Asteroids
				{
					gameState->spawnTime += gameState->dt;
					if (gameState->spawnTime > gameState->asteroidSpawnRate && gameState->asteroidCount < MAX_ASTEROIDS) 
					{
						float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
						float asteroidXPosition = GetRandomValue(0, VIRTUAL_WIDTH);
						Asteroid asteroid =
						{
							.position = (Vector2) {asteroidXPosition, 0},
							.health = (size + 1.0),
							.velocity = (Vector2) {0, GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size},
							.angularVelocity = GetRandomValue(-40.0f, 40.0f),
							.size = size,
							.dying = false,
							.deathTime = 0.0f,
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
						if (!asteroid->dying) {
							asteroid->position.y += asteroid->velocity.y * gameState->dt;
							asteroid->rotation   += asteroid->angularVelocity * gameState->dt;
						} else {
							asteroid->position.y -= 20.0 * gameState->dt;
						}

						float width  = asteroid->sprite.coords.width * asteroid->size;
						float height = asteroid->sprite.coords.height * asteroid->size;
						asteroid->collider = (Rectangle) {
							.x = asteroid->position.x - width / 2.0f,
							.y = asteroid->position.y - height / 2.0f, 
							.width  = width,
							.height = height, 
						};
						if (asteroid->dying) {
							// printf("dying: %f\n", asteroid->deathTime);
							asteroid->deathTime += gameState->dt;
						}
						if (asteroid->deathTime > 0.5f) {
							// printf("dying done: %f\n", asteroid->deathTime);
							asteroid->dying = false;
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
					}
				}
				// Collision asteroid bullet
				{
					for (int asteroidIndex = 0; asteroidIndex < gameState->asteroidCount; asteroidIndex++)
					{
						Asteroid* asteroid = &gameState->asteroids[asteroidIndex];
						for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
						{
							Bullet* bullet = &gameState->bullets[bulletIndex];
							if(bullet->owner == &gameState->player && !asteroid->dying)
							{
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
											PlaySound(audio->sounds[SOUND_EXPLOSIONBLAST]);
											explosion = &gameState->explosions[gameState->explosionCount++];
											explosion->position = bullet->position;
											explosion->position.y -= bulletSrc.height/2.0f;
											explosion->velocity = asteroid->velocity;
											explosion->startTime = GetTime();
											explosion->active = true;
										}
										asteroid->health -= bullet->damage;
										// Replace with last bullet
										*bullet = gameState->bullets[--gameState->bulletCount];

										if (asteroid->health <= 0.0f && !asteroid->dying) {
											asteroid->dying = true;
											asteroid->deathTime = 0.0f;
											gameState->experience += MAX((int)(asteroid->size * 100),1);
											gameState->score += MAX((int)(asteroid->size * 100),1);
											if(explosion != NULL)
											{
												explosion->velocity.y = 0.0f;
											}
										}

										
										// if (asteroid->health <= 0.0)
										// {
										// 	gameState->experience += MAX((int)(asteroid->size * 100),1);
										// 	gameState->score += MAX((int)(asteroid->size * 100),1);
										// 	*asteroid = gameState->asteroids[--gameState->asteroidCount];
										// 	if(explosion != NULL)
										// 	{
										// 		explosion->velocity.y = 0.0f;
										// 	}
										// }
									}
								}
							}
						}

						// Collision asteroid player
						if(CheckCollisionRecs(asteroid->collider, gameState->player.collider) && 
								gameState->player.invulTime <= 0.0f &&
								gameState->player.shieldEnabled == false &&
								!asteroid->dying)
						{
							Rectangle collisionRec = GetCollisionRec(asteroid->collider, gameState->player.collider);
							gameState->currentCollision = collisionRec;
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_PLAYER]]);
							if (pixelPerfectCollision(spriteMasks[SPRITE_PLAYER].pixels, spriteMasks[asteroid->sprite.spriteID].pixels, 
										playerSrc.width, asteroid->sprite.coords.width,
										playerSrc.height, asteroid->sprite.coords.height, 
										gameState->player.collider, asteroid->collider, collisionRec, 0.0f, asteroid->rotation))
							{
								PlaySound(audio->sounds[SOUND_HIT]);
								gameState->player.invulTime = gameState->player.invulDuration;
								*asteroid = gameState->asteroids[--gameState->asteroidCount];
								gameState->currentCollision = (Rectangle){0,0,0,0};
								if(--gameState->player.health < 1) 
								{
									gameState->state = STATE_GAME_OVER;
									gameState->stateChanged = true;
								}
							}
						} 
						// Check if asteroid is off-screen
						if (asteroid->position.y > VIRTUAL_HEIGHT + asteroid->sprite.coords.height * asteroid->size)
						{
							*asteroid = gameState->asteroids[--gameState->asteroidCount];
						}
					}
				}
				// Update explosions
				{
					for (int explosionIndex = 0; explosionIndex < gameState->explosionCount; explosionIndex++)
					{
						Explosion* explosion = &gameState->explosions[explosionIndex];
						if (explosion->active)
						{
							explosion->position.x += explosion->velocity.x * gameState->dt;
							explosion->position.y += explosion->velocity.y * gameState->dt;
						}
					}
				}
				// Spawn boosts
				{
					if (gameState->boostCount < MAX_BOOSTS)
					{
						gameState->boostSpawnTime += gameState->dt;
					}
					if (gameState->boostSpawnTime > gameState->boostSpawnRate && gameState->boostCount < MAX_BOOSTS)
					{
						// int size = (int)GetRandomValue(1, 3);
						float size = GetRandomValue(50.0f, 200.0f) / 100.0f;
						float minSpawnDistance = 50.0f * size;  
						float boostXPosition = MAX(minSpawnDistance, GetRandomValue(0, VIRTUAL_WIDTH));
						Boost boost = {
							.position = (Vector2) {boostXPosition, 0},
							.velocity = (Vector2) {0, GetRandomValue(30.0f, 65.0f) * 5.0f / (float)size},
							.angularVelocity = 0.0f,
							.size = 2.0f,
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
						boost->position.y += boost->velocity.y * gameState->dt;
						boost->rotation += boost->angularVelocity * gameState->dt;
						float width  = boost->sprite.coords.width / boost->sprite.numFrames * boost->size;
						float height = boost->sprite.coords.height * boost->size;
						boost->collider = (Rectangle) {
							.x = boost->position.x - width / 2.0f,
							.y = boost->position.y - height / 2.0f,
							.width = width,
							.height = height, 
						};
						// Check if boost is off-screen
						if (boost->position.y > VIRTUAL_HEIGHT + boost->sprite.coords.height * boost->size)
						{
							*boost = gameState->boosts[--gameState->boostCount];
							gameState->boostSpawnTime = 0.0f;
						}
						// Collision boost player
						if(CheckCollisionRecs(boost->collider, gameState->player.collider))
						{
							Rectangle collisionRec = GetCollisionRec(boost->collider, gameState->player.collider);
							gameState->currentCollision = collisionRec;
							Rectangle playerSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_PLAYER]]);
							Rectangle boostSrc = GetCurrentAnimationFrame(atlas->animations[SpriteToAnimation[SPRITE_SCRAPMETAL]]);
							if (pixelPerfectCollision(spriteMasks[SPRITE_PLAYER].pixels, spriteMasks[boost->sprite.spriteID].pixels, 
										playerSrc.width, boostSrc.width,
										playerSrc.height, boostSrc.height,
										gameState->player.collider, boost->collider, collisionRec, 0.0f, boost->rotation))
							{
								*boost = gameState->boosts[--gameState->boostCount];
								gameState->player.shieldEnabled = true;
								gameState->player.shieldTime = 5.25f;
								if(IsSoundPlaying(audio->sounds[SOUND_SHIELD]))
								{
									StopSound(audio->sounds[SOUND_SHIELD]);
								}
								PlaySound(audio->sounds[SOUND_SHIELD]);
								gameState->currentCollision = (Rectangle){0,0,0,0};
							}
						} 
					}
				}
				break;
			}
		case STATE_UPGRADE:
			{
				LoopSoundtrack(&audio->music[audio->currentSongtrackID]);
				if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
					gameState->state = STATE_PAUSED;
					gameState->lastState = STATE_UPGRADE;
					gameState->stateChanged = true;
				}
				if (IsSoundPlaying(audio->sounds[SOUND_SHIELD]))
				{
					StopSound(audio->sounds[SOUND_SHIELD]);
				}

				Vector2 mousePos = GetMousePosition();
				bool clickedUpgrade = false;
				int lastUpgrade = gameState->pickedUpgrade; 
				if (IsKeyPressed(KEY_LEFT)) 
				{
					PlaySound(audio->sounds[SOUND_CARDSELECT]);
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade - 1 + UPGRADE_COUNT) % UPGRADE_COUNT);
				} 
				else if (IsKeyPressed(KEY_RIGHT)) 
				{
					PlaySound(audio->sounds[SOUND_CARDSELECT]);
					gameState->pickedUpgrade = (Upgrade)((gameState->pickedUpgrade + 1) % UPGRADE_COUNT);
				}
				if (mousePos.x == options->lastMousePos.x && mousePos.y == options->lastMousePos.y)
				{
					options->disableCursor = true;
				} 
				else 
				{
					options->disableCursor = false;
				}
				options->lastMousePos = mousePos;
				int mouseOverUpgrade = -1;
				for (int i = 0; i < UPGRADE_COUNT; i++) 
				{
					if (CheckCollisionPointRec(mousePos, gameState->upgradeCards[i].rect))
					{
						mouseOverUpgrade = i;
						if (!options->disableCursor) {
							gameState->pickedUpgrade = i; 
							if (lastUpgrade != gameState->pickedUpgrade) 
							{
								PlaySound(audio->sounds[SOUND_CARDSELECT]);
							}
						}
						// break;
					}
				}
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouseOverUpgrade != -1)
				{
					gameState->pickedUpgrade = mouseOverUpgrade;
					clickedUpgrade = true;
					// printf("clickedUpgrade: %d\n", clickedUpgrade);
					// printf("selectedUpgrade: %d\n", gameState->pickedUpgrade);
				}
				// printf("clickedUpgrade: %d\n", clickedUpgrade);
				if (IsKeyPressed(KEY_ENTER) || clickedUpgrade) {
					if (gameState->pickedUpgrade == UPGRADE_MULTISHOT) {
						gameState->player.bulletCount += 2;
					} else if (gameState->pickedUpgrade == UPGRADE_DAMAGE) {
						gameState->player.damageMulti += 0.5f;
					} else if (gameState->pickedUpgrade == UPGRADE_FIRERATE) {
						gameState->player.fireRate += 0.5f;
					}
					gameState->state = STATE_RUNNING;
					gameState->stateChanged = true;
				}
				break;
			}
		case STATE_GAME_OVER:
			{
				LoopSoundtrack(&audio->music[audio->currentSongtrackID]);
				if (IsKeyPressed(KEY_ENTER)) {
#ifdef PLATFORM_WEB
					GameState gameState = {0};
					Audio audio = {0};
					InitializeOptions(gameMemory->options);
					InitializeGameState(gameMemory->gameState);
					CloseAudioDevice();
					InitializeAudio(gameMemory->audio, gameMemory->options);
					// gameMemory->options = &options;
					RenderTexture2D scene = LoadRenderTexture(gameMemory->options->screenWidth, gameMemory->options->screenHeight);
					RenderTexture2D litScene = LoadRenderTexture(gameMemory->options->screenWidth, gameMemory->options->screenHeight);
					SpriteMask spriteMasks[SPRITE_COUNT];
					TextureAtlas atlas = initTextureAtlas(spriteMasks);
					gameMemory->scene = &scene;
					gameMemory->litScene = &litScene;

					// Write the initialized state to gameMemory
					gameMemory->atlas = &atlas;
					gameMemory->spriteMasks = spriteMasks;
					gameMemory->options->previousWidth  = VIRTUAL_WIDTH;
					gameMemory->options->previousHeight = VIRTUAL_HEIGHT;
					*gameMemory->shader = LoadShader(0, TextFormat("./src/shaders/default_web.glsl", GLSL_VERSION));
					*gameMemory->lightShader = LoadShader(0, TextFormat("./src/shaders/light_web.fs", GLSL_VERSION));
					*gameMemory->explosionShader = LoadShader(0, TextFormat("./src/shaders/explode_web.glsl", GLSL_VERSION));
					gameMemory->gameState->state = STATE_RUNNING;
					gameMemory->gameState->stateChanged = true;
#else
					InitializeOptions(options);
					InitializeGameState(gameState);
					gameState->state = STATE_RUNNING;
					gameState->stateChanged = true;
#endif
				}
				break;
			}
		case STATE_PAUSED:
			{
				// PauseMusicStream(audio->music);
				LoopSoundtrack(&audio->music[audio->currentSongtrackID]);
				if(IsSoundPlaying(audio->sounds[SOUND_SHIELD]))
				{
					PauseSound(audio->sounds[SOUND_SHIELD]);
				}
				if (options->languageChanged)
				{
					options->languageChanged = false;
				}
				if (options->musicVolumeChanged) 
				{
					SetMusicVolume(audio->music[audio->currentSongtrackID], options->musicVolume);
					options->musicVolumeChanged = false;
				}
				if (options->fxVolumeChanged) 
				{
					SetFxVolume(audio, options->fxVolume);
					options->fxVolumeChanged = false;
				}
				if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) 
				{
					gameState->state = gameState->lastState;
					gameState->stateChanged = true;
				}
				break;
			}
	}
}

void DrawScene(GameState* gameState, Options* options, TextureAtlas* atlas, RenderTexture2D* scene, Shader* shader, Shader* explosionShader)
{
	int texSizeLoc = GetShaderLocation(*shader, "textureSize");
	int progressLoc = GetShaderLocation(*explosionShader, "progress");
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
				// Draw Colliders (outside of shader mode for corect color)
				if (options->showDebugInfo)
				{
					DrawRectangleLinesEx(gameState->player.collider, 2.0, GREEN);
					for (int i = 0; i < gameState->bulletCount; i++)
					{
						DrawRectangleLinesEx(gameState->bullets[i].collider, 2.0, PURPLE);
					}
					for (int i = 0; i < gameState->asteroidCount; i++)
					{
						DrawRectangleLinesEx(gameState->asteroids[i].collider, 2.0, RED);
					}
					for (int i = 0; i < gameState->boostCount; i++)
					{
						DrawRectangleLinesEx(gameState->boosts[i].collider, 2.0, YELLOW);
					}
					for (int i = 0; i < gameState->enemyCount; i++)
					{
						DrawRectangleLinesEx(gameState->enemies[i].collider, 2.0, BLUE);
					}
					DrawRectangleRec(gameState->currentCollision, RED);
					gameState->currentCollision = (Rectangle){0,0,0,0};
				}


				// Draw Stars
				{
					BeginBlendMode(BLEND_ALPHA);
					for (int starIndex = 0; starIndex < gameState->starCount; starIndex++)
					{
						Star* star = &gameState->stars[starIndex];

						const int texture_x = star->position.x - star->sprite.coords.width / 2.0 * star->size;
						const int texture_y = star->position.y - star->sprite.coords.height / 2.0 * star->size;
						Color starColor = ColorAlpha(WHITE, star->alpha);
						DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_STAR1).coords, (Vector2) {texture_x, texture_y}, starColor);
					}
					EndBlendMode();
				}
				// Only after drawing stars with alpha values. Otherwise alpha needs to be passed to the shader?
				BeginShaderMode(*shader);
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
						if (asteroid->dying) {
							BeginShaderMode(*explosionShader);
							BeginBlendMode(BLEND_ALPHA);
							// float progress = Clamp(asteroid->deathTime/4.0,0.0f,1.0f);		
							float progress = Remap(asteroid->deathTime,0.0f,0.5f,0.0f,1.0f);		
							SetShaderValue(*explosionShader, progressLoc, &progress, SHADER_UNIFORM_FLOAT);
							DrawTexturePro(atlas->textureAtlas, asteroid->sprite.coords, asteroidDrawRect, 
									(Vector2){asteroid->collider.width/2.0f, asteroid->collider.height/2.0f}, asteroid->rotation, WHITE);
							EndBlendMode();
							EndShaderMode();
						} else {
							SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
							DrawTexturePro(atlas->textureAtlas, asteroid->sprite.coords, asteroidDrawRect, 
									(Vector2){asteroid->collider.width/2.0f, asteroid->collider.height/2.0f}, asteroid->rotation, WHITE);
						}
						// SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// DrawTexturePro(atlas->textureAtlas, asteroid->sprite.coords, asteroidDrawRect, 
						// 		(Vector2){asteroid->collider.width/2.0f, asteroid->collider.height/2.0f}, asteroid->rotation, WHITE);

						// DrawRectangleLines(asteroidDrawRect.x, asteroidDrawRect.y,
						// 		asteroidDrawRect.width, asteroidDrawRect.height, GREEN);
					}
				}
				// Draw Bullets
				{
					for (int bulletIndex = 0; bulletIndex < gameState->bulletCount; bulletIndex++)
					{
						Bullet* bullet = &gameState->bullets[bulletIndex];
						Vector2 texSize = { bullet->collider.width, bullet->collider.height };
						SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// Change the frame per second speed of animation
						// atlas->animations[SpriteToAnimation[SPRITE_BULLET]].framesPerSecond = 14;
						bool flipY = false;
						if (bullet->owner != &gameState->player) flipY = true;
						DrawSpriteAnimationPro(&atlas->textureAtlas, 
											   &atlas->animations[SpriteToAnimation[SPRITE_BULLET]],
											   bullet->collider, 
											   (Vector2){0, 0}, 
											   bullet->rotation, 
											   WHITE, 
											   *shader, 
											   false, 
											   flipY);
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
						SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						// Rectangle destination = {texture_x, texture_y, width, height}; // origin in coordinates and scale
						DrawSpriteAnimationPro(&atlas->textureAtlas, &atlas->animations[SpriteToAnimation[SPRITE_SCRAPMETAL]], boostDrawRect, pivot, boost->rotation, WHITE, *shader, false, false);
						// DrawRectangleLines(boostDrawRect.x, boostDrawRect.y, boostDrawRect.width, boostDrawRect.height, RED);
						// DrawRectangleLines(boost->collider.x, boost->collider.y, boost->collider.width, boost->collider.height, GREEN);
					}
				}
				// Draw Enemies
				{
					for (int i = 0; i < gameState->enemyCount; i++)
					{
						Enemy* enemy = &gameState->enemies[i];
						Vector2 texSize = { enemy->sprite.coords.width, enemy->sprite.coords.height };
						SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						DrawTexturePro(atlas->textureAtlas, enemy->sprite.coords, enemy->collider, (Vector2){0,0}, 0, WHITE);
					}
					// char buffer[100] = {0};
					// sprintf(buffer, "Timer: %.2f", gameState->enemySpawnTime);
					// DrawTextEx(options->font, buffer, (Vector2){20,50}, 30, 1, WHITE);
					// sprintf(buffer, "Count: %d", gameState->enemyCount);
					// DrawTextEx(options->font, buffer, (Vector2){20,80}, 30, 1, WHITE);
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
							if (gameState->explosionCount > 0)
							{
								*explosion = gameState->explosions[--gameState->explosionCount];
							} else {
								gameState->explosionCount = 0;
							}
							continue;
						}

						Rectangle dest = {
							explosion->position.x - width/2,
							explosion->position.y - height/2,
							width,
							height
						};

						Vector2 texSize = { width, height };
						SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						bool finished = DrawSpriteAnimationOnce(
								atlas->textureAtlas,
								atlas->animations[SpriteToAnimation[SPRITE_EXPLOSION]],
								dest,
								(Vector2){0, 0},
								0.0f,
								WHITE,
								*shader,
								explosion->startTime
								);

						if (finished)
							explosion->active = false;
					}
				}
				// Draw Player
				{
					atlas->animations[SpriteToAnimation[SPRITE_PLAYER]].framesPerSecond = 14;
					const int texture_x = gameState->player.position.x - gameState->player.sprite.coords.width * gameState->player.size / gameState->player.animationFrames / 2.0;
					const int texture_y = gameState->player.position.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0;
					Rectangle playerDestination = {texture_x, texture_y, 
						gameState->player.sprite.coords.width / gameState->player.animationFrames * gameState->player.size, 
						gameState->player.sprite.coords.height * gameState->player.size}; // origin in coordinates and scale
					Vector2 origin = {0, 0}; // so it draws from top left of image
					Vector2 texSize = { playerDestination.width, playerDestination.height };
					SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
					if (gameState->player.invulTime <= 0.0f) {
						DrawSpriteAnimationPro(&atlas->textureAtlas, &atlas->animations[SpriteToAnimation[SPRITE_PLAYER]], playerDestination, origin, 0, WHITE, *shader, 0, 0);
					} else {
						if (((int)(gameState->player.invulTime * 10)) % 2 == 0) {
							DrawSpriteAnimationPro(&atlas->textureAtlas, &atlas->animations[SpriteToAnimation[SPRITE_PLAYER]], playerDestination, origin, 0, WHITE, *shader, 0, 0);
						}
					}
					// DrawCircleV(gameState->player.position, 8.0f, GREEN);
					// DrawRectangleLines(playerDestination.x, playerDestination.y, playerDestination.width, playerDestination.height, BLUE);

					// Draw shield
					if (gameState->player.shieldEnabled)
					{
						atlas->animations[SpriteToAnimation[SPRITE_SHIELD]].framesPerSecond = 14;
						Vector2 texSize = { getSprite(SpriteToAnimation[SPRITE_SHIELD]).coords.width / getSprite(SpriteToAnimation[SPRITE_SHIELD]).numFrames,
											getSprite(SpriteToAnimation[SPRITE_SHIELD]).coords.height };
						SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
						DrawSpriteAnimationPro(&atlas->textureAtlas, &atlas->animations[SpriteToAnimation[SPRITE_SHIELD]], playerDestination, origin, 0, WHITE, *shader, false, false);
					}
				}
				EndShaderMode();
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

void DrawLightmap(GameState* gameState, Options* options, RenderTexture2D* litScene, Shader* lightShader)
{
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());

	int uLightPos = GetShaderLocation(*lightShader, "lightPos");
	int uLightRadius = GetShaderLocation(*lightShader, "lightRadius");
	int uLightCount = GetShaderLocation(*lightShader, "lightCount");
	int uAspect = GetShaderLocation(*lightShader, "aspect");
	int uAmbience = GetShaderLocation(*lightShader, "ambience");

	float ambience = 0.6f;
	float lightRadius = 0.15f;  // normalized radius
	float aspect = (float)viewport.width / (float)viewport.height;
	// SetShaderValueTexture(*lightShader, GetShaderLocation(*lightShader, "lightTexture"), litScene->texture);
	SetShaderValue(*lightShader, uLightRadius, &lightRadius, SHADER_UNIFORM_FLOAT);
	SetShaderValue(*lightShader, uAspect, &aspect, SHADER_UNIFORM_FLOAT);
	SetShaderValue(*lightShader, uAmbience, &ambience, SHADER_UNIFORM_FLOAT);
	// --- Build lightmap ---
	BeginTextureMode(*litScene);
	ClearBackground(BLACK);
	BeginBlendMode(BLEND_ADDITIVE);
	// Prepare arrays
	Vector2 lights[128];
	int lc = 0;

	for (int i = 0; i < gameState->bulletCount; i++) {
		// convert pixel -> normalized UV (0–1)
		lights[lc].x = gameState->bullets[i].position.x / VIRTUAL_WIDTH;
		lights[lc].y = 1.0f - gameState->bullets[i].position.y / VIRTUAL_HEIGHT;
		lc++;
	}
	if (gameState->boostCount > 0) {
		for (int i = 0; i < gameState->boostCount; i++) {
			// convert pixel -> normalized UV (0–1)
			lights[lc].x = gameState->boosts[i].position.x / VIRTUAL_WIDTH;
			lights[lc].y = 1.0f - gameState->boosts[i].position.y / VIRTUAL_HEIGHT;
			lc++;
		}
	}
	if (gameState->enemyCount > 0) {
		for (int i = 0; i < gameState->enemyCount; i++) {
			// convert pixel -> normalized UV (0–1)
			lights[lc].x = gameState->enemies[i].position.x / VIRTUAL_WIDTH;
			lights[lc].y = 1.0f - gameState->enemies[i].position.y / VIRTUAL_HEIGHT;
			lc++;
		}
	}

	lights[lc].x = gameState->player.position.x / VIRTUAL_WIDTH;
	lights[lc].y = 1.0f - gameState->player.position.y / VIRTUAL_HEIGHT;
	lc++;
	// Upload array
	SetShaderValue(*lightShader, uLightCount, &lc, SHADER_UNIFORM_INT);
	SetShaderValueV(*lightShader, uLightPos, lights, SHADER_UNIFORM_VEC2, lc);

	EndBlendMode();
	EndTextureMode();
}

void DrawHealthBar(GameState* gameState, Options* options, TextureAtlas* atlas, Shader* shader)
{
	// Draw player health
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
	int texSizeLoc = GetShaderLocation(*shader, "textureSize");
	Vector2 texSize = { getSprite(SPRITE_HEART).coords.width / getSprite(SPRITE_HEART).numFrames,
						getSprite(SPRITE_HEART).coords.height };
	SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
	for (int i = 1; i <= gameState->player.health; i++)
	{
		const int texture_x = letterBoxOffsetX + i * 16;
		const int texture_y = letterBoxOffsetY + getSprite(SPRITE_HEART).coords.height;
		DrawTextureRec(atlas->textureAtlas, getSprite(SPRITE_HEART).coords, (Vector2){texture_x, texture_y}, WHITE);
	}
}

void DrawShieldText(GameState* gameState, Options* options, TextureAtlas* atlas, Shader* shader)
{
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float scale = viewport.width/VIRTUAL_WIDTH;
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;

	// const int texture_x = (letterboxWidth + gameState->player.position.x) * scale;
	// const int texture_y = (letterboxHeight + gameState->player.position.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0) * scale;
	const int texture_x = letterBoxOffsetX + gameState->player.position.x * scale;
	const int texture_y = letterBoxOffsetY + (gameState->player.position.y - gameState->player.sprite.coords.height * gameState->player.size / 2.0) * scale;
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

void DrawScore(GameState* gameState, Options* options, TextureAtlas* atlas)
{

	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float scale = viewport.width / VIRTUAL_WIDTH;
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
	// Draw Score
	float recHeight = 30.0f;
	float recWidth = 100.0f;
	float recPosX = letterBoxOffsetX + VIRTUAL_WIDTH * scale - recWidth - 10.0f;
	float recPosY = letterBoxOffsetY + recHeight - 10.0f;

	DrawRectangle(recPosX, recPosY, gameState->experience / ((float)gameState->player.level*1.5f) / 5.0f, recHeight, ColorAlpha(BLUE, 0.5));
	DrawRectangleLines(recPosX, recPosY, recWidth, recHeight, ColorAlpha(WHITE, 0.5));
	Vector2 textSize = MeasureTextEx(options->font, T(TXT_EXPERIENCE), 20.0f, GetDefaultSpacing(20.0f));
	DrawTextEx(options->font, T(TXT_EXPERIENCE), (Vector2){recPosX + recWidth / 2.0f - textSize.x / 2.0f, recPosY + recHeight / 2.0f - textSize.y / 2.0f}, 20.0f, GetDefaultSpacing(20.0f), WHITE);

	recPosX = letterBoxOffsetX + VIRTUAL_WIDTH * 0.5 * scale;
	recPosY = letterBoxOffsetY + VIRTUAL_HEIGHT * 0.05 * scale + 10.0f;
	textSize = MeasureTextEx(options->font, TF(TXT_SCORE, gameState->score), 
			                         20.0f, GetDefaultSpacing(20.0f));
	DrawTextEx(options->font, TF(TXT_SCORE, gameState->score),
			  (Vector2){recPosX - textSize.x / 2.0f,
			            recPosY - textSize.y / 2.0f}, 
						20.0f, GetDefaultSpacing(20.0f), WHITE);
}

void DrawUpgrades(GameState* gameState, Options* options, TextureAtlas* atlas, Shader* shader)
{
	int texSizeLoc = GetShaderLocation(*shader, "textureSize");
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
	DrawTextWave(options->font, T(TXT_LEVEL_UP), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f - 100.0f}, 40, WHITE, true, gameState->time, 7.0f, 3.0f, 0.5f);
	DrawTextWave(options->font, T(TXT_CHOOSE_UPGRADE), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f - 55.0f}, 40, WHITE, true, gameState->time, 7.0f, 3.0f, 0.5f);
	float scaling = 3.0f;
	const float width  = getSprite(SPRITE_UPGRADEMULTISHOT).coords.width;
	const float height = getSprite(SPRITE_UPGRADEMULTISHOT).coords.height;
	const float pos_x  = letterBoxOffsetX + viewport.width/2 - width/2;
	const float pos_y  = letterBoxOffsetY + viewport.height/2 - height/2 + 130;
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

	BeginShaderMode(*shader);
	for (int i = 0; i < UPGRADE_COUNT; i++) {
		Rectangle upgradeRect = 
		{
			.width  = width,
			.height = height,
			.x = pos_x + (i - 1) * spacing_x + width / 2.0f, // i-1 to center the middle upgrade
			.y = pos_y + height / 2.0f,
		};

		Vector2 texSize = { width, height };
		SetShaderValue(*shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
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
	EndShaderMode();
}

void DrawPauseMenu(GameState* gameState, Options* options, TextureAtlas* atlas)
{
	const Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float scale = viewport.width / VIRTUAL_WIDTH;
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
	DrawTextCentered(options->font, T(TXT_GAME_PAUSED), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/5.0f}, 40 * scale, WHITE);
	// Draw a window box
	const float boxWidth = 475.0f * scale;
	const float boxHeight = 350.0f * scale;
	const float boxPosX = letterBoxOffsetX + viewport.width/2;
	const float boxPosY = letterBoxOffsetY + viewport.height/2;
	GuiWindowBox((Rectangle){boxPosX-boxWidth/2,
			                 boxPosY-boxHeight/2, 
							 boxWidth, boxHeight}, T(TXT_SETTINGS));
	// Language label
	const float labelWidth = 160.0f * scale;
	const float labelHeight = 100.0f * scale;
	const float buttonWidth = 100 * scale;
	const float buttonHeight = 50 * scale;
	const float sliderWidth = 160 * scale;
	const float sliderHeight = 20 * scale;
	const float checkboxWidth = 20 * scale;
	const float checkboxHeight = 20 * scale;
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6, 
			labelWidth, labelHeight}, T(TXT_LANGUAGE));
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6+1*boxHeight/12, 
			labelWidth, labelHeight}, T(TXT_MUSICVOLUME));
	GuiLabel((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
			boxPosY-labelHeight/2-boxHeight/6+2*boxHeight/12, 
			labelWidth, labelHeight}, T(TXT_FXVOLUME));
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
	GuiCheckBox((Rectangle){ boxPosX-labelWidth/2-boxWidth/6, 
							 boxPosY-sliderHeight/2-boxHeight/6+3*boxHeight/12, 
							 checkboxWidth, checkboxHeight }, T(TXT_SHOW_DEBUG_INFO), &options->showDebugInfo);

	if (GuiButton((Rectangle){ boxPosX-boxWidth/4-buttonWidth/2+checkboxWidth, 
							   boxPosY-buttonHeight/4+boxHeight/6+checkboxHeight, 
							   buttonWidth, buttonHeight }, T(TXT_CONTINUE))) 
	{
		gameState->state = gameState->lastState;
		gameState->stateChanged = true;
	}
	if (GuiButton((Rectangle){ boxPosX+boxWidth/4-buttonWidth/2+checkboxWidth, 
							   boxPosY-buttonHeight/4+boxHeight/6+checkboxHeight, 
							   buttonWidth, buttonHeight }, T(TXT_QUIT))) 
	{
		gameState->shouldExit = true;
	}
	// Draw dropdown box
	const float dropdownWidth = 160.0f * scale;
	const float dropdownHeight = 20.0f * scale;
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
		options->font = LoadLanguageFont("./assets/fonts/UnifontExMono.ttf", options->maxFontSize, options->language);
		GuiSetFont(options->font);
		options->lastLanguage = options->language;
	}
}

void DrawFPSInViewport(Rectangle viewport)
{
    float offsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
    float offsetY = (GetRenderHeight() - viewport.height) / 2.0f;

    DrawFPS(offsetX + 15, offsetY + 50);
}

void DrawEnemyHealthBar(GameState* gameState, Options* options, TextureAtlas* atlas, Shader* shader) 
{
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	float scale = viewport.width / VIRTUAL_WIDTH;
	float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
	float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
	float recHeight = 10.0f;
	float recWidth = 50.0f;
	for (int i = 0; i < gameState->enemyCount; i++) {
		Enemy* enemy = &gameState->enemies[i];
		float recPosX = letterBoxOffsetX + (enemy->position.x - recWidth/2.0f) * scale ;
		float recPosY = letterBoxOffsetY + (enemy->position.y + enemy->sprite.coords.height - recHeight) * scale ;
		DrawRectangle(recPosX, recPosY, recWidth/20.0f * enemy->health * scale, recHeight * scale, RED);
		DrawRectangleLines(recPosX, recPosY, recWidth * scale, recHeight * scale, WHITE);
	}
}

void DrawUI(GameState* gameState, Options* options, TextureAtlas* atlas, Shader* shader)
{
	Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
	switch (gameState->state) {
		case STATE_RUNNING:
			{
				DrawHealthBar(gameState, options, atlas, shader);
				DrawEnemyHealthBar(gameState, options, atlas, shader);
				DrawScore(gameState, options, atlas);

				// DrawCircleV(gameState->player.position, 5.0f, RED);
				// DrawCircleV((Vector2){texture_x, texture_y}, 5.0f, RED);
				if (gameState->player.shieldEnabled)
				{
					DrawShieldText(gameState, options, atlas, shader);
				}
				break;
			}
		case STATE_MAIN_MENU:
			{
				float scale = viewport.width / VIRTUAL_WIDTH;
				float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
				float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				DrawTextWave(options->titleFont, T(TXT_GAME_TITLE), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f}, 90 * scale, WHITE, false, gameState->time, 2.0f, 5.0f, 0.5f);
				DrawTextCentered(options->titleFont, T(TXT_INSTRUCTIONS), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f + 90}, 20 * scale, WHITE);
				DrawTextCentered(options->titleFont, T(TXT_PRESS_TO_PLAY), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f + 120}, 20 * scale, WHITE);
				DrawTextCentered(options->titleFont, "v0.1", (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height - 15}, 15 * scale, WHITE);

				break;
			}
		case STATE_GAME_OVER:
			{
				float letterBoxOffsetX = (GetRenderWidth()  - viewport.width)  / 2.0f;
				float letterBoxOffsetY = (GetRenderHeight() - viewport.height) / 2.0f;
				Color backgroundColor = ColorFromHSV(259, 1, 0.07);
				ClearBackground(backgroundColor);
				DrawTextCentered(options->font, T(TXT_GAME_OVER), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f}, 40, WHITE);
				char scoreText[100] = {0};
				DrawTextCentered(options->font, TF(TXT_SCORE, gameState->score), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f + 30.0f}, 20.0f, WHITE);
				DrawTextCentered(options->font, T(TXT_TRY_AGAIN), (Vector2){letterBoxOffsetX + viewport.width/2.0f, letterBoxOffsetY + viewport.height/2.0f + 60.0f}, 20.0f, WHITE);
				break;
			}
		case STATE_UPGRADE:
			{
				DrawHealthBar(gameState, options, atlas, shader);
				DrawScore(gameState, options, atlas);
				DrawUpgrades(gameState, options, atlas, shader);
				break;
			}
		case STATE_PAUSED:
			{
				DrawHealthBar(gameState, options, atlas, shader);
				DrawEnemyHealthBar(gameState, options, atlas, shader);
				DrawScore(gameState, options, atlas);
				if (gameState->lastState == STATE_RUNNING) 
				{
				}
				else if (gameState->lastState == STATE_UPGRADE) 
				{
					DrawUpgrades(gameState, options, atlas, shader);
				}
				DrawPauseMenu(gameState, options, atlas);
				break;
			}
	}

	if(options->showDebugInfo)
	{
		DrawFPSInViewport(viewport);
	}
}

void DrawComposite(RenderTexture2D* scene, Options* options, RenderTexture2D* litScene, GameState* gameState, Shader* lightShader)
{
    Rectangle viewport = GetScaledViewport(GetRenderWidth(), GetRenderHeight());
    Rectangle src = {
        0, 0,
        (float)scene->texture.width,
        -(float)scene->texture.height
    };

    if (!options->disableShaders) BeginShaderMode(*lightShader);
    DrawTexturePro(scene->texture, src, viewport, (Vector2){0, 0}, 0, WHITE);
    if (!options->disableShaders) EndShaderMode();
}

void DrawGame(GameMemory* gameMemory)
{
	GameState* gameState = gameMemory->gameState;
	Options* options = gameMemory->options;
	TextureAtlas* atlas = gameMemory->atlas;
	RenderTexture2D* scene = gameMemory->scene;
	RenderTexture2D* litScene = gameMemory->litScene;
	Shader* shader = gameMemory->shader;
	Shader* lightShader = gameMemory->lightShader;
	Shader* explosionShader = gameMemory->explosionShader;

	DrawLightmap(gameState, options, litScene, lightShader);
	DrawScene(gameState, options, atlas, scene, shader, explosionShader);

	BeginDrawing();
	{
		ClearBackground(BLACK);
		DrawComposite(scene, options, litScene, gameState, lightShader);
		DrawUI(gameState, options, atlas, shader);
	}
	EndDrawing();
}

void UpdateDrawFrame(GameMemory* gameMemory)
{
	gameMemory->gameState->dt = GetFrameTime() * gameMemory->gameState->timeScale;
	gameMemory->gameState->time += gameMemory->gameState->dt;
	HandleResize(gameMemory->options);
	UpdateGame(gameMemory);
	DrawGame(gameMemory);
#ifndef PLATFORM_WEB
	if (gameMemory->gameState->gifRecorder.recording)
	{
		GifRecordUpdate(&gameMemory->gameState->gifRecorder);
	}
#endif
}


