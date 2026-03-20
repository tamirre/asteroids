// Building for web on linux for now
#include <dlfcn.h>

// CopyFileCustom
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define LoadLib(path) dlopen(path, RTLD_NOW)
#define GetSym(lib, name) dlsym(lib, name)
#define CloseLib(lib) dlclose(lib)

static const char dll[256] = "./game.wasm";

static GameCode *g_game;
static GameMemory* g_memory;

static inline GameCode LoadGameCode()
{
    GameCode game = {0};

    game.handle = LoadLib(dll);

    if (!game.handle)
    {
        printf("LoadLibrary failed\n");
        return game;
    } else {
		printf("Successfully loaded %s\n", dll);
	}

    game.Init   = (GameInitFn)GetSym(game.handle, "InitGame");
    game.Update = (GameUpdateFn)GetSym(game.handle, "UpdateDrawFrame");
    game.Cleanup = (GameCleanupFn)GetSym(game.handle, "Cleanup");
	if (!game.Init || !game.Update || !game.Cleanup)
	{
		printf("dlsym error\n");
	}
    printf("Game loaded\n");

    return game;
}

static inline void UnloadGameCode(GameCode* game)
{
	if (game->handle)
	{
		if(CloseLib(game->handle))
		{
			printf("DLL unloaded successfully\n");
		}
		else
		{
			const char* err = dlerror();   // get the error message
			if (err)
				printf("dlclose failed: %s\n", err);
			else
				printf("dlclose failed: unknown error\n");
		}
		// game->handle = NULL;
		// game->Init = NULL;
		// game->Update = NULL;
		// game->Cleanup = NULL;
		// game->InitAudio = NULL;
	}
}

static inline void WebWrapper()
{
	// time_t newWriteTime = GetLastWriteTime(dll);
	// if (newWriteTime != game.lastWriteTime)
	// {
	// 	printf("Hot reloading game...\n");
	//
	// 	*g_memory->shader = LoadShader(0, TextFormat("./src/shaders/test_web.glsl", GLSL_VERSION));
	// 	*g_memory->lightShader = LoadShader(0, TextFormat("./src/shaders/light_web.fs", GLSL_VERSION));
	//
	// 	// CloseAudioDevice();
	// 	UnloadGameCode(g_game);
	// 	GameCode newGame = LoadGameCode();
	// 	if (newGame.handle)
	// 	{
	// 		g_game = &newGame; 
	// 	}
	// }
	if (g_game->Update) g_game->Update(g_memory);
}

