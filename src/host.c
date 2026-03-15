#include <sys/stat.h>
#include <dlfcn.h>
#include <unistd.h>

#include "game.h"

typedef struct GameCode {
	void* handle;
	time_t lastWriteTime;
	GameUpdateFn Update;
	GameDrawFn Draw;
} GameCode;

time_t GetLastWriteTime(const char* path)
{
	struct stat attr;
	if (stat(path, &attr) == 0)
		return attr.st_mtime;

	return 0;
}

void CopyFile(const char* src, const char* dst)
{
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "cp %s %s", src, dst);
	system(cmd);
}

GameCode LoadGameCode()
{
	GameCode game = {0};
	CopyFile("./game.so", "./game_tmp.so");
	game.handle = dlopen("./game_tmp.so", RTLD_NOW);
	if (!game.handle)
	{
		printf("dlopen error: %s\n", dlerror());
		return game;
	}
	game.Update = dlsym(game.handle, "GameUpdate");
	game.Draw   = dlsym(game.handle, "GameDraw");
	game.lastWriteTime = GetLastWriteTime("./game.so");
	printf("Game code loaded\n");

	return game;
}

void UnloadGameCode(GameCode* game)
{
	if (game->handle)
	{
		dlclose(game->handle);
		game->handle = NULL;
	}
}

int main()
{
	InitWindow(1280, 720, "Raylib Hot Reload");
	SetTargetFPS(60);
	GameState gameState = {0};
	Options options = {0};
	Audio audio = {0};

	TextureAtlas atlas = {0};
	SpriteMask spriteMasks[128] = {0};

	RenderTexture2D scene = {0};
	RenderTexture2D litScene = {0};

	Shader shader = {0};
	Shader lightShader = {0};

	int currentSongtrackID = 0;
	Rectangle currentCollision = {0};
	bool shouldExit = false;
	GameMemory mem =
	{
		.gameState = &gameState,
		.options = &options,
		.audio = &audio,
		.atlas = &atlas,
		.spriteMasks = spriteMasks,

		.scene = &scene,
		.litScene = &litScene,

		.shader = &shader,
		.lightShader = &lightShader,

		.currentSongtrackID = &currentSongtrackID,
		.currentCollision = &currentCollision,
		.shouldExit = &shouldExit
	};
	GameCode game = LoadGameCode();

	while (!WindowShouldClose())
	{
		time_t newWriteTime = GetLastWriteTime("./game.so");

		if (newWriteTime != game.lastWriteTime)
		{
			printf("Hot reloading game...\n");

			UnloadGameCode(&game);
			game = LoadGameCode();
		}

		float dt = GetFrameTime();

		if (game.Update) game.Update(&mem, dt);
		if (game.Draw) game.Draw(&mem);
	}

	UnloadGameCode(&game);
	CloseWindow();
}
