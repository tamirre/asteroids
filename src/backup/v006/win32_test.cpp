#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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

// TODO: This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

// NOTE: Because of the support of the library starting at windows 8,
//       we only import certain chosen functions dynamically ourselves 
//       instead of linking them.

// NOTE: This is our support for XInputGetState (initialized to a stub)
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: This is our support for XInputSetState (initialized to a stub)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if(XInputLibrary)
    {
	XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
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
     
/*
First testing function for rendering pixels in different colors
 */
internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
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

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if(Buffer->Memory)
    {
	VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

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
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

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
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);
	    
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
            Result = DefWindowProc(Window, Message, WParam, LParam);	    
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

    Win32LoadXInput();
    
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "TestWindowClass";

    if(RegisterClass(&WindowClass))
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
            GlobalRunning = true;
	    int XOffset = 0;
	    int YOffset = 0;
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

		// TODO: Should we poll this more frequently?
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
		{
                    XINPUT_STATE ControllerState;
    		    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
			// NOTE: This controller is plugged in
			// TODO: See if ControllerState.dwPacketNumber increments to rapidly
			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

			bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
			bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
			bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);			
			bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
			bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
			bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
			bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

			int16 StickX = Pad->sThumbLX;
			int16 StickY = Pad->sThumbLY;
			if(AButton)
			{
            		    YOffset = YOffset + 2;
	        	}
		    }
		    else
		    {
			// NOTE: This controller is not available
		    }
		}
		
		RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
		
		HDC DeviceContext = GetDC(Window);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                ReleaseDC(Window, DeviceContext);
		
		++XOffset;

	    }
		                   
	}
	else
	{
	    // Optional logging
	}
    }
    else
    {
	// Optional logging
    }
      
    return 0;
}
