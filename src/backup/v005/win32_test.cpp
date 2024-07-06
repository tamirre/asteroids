#include <windows.h>
#include <stdint.h>

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

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Windim;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Windim.Width = ClientRect.right - ClientRect.left;
    Windim.Height = ClientRect.bottom - ClientRect.top;

    return Windim;
}


//     
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
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
                           win32_offscreen_buffer Buffer)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
		  /*
		  X, Y, Width, Height,
		  X, Y, Width, Height,
		  */
		  0, 0, WindowWidth, WindowHeight,
		  0, 0, Buffer.Width, Buffer.Height,
		  Buffer.Memory,
                  &Buffer.Info,
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
	case WM_PAINT:
	{
	    PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
	    int X = Paint.rcPaint.left;
	    int Y = Paint.rcPaint.top;
    	    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    	    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
	    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	    
	    Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);
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
		RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
		
		HDC DeviceContext = GetDC(Window);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
					   GlobalBackbuffer);
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
