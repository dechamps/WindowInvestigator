#include <Windows.h>

// Declarations reverse-engineered from twinui.dll
__declspec(dllimport) BOOL WINAPI IsShellManagedWindow(HWND hwnd);
__declspec(dllimport) BOOL WINAPI IsShellFrameWindow(HWND hwnd);
static const ATOM ATOM_OVERPANNING = 0xa91d;
// https://blog.adeltax.com/window-z-order-in-windows-10/
__declspec(dllimport) BOOL WINAPI GetWindowBand(HWND hWnd, PDWORD pdwBand);
