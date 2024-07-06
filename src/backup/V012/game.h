/* Include guard: this way the .h file never gets included more than once */
#if !defined(GAME_H)

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

/* internal void GameClearSoundBuffer(game_sound_buffer *SoundBuffer); */

internal void GameOutputSound(game_sound_buffer *SoundBuffer);
    
internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer, int XOffset, int YOffset);


#define GAME_H
#endif
