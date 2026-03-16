#include <sys/stat.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>

#include "game.h"

#if defined(_WIN32)

#include <windows.h>

#define LIB_HANDLE HMODULE
#define LoadLib(path) LoadLibraryA(path)
#define GetSym(lib, name) GetProcAddress(lib, name)
#define CloseLib(lib) FreeLibrary(lib)

#else

#include <dlfcn.h>

#define LIB_HANDLE void*
#define LoadLib(path) dlopen(path, RTLD_NOW)
#define GetSym(lib, name) dlsym(lib, name)
#define CloseLib(lib) dlclose(lib)

#endif

typedef struct GameCode {
    LIB_HANDLE handle;
    time_t lastWriteTime;

    GameInitFn Init;
    GameUpdateFn Update;
    GameCleanupFn Cleanup;
} GameCode;

time_t GetLastWriteTime(const char* path)
{
	struct stat attr;
	if (stat(path, &attr) == 0)
		return attr.st_mtime;

	return 0;
}

#if defined(_WIN32)

void CopyFile(const char *src, const char *dst)
{
    CopyFileA(src, dst, FALSE);
}

#else

void CopyFile(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (in < 0 || out < 0) return;

    char buf[8192];
    ssize_t n;

    while ((n = read(in, buf, sizeof(buf))) > 0)
        write(out, buf, n);

    close(in);
    close(out);
}

#endif
GameCode LoadGameCode()
{
    GameCode game = {0};

#if defined(_WIN32)
    const char *src = "game.dll";
    const char *tmp = "game_loaded.dll";
#else
    const char *src = "./game.so";
    const char *tmp = "./game_loaded.so";
#endif

    CopyFile(src, tmp);

    game.handle = LoadLib(tmp);

    if (!game.handle)
    {
#ifndef _WIN32
        printf("dlopen error: %s\n", dlerror());
#else
        printf("LoadLibrary failed\n");
#endif
        return game;
    } 

    game.Init   = (GameInitFn)GetSym(game.handle, "InitGame");
    game.Update = (GameUpdateFn)GetSym(game.handle, "UpdateDrawFrame");
    game.Cleanup = (GameCleanupFn)GetSym(game.handle, "Cleanup");
    // game.Draw   = (GameDrawFn)GetSym(game.handle, "DrawGame");

    if (!game.Update || !game.Init)
    {
#ifndef _WIN32
        printf("dlsym error: %s\n", dlerror());
#endif
    }

    game.lastWriteTime = GetLastWriteTime(src);

    printf("Game loaded\n");

    return game;
}

void UnloadGameCode(GameCode* game)
{
	if (game->handle)
	{
		CloseLib(game->handle);
		game->handle = NULL;
	}
}

int main()
{
	GameState gameState = {0};
	Options options = {0};
	Audio audio = {0};

	TextureAtlas atlas = {0};
	SpriteMask spriteMasks[128] = {0};

	RenderTexture2D scene = {0};
	RenderTexture2D litScene = {0};

	Shader shader = {0};
	Shader lightShader = {0};

	Rectangle currentCollision = {0};
	bool shouldExit = false;

	static GameMemory gameMemory = {0};
	gameMemory.gameState = &gameState;
	gameMemory.options = &options;
	gameMemory.audio = &audio;
	gameMemory.atlas = &atlas;
	gameMemory.spriteMasks = spriteMasks;
	gameMemory.scene = &scene;
	gameMemory.litScene = &litScene;
	gameMemory.shader = &shader;
	gameMemory.lightShader = &lightShader;

	GameCode game = LoadGameCode();

#ifdef PLATFORM_WEB
	shader = LoadShader(0, TextFormat("./shaders/test_web.glsl", GLSL_VERSION));
	lightShader = LoadShader(0, TextFormat("./shaders/light_web.fs", GLSL_VERSION));
#else
	shader = LoadShader(0, TextFormat("./src/shaders/test.glsl", GLSL_VERSION));
	lightShader = LoadShader(0, TextFormat("./src/shaders/light.fs", GLSL_VERSION));
#endif
	if (game.Init) game.Init(&gameMemory);
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(game.Update, TARGET_FPS, 1);
#else
	while (!WindowShouldClose() && !shouldExit)
	{
		time_t newWriteTime = GetLastWriteTime("./game.so");
		if (newWriteTime != game.lastWriteTime)
		{
			printf("Hot reloading game...\n");

#ifdef PLATFORM_WEB
			shader = LoadShader(0, TextFormat("./shaders/test_web.glsl", GLSL_VERSION));
			lightShader = LoadShader(0, TextFormat("./shaders/light_web.fs", GLSL_VERSION));
#else
			shader = LoadShader(0, TextFormat("./src/shaders/test.glsl", GLSL_VERSION));
			lightShader = LoadShader(0, TextFormat("./src/shaders/light.fs", GLSL_VERSION));
#endif

			if (game.Init) game.Init(&gameMemory);
			UnloadGameCode(&game);
			game = LoadGameCode();
		}
		options.previousWidth  = VIRTUAL_WIDTH;
		options.previousHeight = VIRTUAL_HEIGHT;

		if (game.Update) game.Update(&gameMemory);
	}
#endif

	if (game.Cleanup) game.Cleanup(&gameMemory);
	UnloadGameCode(&game);
	CloseWindow();
}
