#include "game.h"

// internal void
// GameClearSoundBuffer(game_sound_buffer *SoundBuffer)
// {
//     SoundBuffer->SamplesPerSecond = 48000;
//     SoundBuffer->SampleCount = 2 * SoundBuffer->SamplesPerSecond;
//     // SoundBuffer->Samples = malloc((int16)SoundBuffer->SampleCount * sizeof(int16));
//     // memset(SoundBuffer->Samples, 0, SoundBuffer->SampleCount * sizeof(int16));
// }

internal void
GameOutputSound(game_sound_buffer *SoundBuffer, int ToneHz)
{

    local_persist real32 tSine;
    // int ToneHz = 256;
    int ToneVolume = 1000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz; 

    int16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {

        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;        
    }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
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
/*
First testing function for rendering pixels in different colors
 */
internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer, int XOffset, int YOffset, int ToneHz)
{
    // GameClearSoundBuffer(SoundBuffer);
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}

