#include <windows.h>
#include <stdio.h>
#include <common.h>

#include "ppu.h"

static HBITMAP bitmap;
static void* bitmapPtr;

#define PALETTE_SIZE 64

struct NesBitmap
{
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD bmiColors[PALETTE_SIZE];
};
NesBitmap gdiBitmapPalette;

#define SET_RGB(dest, blue, green, red) do {           \
    dest.rgbBlue     = blue;                           \
    dest.rgbGreen    = green;                          \
    dest.rgbRed      = red;                            \
    dest.rgbReserved = 0;                              \
  } while(0)

static void HacknesCreate(HWND hwnd)
{
  HDC hdc = GetDC(hwnd);
  if(hdc == NULL) {
    printf("HacknesCreate: GetDC failed (e=%d)\n", GetLastError());
    DestroyWindow(hwnd);
    return;
  }

  ZeroMemory(&gdiBitmapPalette.bmiHeader, sizeof(gdiBitmapPalette.bmiHeader));
  gdiBitmapPalette.bmiHeader.biSize         = sizeof(gdiBitmapPalette.bmiHeader);
  gdiBitmapPalette.bmiHeader.biWidth        = SCREEN_PIXEL_WIDTH;
  gdiBitmapPalette.bmiHeader.biHeight       = -SCREEN_PIXEL_HEIGHT; // Negative means top-to-bottom
  gdiBitmapPalette.bmiHeader.biPlanes       = 1;
  gdiBitmapPalette.bmiHeader.biBitCount     = 8;
  gdiBitmapPalette.bmiHeader.biCompression  = BI_RGB; // TODO: use another format
  gdiBitmapPalette.bmiHeader.biSizeImage    = 0;
  gdiBitmapPalette.bmiHeader.biClrUsed      = PALETTE_SIZE;
  gdiBitmapPalette.bmiHeader.biClrImportant = PALETTE_SIZE;

  for(ubyte i = 0; i < PPU_PALETTE_SIZE; i++) {
    SET_RGB(gdiBitmapPalette.bmiColors[i],
            ppuPalette[i].blue,
            ppuPalette[i].green,
            ppuPalette[i].red);
  }
  
  bitmap = CreateDIBSection(hdc, (BITMAPINFO*)&gdiBitmapPalette,
                            DIB_RGB_COLORS, &bitmapPtr, NULL, 0);
  if(bitmap == NULL) {
    printf("HacknesCreate: CreateDibSection failed (e=%d)\n", GetLastError());
    DestroyWindow(hwnd);
  }
  ReleaseDC(hwnd, hdc);
}
static void HacknesDelete(HWND hwnd)
{
  if(bitmap) {
    if(!DeleteObject(bitmap)) {
      printf("HacknesDelete: WARNING: DeleteObject failed (e=%d)\n", GetLastError());
    }
    bitmap = NULL;
  }
}

struct ScopedDC
{
  HDC hdc;
  ScopedDC(HDC hdc) : hdc(hdc)
  {
  }
  ~ScopedDC()
  {
    DeleteDC(hdc);
  }
};

void UpdateBitmap()
{
  {
    static ubyte color = 0;
    color++;
    if(color > PPU_PALETTE_SIZE) {
      color = 0;
    }

    for(int i = 0; i < SCREEN_PIXEL_WIDTH*SCREEN_PIXEL_HEIGHT; i++) {
      ((ubyte*)bitmapPtr)[i] = color;
    }
  }
  {
    static int pos = 0;
    for(int row = 30; row < 40; row++) {
      int rowOffset = row * SCREEN_PIXEL_WIDTH;
      for(int i = pos; i < pos+30; i++) {
        ((ubyte*)bitmapPtr)[rowOffset + (i%SCREEN_PIXEL_WIDTH)] = 0x13;
      }
    }
    pos++;
  }
}

struct WindowProperties
{
  int width;
  int height;
};
WindowProperties windowProperties;

bool ResizeWindowByContent(HWND hwnd, int width, int height)
{
  RECT windowRect;
  if(!GetClientRect(hwnd, &windowRect)) {
    printf("ResizeWindowByContent: Error: GetClientRect failed (e=%d)\n", GetLastError());
    return false;
  }
  printf("SizeWindowToScale: window %d x %d > content %d x %d\n",
         windowProperties.width, windowProperties.height,
         windowRect.right, windowRect.bottom);
  int windowDressingX = windowProperties.width - windowRect.right;
  int windowDressingY = windowProperties.height - windowRect.bottom;

  if(!SetWindowPos(hwnd, NULL, 0, 0,
                   width + windowDressingX,
                   height + windowDressingY, SWP_NOMOVE)) {
    printf("ResizeWindowByContent: Error: SetWindowPos failed (e=%d)\n", GetLastError());
    return false;
  }

  return true;
}

static void HacknesPaint(HWND hwnd)
{
  PAINTSTRUCT paint;
  HDC hdc = BeginPaint(hwnd, &paint);
  if(hdc == NULL) {
    printf("BeginPaint failed (e=%d)\n", GetLastError());
    DestroyWindow(hwnd);
    return;
  }

  printf("Painting...\n");
  
  {
    // TODO: can memoryDC be created in the WM_CREATE context, so
    //       it doesn't have to be created at every paint?
    ScopedDC memoryDC(CreateCompatibleDC(hdc));
    if(memoryDC.hdc == NULL) {
      printf("HacknesPaint: Error: CreateCompatibleDC failed (e=%d)\n", GetLastError());
      goto DONE;
    }
    {
      UpdateBitmap();

      /*
        TODO: find out if and when I should call this
      if(!GdiFlush()) {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      }
      */
      
      // TODO: should I save the old object and restore it?
      SelectObject(memoryDC.hdc, bitmap);

      RECT windowRect;
      GetClientRect(hwnd, &windowRect);

      // NOTE: if the window content width matches the SCREEN_PIXEL_WIDTH,
      //       then we could use BitBlt instead of StretchBlt, which may
      //       or may not be faster.  StretchBlt may already check for this
      //       optimization.
      //if(!BitBlt(hdc, 0, 0, SCREEN_PIXEL_WIDTH, SCREEN_PIXEL_HEIGHT,
      //memoryDC.hdc, 0, 0, SRCCOPY)) {
      //printf("BitBlt failed (e=%d)\n", GetLastError());
      //goto DONE;
      //}
        
      if(!StretchBlt(hdc, 0, 0, windowRect.right, windowRect.bottom,
                     memoryDC.hdc, 0, 0, SCREEN_PIXEL_WIDTH, SCREEN_PIXEL_HEIGHT, SRCCOPY)) {
        printf("StretchBlt failed (e=%d)\n", GetLastError());
        goto DONE;
      }
    }
  }

 DONE:
  if(!EndPaint(hwnd, &paint)) {
    printf("EndPaint failed (e=%d)\n", GetLastError());
    PostQuitMessage(1);
  }
}

static CRITICAL_SECTION windowSync;
static HWND windowToDestroy = NULL;

struct ThreadStartEvent
{
public:
  HANDLE event;
  bool signalSuccessStatus;

  // Called before starting the thread.
  // On error, the Check the "event" member will be NULL
  void SetupBeforeStart()
  {
    event = CreateEvent(NULL, TRUE, FALSE, NULL);
  }
  // Returns: false on error
  bool WaitForSignal()
  {
    DWORD result = WaitForSingleObject(event, INFINITE);
    if(result != WAIT_OBJECT_0) {
      printf("WaitForStartSignal: Error: WaitForSingleObject returned 0x%08X (e=%d)\n", result, GetLastError());
      // I think this is the only error code that guarantees
      // the other thread won't be using the event
      if(result == WAIT_ABANDONED) {
        Cleanup();
      }
      return true; // success
    }
    Cleanup();
    return signalSuccessStatus;
  }
  // Called by the new thread that was started to signal the event
  void SetSignal(bool success)
  {
    this->signalSuccessStatus = success;
    SetEvent(event);
  }
  // Automatically called in WaitForStartSignal if it succeeds.
  // Call this if you need to cleanup this event after calling
  // SetupBeforeStart and cannot call WaitForStartSignal.
  void Cleanup()
  {
    if(event != NULL) {
      CloseHandle(event);
      event = NULL;
    }
  }
};

static ThreadStartEvent graphicSetupDoneEvent;


LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch(msg) {
  case WM_CREATE:
    if(!ResizeWindowByContent(hwnd, SCREEN_PIXEL_WIDTH, SCREEN_PIXEL_HEIGHT)) {
      DestroyWindow(hwnd);
      return 1; // fail
    }
    HacknesCreate(hwnd);
    return 0;
  case WM_ERASEBKGND:
    //printf("WM_ERASEBKGND\n");
    // Ignore these messages
    return 0;
  case WM_PAINT:
    HacknesPaint(hwnd);
    return 0;
  case WM_CLOSE:
    printf("WM_CLOSE\n");
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    printf("WM_DESTROY\n");
    HacknesDelete(hwnd);
    EnterCriticalSection(&windowSync);
    if(windowToDestroy) {
      windowToDestroy = NULL;
    }
    LeaveCriticalSection(&windowSync);
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

// NOTE: you need to create the window on the same thread
//       that process the windows messages.

// Returns: false on error
static bool CreateHacknesWindow()
{
  // Set the starting size of the window to 300 x 300
  windowProperties.width = 300;
  windowProperties.height = 300;
  
  #define WINDOW_CLASS_NAME "HacknesWindowClass"
  
  {
    WNDCLASSEX windowClass;
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    windowClass.lpfnWndProc = WinProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = NULL;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hIconSm = NULL;
    ATOM classAtom = RegisterClassEx(&windowClass);
    if(classAtom == 0) {
      printf("CreateHacknesWindow: RegisterClassEx failed (e=%d)\n", GetLastError());
      return false; // fail
    }
  }

  // TODO: set a better position
  HWND window = CreateWindowEx(NULL, WINDOW_CLASS_NAME, "HackNES",
                               WS_OVERLAPPEDWINDOW, 200, 200,
                               windowProperties.width,
                               windowProperties.height,
                               NULL, NULL, NULL, NULL);
  if(window == NULL) {
    printf("CreateHacknesWindow: CreateWindowEx failed (e=%d)\n", GetLastError());
    return false; // fail
  }
  EnterCriticalSection(&windowSync);
  windowToDestroy = window;
  LeaveCriticalSection(&windowSync);

  ShowWindow(window, SW_SHOW);
  UpdateWindow(window);
  return true; // succes
}

DWORD MessageLoop()
{
  while(1) {
    MSG msg;
    BOOL bRet = GetMessage(&msg, NULL, 0, 0);
    if (bRet > 0) { // (bRet > 0 indicates a message that must be processed.)
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else if (bRet < 0) {  // (bRet == -1 indicates an error.)
      printf("WindowThread: GetMessage failed (e=%d)\n", GetLastError());
      return 1; // fail
    } else { // (bRet == 0 indicates "exit program".)
      printf("WindowThread: exit\n");
      return 0; // success
    }
  }
}

static DWORD WINAPI WindowThread(LPVOID lpParam)
{
  if(!CreateHacknesWindow()) {
    graphicSetupDoneEvent.SetSignal(false);
    return 1; // fail
  }
  graphicSetupDoneEvent.SetSignal(true);
  return MessageLoop();
}


static HANDLE messageLoopThreadHandle = NULL;

// TODO: need to plumb in a way for the WM_DESTROY event
//       to signal the CPU that we need to quit.
//       I might do this by passing in a callback function to
//       setupGraphics.
//       Note that the bitmapPtr needs to remain valid until
//       this callback indicates it is done with it.
// Returns: false on fail
bool setupGraphics()
{
  InitializeCriticalSection(&windowSync);
  graphicSetupDoneEvent.SetupBeforeStart();

  messageLoopThreadHandle = CreateThread(NULL, 0, &WindowThread, NULL, 0, NULL);
  if(messageLoopThreadHandle == NULL) {
    printf("setupGraphics: CreateThread for window message loop failed (e=%d)\n", GetLastError());
    graphicSetupDoneEvent.Cleanup();
    DeleteCriticalSection(&windowSync);
    return false; // fail
  }

  // TODO: log the timing
  
  printf("setupGraphics: waiting for graphics thread to start/setup...\n");
  if(!graphicSetupDoneEvent.WaitForSignal()) {
    printf("setupGraphics: Error: setup failed\n");
    graphicSetupDoneEvent.Cleanup();
    DeleteCriticalSection(&windowSync);
    return false;
  }
  
  return true; // success
}

void cleanupGraphics()
{
  if(messageLoopThreadHandle) {
    EnterCriticalSection(&windowSync);
    if(windowToDestroy) {
      printf("[DEBUG] cleanupGraphics: destroying window...\n");
      DestroyWindow(windowToDestroy);
      windowToDestroy = NULL;
    }
    LeaveCriticalSection(&windowSync);

    printf("cleanupGraphics: waiting for windows message loop to finish..\n");
    WaitForSingleObject(messageLoopThreadHandle, INFINITE);
  }
  
  DeleteCriticalSection(&windowSync);
}

ubyte* getPixelBuffer()
{
  return (ubyte*)bitmapPtr;
}
