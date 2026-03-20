#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>

#define LIB_HANDLE HMODULE
#define LoadLib(path) LoadLibraryA(path)
#define GetSym(lib, name) GetProcAddress(lib, name)
#define CloseLib(lib) FreeLibrary(lib)
static const char dll[256] = "./src/game.dll";

static inline void PrintLastError(const char* prefix)
{
    DWORD errorCode = GetLastError();

    if (errorCode == 0)
    {
        printf("%s: no error\n", prefix);
        return;
    }

    LPSTR messageBuffer = NULL;

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    printf("%s: (%lu) %s\n", prefix, errorCode, messageBuffer);

    LocalFree(messageBuffer);
}

static inline int CopyFileCustom(const char *from, const char *to)
{
    if (!CopyFileA(from, to, FALSE)) {
        PrintLastError("CopyFile failed");
        return -1;
    }
    return 0;
}

static inline GameCode LoadGameCode()
{
    GameCode game = {0};

	char tmp[256];
	snprintf(tmp, sizeof(tmp), "./src/game_%ld.so", time(NULL));
	printf("Copying game from %s to %s\n", dll, tmp);
	if(CopyFileCustom(dll, tmp) == -1)
	{
		printf("Copying game from %s to %s failed\n", dll, tmp);
	}

	printf("Loading game from %s\n", tmp);
	HMODULE result = LoadLib(tmp);
	if (result == NULL)
	{
		printf("LoadLibrary failed\n");
		return game;
	} else {
		game.handle = result;
	}

    if (!game.handle)
    {
		PrintLastError("dlopen error");
        return game;
    } 

    game.Init   = (GameInitFn)GetSym(game.handle, "InitGame");
    game.Update = (GameUpdateFn)GetSym(game.handle, "UpdateDrawFrame");
    game.Cleanup = (GameCleanupFn)GetSym(game.handle, "Cleanup");
    game.InitAudio = (GameInitAudioFn)GetSym(game.handle, "InitAudio");

    if (!game.Update || !game.Init)
    {
		PrintLastError("dlsym error");
    }

    game.lastWriteTime = GetLastWriteTime(dll);

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
            PrintLastError("FreeLibrary failed");
        }
		// game->handle = NULL;
		// game->Init = NULL;
		// game->Update = NULL;
		// game->Cleanup = NULL;
		// game->InitAudio = NULL;
	}
}
