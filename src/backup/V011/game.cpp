#include "game.h"

/*
First testing function for rendering pixels in different colors
 */
internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; Y++)
    {
	uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer->Width; X++)
	{
	    /*
              Pixel in memory: BB GG RR xx -AND NOT- RR GG BB xx 
              (xx at the end is buffer, because of 4 bytes per pixel so the size must be 8x4=32) 
              Why? LITTLE ENDIAN ARCHITECTURE
              0x xxBBGGRR
	     */	    
	    uint8 Blue = (X + XOffset);
	    uint8 Green = (Y + YOffset);

            *Pixel++ = ((Green << 8) | Blue);
	}
	Row += Buffer->Pitch;
    }
}

