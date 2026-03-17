#include <sys/stat.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

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

// static char lastLoadedFile[256] = {0};

typedef struct GameCode {
    LIB_HANDLE handle;
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

#if defined(_WIN32)

void CopyFile(const char *src, const char *dst)
{
    CopyFileA(src, dst, FALSE);
}

#else

//source: https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
int CopyFile(const char *from, const char *to)
{
	int fd_to, fd_from;
	char buf[4096];
	ssize_t nread;
	int saved_errno;

	fd_from = open(from, O_RDONLY);
	if (fd_from < 0)
		return -1;

	fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd_to < 0)
		goto out_error;

	while (nread = read(fd_from, buf, sizeof buf), nread > 0)
	{
		char *out_ptr = buf;
		ssize_t nwritten;

		do {
			nwritten = write(fd_to, out_ptr, nread);

			if (nwritten >= 0)
			{
				nread -= nwritten;
				out_ptr += nwritten;
			}
			else if (errno != EINTR)
			{
				goto out_error;
			}
		} while (nread > 0);
	}

	if (nread == 0)
	{
		if (close(fd_to) < 0)
		{
			fd_to = -1;
			goto out_error;
		}
		close(fd_from);

		/* Success! */
		return 0;
	}

out_error:
	saved_errno = errno;

	close(fd_from);
	if (fd_to >= 0)
		close(fd_to);

	errno = saved_errno;
	return -1;
}


#endif

GameCode LoadGameCode()
{
    GameCode game = {0};

#if defined(_WIN32)
    const char *src = "./src/game.dll";
    const char *tmp = "./src/game_loaded.dll";
#else
    const char *src = "./src/game.so";
    // const char *tmp = "./src/game_loaded.so";
#endif

	char tmp[256];
	snprintf(tmp, sizeof(tmp), "./src/game_%ld.so", time(NULL));
	CopyFile(src, tmp);
	// usleep(100000);

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
    game.InitAudio = (GameInitAudioFn)GetSym(game.handle, "InitAudio");
    // game.Draw   = (GameDrawFn)GetSym(game.handle, "DrawGame");

    if (!game.Update || !game.Init)
    {
#ifndef _WIN32
        printf("dlsym error: %s\n", dlerror());
#endif
    }

    game.lastWriteTime = GetLastWriteTime(src);

    printf("Game loaded\n");
	
	// remove previous file if there was one
    // if (lastLoadedFile[0]) {
	unlink(tmp);
    // }
    // strncpy(lastLoadedFile, tmp, sizeof(lastLoadedFile));

    return game;
}

void UnloadGameCode(GameCode* game)
{
	if (game->handle)
	{
		CloseLib(game->handle);
		// game->handle = NULL;
	}
}

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
	gameMemory.lightShader = &lightShader;

	GameCode game = LoadGameCode();
	if (game.Init) game.Init(&gameMemory);
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(game.Update, TARGET_FPS, 1);
#else
	while (!WindowShouldClose() && !gameMemory.gameState->shouldExit)
	{
		time_t newWriteTime = GetLastWriteTime("./src/game.so");
		if (newWriteTime != game.lastWriteTime)
		{

			printf("Hot reloading game...\n");

#ifdef PLATFORM_WEB
			shader = LoadShader(0, TextFormat("./src/shaders/test_web.glsl", GLSL_VERSION));
			lightShader = LoadShader(0, TextFormat("./src/shaders/light_web.fs", GLSL_VERSION));
#else
			shader = LoadShader(0, TextFormat("./src/shaders/test.glsl", GLSL_VERSION));
			lightShader = LoadShader(0, TextFormat("./src/shaders/light.fs", GLSL_VERSION));
#endif
			// CloseAudioDevice();
			// usleep(500000);
			UnloadGameCode(&game);
			// usleep(100000);
			GameCode newGame = LoadGameCode();
			if (newGame.handle)
			{
				game = newGame; // swap function pointers only
				// InitAudioDevice();
				// game.InitAudio(gameMemory.audio, gameMemory.options); 
			}
		}

		if (game.Update) game.Update(&gameMemory);
	}
#endif

	if (game.Cleanup) game.Cleanup(&gameMemory);
	UnloadGameCode(&game);
	CloseWindow();
}
