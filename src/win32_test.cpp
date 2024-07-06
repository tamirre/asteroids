#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef int32    bool32;

typedef float real32;
typedef double real64;

#include "game.h"
#include "game.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_test.h"

// TODO: This is a global for now.
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE: Because of the support of the library starting at windows 8,
//       we only import certain chosen functions dynamically ourselves 
//       instead of linking them.

// NOTE: This is our support for XInputGetState (initialized to a stub)
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: This is our support for XInputSetState (initialized to a stub)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// NOTE: This is our support for directsound (initialized to a stub)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void 
DEBUGPlatformFreeFileMemory(void *Memory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
                {
                    
                    // File read successfully
                    Result.ContentSize = FileSize32;
                }
                else
                {
                    // Failed to read file
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

internal bool32
DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
{
    bool32 Result = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {

            // File written successfully
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            // Failed to write file
        }
        CloseHandle(FileHandle);
    }
    return Result;    
}


internal void
Win32LoadXInput(void)
{
    // TODO: test this on different windows versions
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }    
    if(!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }    
    if(XInputLibrary)
    {
	XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE: Load the library
    HMODULE DSoundLibrary =  LoadLibraryA("dsound.dll"); 

    if(DSoundLibrary)
    {
        // NOTE: Get a DirectSound object! - cooperative           
        direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // TODO: Double check if this works on different windows versions
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // NOTE: "Create" a primary buffer
                // TODO: DSBCAPS_GLOBALFOCUS?

                // NOTE: This is not really a "Buffer", it's a handle to the soundcard to tell it the sound format.
                // The secondary buffer is what we write to.
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,  &PrimaryBuffer, 0)))
                {

                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        // NOTE: We have finally set the format!
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostic
                    }                        
                }
                else
                {
                    // TODO: Diagnostic                    
                }
            }
            else
            {
                // TODO: Diagnostic                
            }

            // NOTE: "Create" a secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Secondary buffer created successfully .\n");
            }
            else
            {
                // TODO: Diagnostic
            }

        }
        else
        {
            // TODO: Diagnostic
        }

    }
    else
    {
        // TODO: Diagnostic
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Windim;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Windim.Width = ClientRect.right - ClientRect.left;
    Windim.Height = ClientRect.bottom - ClientRect.top;

    return Windim;
}
     

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    // if(Buffer->Memory)
    // {
    //     VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    // }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    // NOTE: When the biHeight field is negative, this is the clue
    // to Windows to treat this bitmap as top-down, not bottom-up, meaning
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left.
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    int BitmapMemorySize = (Width * Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
			   HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
		  /*
		  X, Y, Width, Height,
		  X, Y, Width, Height,
		  */
		  0, 0, WindowWidth, WindowHeight,
		  0, 0, Buffer->Width, Buffer->Height,
		  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}


void
win32_ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size,
                                0)))
    {
        uint8 *SampleDest = (uint8 *)Region1;
        for(int ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *SampleDest++ = 0;        
        }
        SampleDest = (uint8 *)Region2;
        for(int ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *SampleDest++ = 0;        
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }    
}

void
win32_FillSoundBuffer(win32_sound_output *SoundOutput, game_sound_buffer *SourceBuffer, DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size,
                                0)))
    {
        // TODO: assert that regionsize is valid

        // TODO: Collapse these two loops
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *SampleDest = (int16 *)Region1;
        int16 *SampleSource = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {

            *SampleDest++ = *SampleSource++;
            *SampleDest++ = *SampleSource++;            
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        SampleDest = (int16 *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {                            
            *SampleDest++ = *SampleSource++;
            *SampleDest++ = *SampleSource++;            
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
    else
    {
                // Optional logging
    }
}

void internal
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state *OldState, DWORD ButtonBit,
                                game_button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

LRESULT CALLBACK
MainWindowCallback(HWND Window,
		   UINT Message,
		   WPARAM WParam,
		   LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_SIZE:
	{
	    OutputDebugStringA("WM_SIZE\n");
	} break;
	case WM_DESTROY:
	{
	    // TODO: Handle this as an error -> recreate window?
	    GlobalRunning = false;
	    OutputDebugStringA("WM_DESTROY\n");
	} break;
	case WM_CLOSE:
	{
	    // TODO: Handle this with a message to the user?
	    GlobalRunning = false;	    
	    OutputDebugStringA("WM_CLOSE\n");	    
	} break;
	case WM_ACTIVATEAPP:
	{
	    OutputDebugStringA("WM_ACTIVATEAPP\n");	    
	} break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
	{
	    uint32 VKCode = WParam;	
            bool32 WasDown = ((LParam & (1 << 30)) != 0);
            bool32 IsDown = ((LParam & (1 << 31)) == 0);
	    
	    if(VKCode == 'W')
	    {		
	    }
	    else if(VKCode == 'A')
	    {
	    }
	    else if(VKCode == 'S')
	    {
	    }
	    else if(VKCode == 'D')
	    {
	    }
	    else if(VKCode == 'Q')
	    {
	    }
	    else if(VKCode == 'E')
	    {
	    }
	    else if(VKCode == VK_UP)
	    {
	    }
	    else if(VKCode == VK_DOWN)
	    {
	    }
	    else if(VKCode == VK_LEFT)
	    {
	    }
	    else if(VKCode == VK_RIGHT)
	    {
	    }
	    else if(VKCode == VK_ESCAPE)
	    {
		OutputDebugStringA("Escape: ");
		if(IsDown)
		{
                    OutputDebugStringA("IsDown ");
		}
		if(WasDown)
		{
		    OutputDebugStringA("WasDown");
		}
		OutputDebugStringA("\n");
		
	    }
	    else if(VKCode == VK_SPACE)
	    {
	    }
            bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
            if((VKCode == VK_F4) && AltKeyWasDown)
	    {
                GlobalRunning = false;
	    }

	} break;    
	    
	case WM_PAINT:
	{
	    PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
	    int X = Paint.rcPaint.left;
	    int Y = Paint.rcPaint.top;
    	    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    	    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
	    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	    
	    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
	    EndPaint(Window, &Paint);	    
	} break;
	default:
	{
            Result = DefWindowProcA(Window, Message, WParam, LParam);	    
	} break;	
    }

    return Result;
}



int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     CommandLine,
        int       ShowCode)
{

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    Win32LoadXInput();
    
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "TestWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
	     CreateWindowEx(0, 
                            WindowClass.lpszClassName, 
                            "Test", 
                            WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            0, 
                            0, 
                            Instance, 
                            0);
	if(Window)
	{
            // NOTE: Since we specified CS_OWNDC, we can just
            // get one device context and use it forever because
            // we are not sharing it with anyone.

            HDC DeviceContext = GetDC(Window);

            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
            // Pulled out the memory allocation compared to handmade hero (?) 
            int16* Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE |
                                                   MEM_COMMIT, PAGE_READWRITE);
            
            
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            win32_ClearSoundBuffer(&SoundOutput);
            
#if INTERNAL
            LPVOID BaseAdress = (LPVOID)Terabytes((uint64)2);
#else
            LPVOID BaseAdress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);            
            GameMemory.TransientStorageSize = Gigabytes((uint64)4);            
            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

            GameMemory.PermanentStorage = (int16 *)VirtualAlloc(BaseAdress, TotalSize,
                                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
            
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            game_input Input[2] = {};
            game_input *NewInput = &Input[0];
            game_input *OldInput = &Input[1];
                
            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);
            uint64 LastCycleCount = __rdtsc();
            
            GlobalRunning = true;
            
	    while(GlobalRunning)
	    {
		MSG Message;
                
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
		    if(Message.message == WM_QUIT)
		    {
			GlobalRunning = false;
		    }
		    TranslateMessage(&Message);
		    DispatchMessage(&Message);
		}


                int MaxControllerCount = XUSER_MAX_COUNT;
                if(MaxControllerCount > ArrayCount(Input->Controllers))
                {
                    MaxControllerCount = ArrayCount(Input->Controllers);
                }
                
		// TODO: Should we poll this more frequently?
                for (DWORD ControllerIndex = 0;
                     ControllerIndex < MaxControllerCount;
                     ++ControllerIndex)
		{

                    game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
                    game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];
                    
                    XINPUT_STATE ControllerState;
    		    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
			// NOTE: This controller is plugged in
			// TODO: See if ControllerState.dwPacketNumber increments to rapidly
			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

			// bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			// bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			// bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			// bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
		 	// bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			// bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
			// bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			// bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);			
			// bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
			// bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
			// bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
			// bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        NewController->IsAnalog = true;
                        
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Down, XINPUT_GAMEPAD_DPAD_DOWN,
                                                        &NewController->Down);

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Right, XINPUT_GAMEPAD_DPAD_RIGHT,
                                                        &NewController->Right);

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Left, XINPUT_GAMEPAD_DPAD_LEFT,
                                                        &NewController->Left);

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Up, XINPUT_GAMEPAD_DPAD_UP,
                                                        &NewController->Up);

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->LeftShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                        &NewController->LeftShoulder);

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->RightShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                        &NewController->RightShoulder);


                        real32 X;
                        if(Pad->sThumbLX < 0)
                        {
                            X = (real32)Pad->sThumbLX / 32768.0f;
                        }
                        else
                        {
                            X = (real32)Pad->sThumbLX / 32767.0f;
                        }
                        NewController->MinX = NewController->MaxX = NewController->EndX = X;

                        real32 Y;
                        if(Pad->sThumbLY < 0)
                        {
                            Y = (real32)Pad->sThumbLY / 32768.0f;
                        }
                        else
                        {
                            Y = (real32)Pad->sThumbLY / 32767.0f;
                        }
                        NewController->MinY = NewController->MaxY = NewController->EndY = Y;
                        
			// int16 StickX = (real32)Pad->sThumbLX / X;
			// int16 StickY = (real32)Pad->sThumbLY / X;

			// if(StickY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || StickY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
			// {
            		//     YOffset += StickY / 4096;
	        	// }

			// if(StickX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || StickX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
			// {
            		//     XOffset += StickX / 4096;
	        	// }

                        // SoundOutput.ToneHz += (int)(256.0f*((real32)StickY / 3000000.0f));
                        // SoundOutput.ToneHz += (int)(256.0f*((real32)StickX / 3000000.0f));
                        // if (SoundOutput.ToneHz <= 0)
                        // {
                        //     SoundOutput.ToneHz = 1;
                        // }
                        // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;

		    }
		    else
		    {
			// NOTE: This controller is not available
		    }
		}

                
                DWORD PlayCursor;
                DWORD WriteCursor;
                DWORD ByteToLock;
                DWORD BytesToWrite;
                DWORD TargetCursor;
                bool SoundIsValid = false;
                if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);

                    if(ByteToLock == PlayCursor)
                    {
                        BytesToWrite = 0;
                    }
                    else if(ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    SoundIsValid = true;

                }
                
               
                game_sound_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = 48000;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;
            
                game_offscreen_buffer GameBuffer = {};
                GameBuffer.Memory = GlobalBackbuffer.Memory;
                GameBuffer.Width = GlobalBackbuffer.Width;
                GameBuffer.Height = GlobalBackbuffer.Height;
                GameBuffer.Pitch = GlobalBackbuffer.Pitch;

		GameUpdateAndRender(&GameMemory, NewInput, &GameBuffer, &SoundBuffer);

                if(SoundIsValid)
                {
                    win32_FillSoundBuffer(&SoundOutput, &SoundBuffer, ByteToLock, BytesToWrite);
                }
                
                // NOTE: DirectSound output test:

                
                GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                // VirtualFree(0, SoundOutput.SecondaryBufferSize, MEM_RELEASE | MEM_DECOMMIT);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                ReleaseDC(Window, DeviceContext);

                int64 EndCycleCount = __rdtsc();                
                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                int32 MSPerFrame = (int32)(1000*CounterElapsed / PerfCountFrequency);
                int32 FPS = PerfCountFrequency/CounterElapsed;
                int32 MCPF = CyclesElapsed / (1000*1000);
                
                char Buffer[256];
                wsprintf(Buffer, "%dms/f, %df/s, %dmc/f\n", MSPerFrame, FPS, MCPF);
                OutputDebugStringA(Buffer);
                
                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;

                game_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
	    }
		                   
	}

    }
    else
    {
	// Optional logging
    }
      
    return 0;
}
