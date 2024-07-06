#include "game.h"

internal void
GameOutputSound(game_sound_buffer *SoundBuffer, int ToneHz)
{

    local_persist real32 tSine;
    // int ToneHz = 256;
    int ToneVolume = 1000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz; 

    int16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
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


internal game_state *
GameStartup(void)
{
    game_state *GameState = new game_state;
    if(GameState)
    {
        GameState->ToneHz = 256;
        GameState->XOffset = 0;
        GameState->YOffset = 0;
    }
    return GameState;
};

internal void
GameShutdown(game_state *GameState)
{
    delete GameState;
};


/*
First testing function for rendering pixels in different colors
 */
internal void
GameUpdateAndRender(game_memory *Memory, game_input *Input,
                    game_offscreen_buffer *Buffer,
                    game_sound_buffer *SoundBuffer)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    

    if(!Memory->IsInitialized)
    {

        char *Filename = __FILE__;

        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents)
        {
            DEBUGPlatformWriteEntireFile("w:/test.out", File.ContentSize, File.Contents);
            DEBUGPlatformFreeFileMemory(File.Contents);
        }
        
        GameState->ToneHz = 256;
        // GameState->XOffset = 0;
        // GameState->YOffset = 0;
        Memory->IsInitialized = true;
    }
    
    game_controller_input *Input0 = &Input->Controllers[0];
    if(Input0->IsAnalog)
    {
        GameState->ToneHz = 256 + (int)(128.0f*(Input0->EndX)) + (int)(128.0f*(Input0->EndY));

        GameState->YOffset += (int)(4.0f*(Input0->EndY));
        GameState->XOffset += (int)(4.0f*(Input0->EndX));
    }
    else
    {
        
    }

    // int AButtonEndedDown;
    // int AButtonHalfTransitionCount;
    if(Input0->Down.EndedDown)
    {
        GameState->YOffset -= 4;
    }
    if(Input0->Up.EndedDown)
    {
        GameState->YOffset += 4;
    }
    if(Input0->Left.EndedDown)
    {
        GameState->XOffset -= 4;
    }
    if(Input0->Right.EndedDown)
    {
        GameState->XOffset += 4;
    }

    GameOutputSound(SoundBuffer, GameState->ToneHz);
    RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);
}

