/* Include guard: this way the .h file never gets included more than once */
#if !defined(GAME_H)

/*
    DEBUG = 1 for debugging
    DEBUG = 0 fast boi

    INTERNAL = 1 for internal run 
    INTERNAL = 0 for release
    
 */

#if DEBUG
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else 
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct game_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide, Memory Order: BB GG RR XX
    void *Memory;   
    int Width;          
    int Height;
    int Pitch;
};

struct game_sound_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};


struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;
    
    real32 EndX;
    real32 EndY;
    
    bool IsAnalog;
    union
    {
        game_button_state Buttons[6];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
        
};

struct game_input
{
    game_controller_input Controllers[4];
};

struct game_state
{
    int ToneHz;
    int XOffset;
    int YOffset;
};

struct game_memory
{
    bool IsInitialized;

    uint64 PermanentStorageSize;
    void *PermanentStorage;

    uint64 TransientStorageSize;
    void *TransientStorage;
};

/* internal void GameClearSoundBuffer(game_sound_buffer *SoundBuffer); */

internal void GameOutputSound(game_sound_buffer *SoundBuffer);
    
internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

internal void GameUpdateAndRender(game_memory *Memory,
                                  game_input *Input,
                                  game_offscreen_buffer *Buffer,
                                  game_sound_buffer *SoundBuffer);

#define GAME_H
#endif
