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
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#define GAME_H
#endif
