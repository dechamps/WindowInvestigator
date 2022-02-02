#include <Windows.h>

#ifdef USER32_PRIVATE_STUB
#define USER32_PRIVATE_ENTRY_POINT __declspec(dllexport)
#define USER32_PRIVATE_ENTRY_POINT_DEF {}
#pragma warning(disable: 4100 4716)
#else
#define USER32_PRIVATE_ENTRY_POINT __declspec(dllimport)
#define USER32_PRIVATE_ENTRY_POINT_DEF
#endif

// Declarations reverse-engineered from twinui.dll
USER32_PRIVATE_ENTRY_POINT BOOL WINAPI IsShellManagedWindow(HWND hwnd) USER32_PRIVATE_ENTRY_POINT_DEF;
USER32_PRIVATE_ENTRY_POINT BOOL WINAPI IsShellFrameWindow(HWND hwnd) USER32_PRIVATE_ENTRY_POINT_DEF;
static const ATOM ATOM_OVERPANNING = 0xa91d;
// https://blog.adeltax.com/window-z-order-in-windows-10/
USER32_PRIVATE_ENTRY_POINT BOOL WINAPI GetWindowBand(HWND hWnd, PDWORD pdwBand) USER32_PRIVATE_ENTRY_POINT_DEF;
