#include <dlfcn.h>

// CopyFileCustom
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define LoadLib(path) dlopen(path, RTLD_NOW)
#define GetSym(lib, name) dlsym(lib, name)
#define CloseLib(lib) dlclose(lib)

static const char dll[256] = "./src/game.so";


//source: https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
static inline int CopyFileCustom(const char *from, const char *to)
{
	int fd_to, fd_from;
	char buf[4096];
	size_t nread;
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

static inline GameCode LoadGameCode()
{
    GameCode game = {0};

	char tmp[256];
	snprintf(tmp, sizeof(tmp), "./src/game_%ld.so", time(NULL));
	printf("Copying game from %s to %s\n", dll, tmp);
	if(CopyFileCustom(dll, tmp) == -1)
	{
		printf("Copying failed\n");
	}

	printf("Loading game from %s\n", tmp);
    game.handle = LoadLib(tmp);
    if (!game.handle)
    {
        printf("LoadLibrary failed\n");
        return game;
    } 

    game.Init   = (GameInitFn)GetSym(game.handle, "InitGame");
    game.Update = (GameUpdateFn)GetSym(game.handle, "UpdateDrawFrame");
    game.Cleanup = (GameCleanupFn)GetSym(game.handle, "Cleanup");
    game.InitAudio = (GameInitAudioFn)GetSym(game.handle, "InitAudio");

    game.lastWriteTime = GetLastWriteTime(dll);

	// unlink(tmp);
    return game;
}

static inline void UnloadGameCode(GameCode* game)
{
	if (game->handle)
	{
		if(CloseLib(game->handle) == 0)
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
		game->handle = NULL;
		game->Init = NULL;
		game->Update = NULL;
		game->Cleanup = NULL;
		game->InitAudio = NULL;
	}
}

