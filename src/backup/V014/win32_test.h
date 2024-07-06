#if !defined(WIN32_GAME_H)

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide, Memory Order: BB GG RR XX
    BITMAPINFO Info;
    void *Memory;   
    int Width;          
    int Height;
    int Pitch;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int WavePeriod; 
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
    real32 tSine;
};


#define WIN32_GAME_H
#endif
