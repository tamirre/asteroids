#include "game.h"
#include <time.h>
#include <sys/stat.h>

typedef struct GameCode {
    void* handle;
    time_t lastWriteTime;

    GameInitFn Init;
    GameUpdateFn Update;
    GameCleanupFn Cleanup;
	GameInitAudioFn InitAudio;
} GameCode;

time_t GetLastWriteTime(const char* path)
{
	struct stat attr;
	if (stat(path, &attr) == 0)
		return attr.st_mtime;

	return 0;
}
#ifdef PLATFORM_WINDOWS
#include "platform_win32.h"
#elif defined(PLATFORM_WEB) || __EMSCRIPTEN__
#include "platform_web.h"
#else
#include "platform_unix.h"
#endif

int main()
{
	GameState gameState = {0};
	Options options = {0};
	Audio audio = {0};
	TextureAtlas atlas = {0};
	SpriteMask spriteMasks[] = {0};
	RenderTexture2D scene = {0};
	RenderTexture2D litScene = {0};
	Shader shader = {0};
	Shader explosionShader = {0};
	Shader lightShader = {0};

	GameMemory gameMemory = {0}; 
	gameMemory.gameState = &gameState;
	gameMemory.options = &options;
	gameMemory.audio = &audio;
	gameMemory.atlas = &atlas;
	gameMemory.spriteMasks = spriteMasks;
	gameMemory.scene = &scene;
	gameMemory.litScene = &litScene;
	gameMemory.shader = &shader;
	gameMemory.explosionShader = &explosionShader;
	gameMemory.lightShader = &lightShader;

	// InitAudioDevice();
#if defined(PLATFORM_WEB)
	GameCode game = LoadGameCode();
	if (game.Init) game.Init(&gameMemory);
	g_game = &game;
	g_memory = &gameMemory;
	*g_memory->shader = LoadShader(0, TextFormat("./src/shaders/default_web.glsl", GLSL_VERSION));
	*g_memory->lightShader = LoadShader(0, TextFormat("./src/shaders/light_web.fs", GLSL_VERSION));
	*g_memory->explosionShader = LoadShader(0, TextFormat("./src/shaders/explode_web.fs", GLSL_VERSION));
	emscripten_set_main_loop(WebWrapper, TARGET_FPS, 1);
#else
	GameCode game = LoadGameCode();
	if (game.Init) game.Init(&gameMemory);
	// printf("WindowShouldClose: %d\n", WindowShouldClose());
	// printf("shouldExit: %d\n", gameMemory.gameState->shouldExit);
	// TODO: WHY DOES WINDOWSHOULDCLOSE() NOT WORK ON WINDOWS?
#if defined(PLATFORM_WINDOWS)
	while (!gameMemory.gameState->shouldExit)
#else
	while (!WindowShouldClose() && !gameMemory.gameState->shouldExit)
#endif
	{
		time_t newWriteTime = GetLastWriteTime(dll);
		if (newWriteTime != game.lastWriteTime)
		{

			printf("Hot reloading game...\n");

			shader = LoadShader(0, TextFormat("./src/shaders/default.glsl", GLSL_VERSION));
			lightShader = LoadShader(0, TextFormat("./src/shaders/light.fs", GLSL_VERSION));

			// CloseAudioDevice();
			UnloadGameCode(&game);
			GameCode newGame = LoadGameCode();
			if (newGame.handle)
			{
				game = newGame; 
			}
			// printf("Reload done!\n");
		}

		if (game.Update) game.Update(&gameMemory);
	}
#endif
	if (game.Cleanup) game.Cleanup(&gameMemory);
	printf("Unloading game...\n");
	UnloadGameCode(&game);
#ifndef PLATFORM_WINDOWS
	CloseWindow();
#endif
}
