#pragma once

#define GIF_NUMBER_FRAMES (1)
#define MSF_GIF_IMPL

#include "third_party/include/msf_gif.h"
#include "raylib.h"

#include <time.h>

typedef struct GifRecorder {
	MsfGifState* gifState;
	bool recording;
	unsigned int frameCounter;
} GifRecorder;

void static inline GifRecordUpdate(GifRecorder* rec) 
{
	if (!rec->recording) return;
    int centiSeconds = (int)(GetFrameTime() * 100.0f / GIF_NUMBER_FRAMES);

    // Only write a frame when we have at least 1 centisecond
    if (centiSeconds <= 0) return;

    // capture screen (slow but simple)
    Image img = LoadImageFromScreen();

    msf_gif_frame(
        rec->gifState,
        (uint8_t*)img.data,
        centiSeconds,
        16,                 // quality (lower = faster)
        img.width * 4
    );

	UnloadImage(img);
}

void static inline GifRecordStart(GifRecorder* rec) 
{
	rec->recording = true;
	rec->frameCounter = 0;
	msf_gif_begin(rec->gifState, GetRenderWidth(), GetRenderHeight());
	TraceLog(LOG_INFO, "Start animated GIF recording");
}

void static inline GifRecordStop(GifRecorder* rec) 
{
	// Stop current recording and save file
	rec->recording = false;
	MsfGifResult result = msf_gif_end(rec->gifState);

	// Get time stamp
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char buffer[100];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M", t);
	SaveFileData(TextFormat("%s/scrn-rec-%s.gif", GetApplicationDirectory(), buffer), result.data, (unsigned int)result.dataSize);
	msf_gif_free(result);
	TraceLog(LOG_INFO, "Finish animated GIF recording");
}

void static inline ScreenShot() 
{
	// Get time stamp
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char buffer[100];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M", t);
	TakeScreenshot(TextFormat("bin/screenshot-%s.png", buffer));
}
