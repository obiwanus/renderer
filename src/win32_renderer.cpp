#include "renderer_platform.h"

#include "renderer.h"
#include <windows.h>
#include <intrin.h>

#include "win32_renderer.h"

global bool32 g_running;

global BITMAPINFO g_bitmap_info;
global LARGE_INTEGER g_performance_frequency;

global GameOffscreenBuffer g_game_backbuffer;

#include "renderer.cpp"

FileReadResult PlatformReadEntireFile(char *filename) {
  FileReadResult result = {};

  HANDLE file_handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                                  0,  // Security attributes
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                  0);  // Template file

  if (file_handle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file_handle, &file_size)) {
      result.memory_size = file_size.QuadPart;
      result.memory =
          VirtualAlloc(0, result.memory_size, MEM_COMMIT, PAGE_READWRITE);
      DWORD bytes_read = 0;

      ReadFile(file_handle, result.memory, (u32)result.memory_size, &bytes_read,
               0);

      CloseHandle(file_handle);

      return result;
    } else {
      OutputDebugStringA("Cannot get file size\n");
      // GetLastError() should help
    }
  } else {
    OutputDebugStringA("Cannot read from file\n");
    // GetLastError() should help
  }

  return result;
}

internal void Win32UpdateWindow(HDC hdc) {
  StretchDIBits(
      hdc, 0, 0, g_game_backbuffer.width, g_game_backbuffer.height,  // dest
      0, 0, g_game_backbuffer.width, g_game_backbuffer.height,       // src
      g_game_backbuffer.memory, &g_bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

internal void Win32ResizeClientWindow(HWND window) {
  RECT client_rect;
  GetClientRect(window, &client_rect);
  int width = client_rect.right - client_rect.left;
  int height = client_rect.bottom - client_rect.top;

  if (width > g_game_backbuffer.max_width) width = g_game_backbuffer.max_width;

  if (height > g_game_backbuffer.max_height)
    height = g_game_backbuffer.max_height;

  g_game_backbuffer.width = width;
  g_game_backbuffer.height = height;

  g_bitmap_info.bmiHeader.biWidth = width;
  g_bitmap_info.bmiHeader.biHeight = -height;
}

inline LARGE_INTEGER Win32GetWallClock() {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);

  return result;
}

inline r32 Win32GetMsElapsed(LARGE_INTEGER start, LARGE_INTEGER End) {
  r32 result = 1000.0f * (r32)(End.QuadPart - start.QuadPart) /
               (r32)g_performance_frequency.QuadPart;
  return result;
}

LRESULT CALLBACK
Win32WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;

  switch (uMsg) {
    case WM_SIZE: {
      Win32ResizeClientWindow(hwnd);
    } break;

    case WM_CLOSE: {
      g_running = false;
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint = {};
      HDC hdc = BeginPaint(hwnd, &Paint);
      Win32UpdateWindow(hdc  // we update the whole window, always
                        );
      EndPaint(hwnd, &Paint);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      if (wParam == VK_ESCAPE) {
        g_running = false;
      }
      // Assert(!"Keyboard input came in through a non-dispatch message!");
    } break;

    default: { result = DefWindowProc(hwnd, uMsg, wParam, lParam); } break;
  }

  return result;
}

internal void Win32ProcessPendingMessages() {
  MSG message;
  while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
    // Get keyboard messages
    switch (message.message) {
      case WM_QUIT: {
        g_running = false;
      } break;

      default: {
        TranslateMessage(&message);
        DispatchMessageA(&message);
      } break;
    }
  }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASS window_class = {};
  window_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  window_class.lpfnWndProc = Win32WindowProc;
  window_class.hInstance = hInstance;
  window_class.lpszClassName = "rendererWindowClass";

  // TODO: query monitor refresh rate
  int target_fps = 30;
  r32 target_mspf = 1000.0f / (r32)target_fps;  // Target ms per frame

  // Set target sleep resolution
  {
#define TARGET_SLEEP_RESOLUTION 1  // 1-millisecond target resolution

    TIMECAPS tc;
    UINT timer_res;

    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
      OutputDebugStringA("Cannot set the sleep resolution\n");
      exit(1);
    }

    timer_res = min(max(tc.wPeriodMin, TARGET_SLEEP_RESOLUTION), tc.wPeriodMax);
    timeBeginPeriod(timer_res);
  }

  QueryPerformanceFrequency(&g_performance_frequency);

  if (RegisterClass(&window_class)) {
    const int window_width = 1000;
    const int window_height = 1000;

    HWND window = CreateWindow(window_class.lpszClassName, 0,
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                               CW_USEDEFAULT, window_width, window_height, 0, 0,
                               hInstance, 0);

    // We're not going to release it as we use CS_OWNDC
    HDC hdc = GetDC(window);

    if (window) {
      g_running = true;

      LARGE_INTEGER last_timestamp = Win32GetWallClock();

      // Init backbuffer
      {
        g_game_backbuffer.max_width = 2000;
        g_game_backbuffer.max_height = 1500;
        g_game_backbuffer.bytes_per_pixel = 4;

        int BufferSize = g_game_backbuffer.max_width *
                         g_game_backbuffer.max_height *
                         g_game_backbuffer.bytes_per_pixel;
        // TODO: put it into the game_code memory?
        g_game_backbuffer.memory =
            VirtualAlloc(0, BufferSize, MEM_COMMIT, PAGE_READWRITE);

        g_bitmap_info.bmiHeader.biSize = sizeof(g_bitmap_info.bmiHeader);
        g_bitmap_info.bmiHeader.biPlanes = 1;
        g_bitmap_info.bmiHeader.biBitCount = 32;
        g_bitmap_info.bmiHeader.biCompression = BI_RGB;

        // Set up proper values of buffers based on actual client size
        Win32ResizeClientWindow(window);
      }

      // Main loop
      while (g_running) {
        Win32ProcessPendingMessages();

        Render();

        Win32UpdateWindow(hdc);

        // Enforce FPS
        // TODO: for some reason Time to sleep drops every now and again,
        // disabling gradient solves or masks this, though I don't see
        // any reason why this might happen
        {
          r32 MillisecondsElapsed =
              Win32GetMsElapsed(last_timestamp, Win32GetWallClock());
          u32 TimeToSleep = 0;

          if (MillisecondsElapsed < target_mspf) {
            TimeToSleep = (u32)(target_mspf - MillisecondsElapsed);
            Sleep(TimeToSleep);

            while (MillisecondsElapsed < target_mspf) {
              MillisecondsElapsed =
                  Win32GetMsElapsed(last_timestamp, Win32GetWallClock());
            }
          } else {
            OutputDebugStringA("Frame missed\n");
          }

          last_timestamp = Win32GetWallClock();
        }
      }
    }
  } else {
    // TODO: logging
    OutputDebugStringA("Couldn't register window class");
  }

  return 0;
}