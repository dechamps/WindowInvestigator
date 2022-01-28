#include <Windows.h>
#include <TraceLoggingProvider.h>
#include <stdio.h>
#include <stdlib.h>
#include <dwmapi.h>

TRACELOGGING_DEFINE_PROVIDER(traceloggingProvider, "WindowMonitor", (0x500d9509, 0x6850, 0x440c, 0xad, 0x11, 0x6e, 0xa6, 0x25, 0xec, 0x91, 0xbc));

// Declarations reverse-engineered from twinui.dll
__declspec(dllimport) BOOL WINAPI IsShellManagedWindow(HWND hwnd);
__declspec(dllimport) BOOL WINAPI IsShellFrameWindow(HWND hwnd);
static const ATOM ATOM_OVERPANNING = 0xa91d;
// https://blog.adeltax.com/window-z-order-in-windows-10/
__declspec(dllimport) BOOL WINAPI GetWindowBand(HWND hWnd, PDWORD pdwBand);

typedef struct {
	// RudeWindowWin32Functions::GetClassNameW()
	wchar_t className[1024];

	// RudeWindowWin32Functions::GetExStyleFromWindow()
	DWORD extendedStyles;

	// RudeWindowWin32Functions::GetStyleFromWindow()
	DWORD styles;

	// RudeWindowWin32Functions::GetWindowRectForFullscreenCheck()
	RECT windowRect;
	RECT clientRect;

	// RudeWindowWin32Functions::InternalGetWindowText()
	wchar_t text[1024];

	// RudeWindowWin32Functions::IsAppWindow()
	BOOL isShellManagedWindow;
	BOOL isShellFrameWindow;

	// TODO: RudeWindowWin32Functions::IsHolographic() missing - the logic is not as trivial as the others
 
	// RudeWindowWin32Functions::IsOverpanning()
	BOOL overpanning;

	// RudeWindowWin32Functions::IsValidDesktopFullscreenWindow() (also isShellManagedWindow)
	DWORD band;
	BOOL hasNonRudeHWNDProperty;
	BOOL hasLivePreviewWindowProperty;
	BOOL hasTreatAsDesktopFullscreenProperty;
	
	// RudeWindowWin32Functions::IsWindow()
	BOOL isWindow;

	//  RudeWindowWin32Functions::IsWindowAlwaysOnTopDesktop() uses GetStyleFromWindow() and GetWindowBand()

	// RudeWindowWin32Functions::IsWindowCloaked()
	DWORD dwmIsCloaked;

	// RudeWindowWin32Functions::IsWindowMinimized()
	BOOL isIconic;

	// RudeWindowWin32Functions::IsWindowOnMonitor() missing as it's relative to a monitor - though windowRect might be enough to deduce its value

	// RudeWindowWin32Functions::IsWindowRelatedForFullscreen() missing because it takes a pair of windows

	// RudeWindowWin32Functions::IsWindowVisible()
	BOOL isVisible;

	// RudeWindowWin32Functions::MonitorFromWindow() missing
} WindowMonitor_WindowInfo;

static WindowMonitor_WindowInfo WindowMonitor_GetWindowInfo(HWND window) {
	WindowMonitor_WindowInfo windowInfo;

	SetLastError(NO_ERROR);
	GetClassNameW(window, windowInfo.className, sizeof(windowInfo.className) / sizeof(*windowInfo.className));
	const DWORD classNameError = GetLastError();
	if (classNameError != NO_ERROR)
		TraceLoggingWrite(traceloggingProvider, "classNameError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(classNameError, "ErrorCode"));

	SetLastError(NO_ERROR);
	windowInfo.extendedStyles = (DWORD)GetWindowLongPtrW(window, GWL_EXSTYLE);
	const DWORD extendedStylesError = GetLastError();
	if (extendedStylesError != NO_ERROR)
		TraceLoggingWrite(traceloggingProvider, "extendedStylesError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(extendedStylesError, "ErrorCode"));

	SetLastError(NO_ERROR);
	windowInfo.styles = (DWORD)GetWindowLongPtrW(window, GWL_STYLE);
	const DWORD stylesError = GetLastError();
	if (stylesError != NO_ERROR)
		TraceLoggingWrite(traceloggingProvider, "stylesError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(stylesError, "ErrorCode"));

	if (!GetWindowRect(window, &windowInfo.windowRect))
		TraceLoggingWrite(traceloggingProvider, "windowRectError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	if (!GetClientRect(window, &windowInfo.clientRect))
		TraceLoggingWrite(traceloggingProvider, "clientRectError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	SetLastError(NO_ERROR);
	InternalGetWindowText(window, windowInfo.text, sizeof(windowInfo.text) / sizeof(*windowInfo.text));
	const DWORD textError = GetLastError();
	if (textError != NO_ERROR)
		TraceLoggingWrite(traceloggingProvider, "textError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(textError, "ErrorCode"));

	windowInfo.isShellManagedWindow = IsShellManagedWindow(window);

	windowInfo.isShellFrameWindow = IsShellFrameWindow(window);

	windowInfo.overpanning = GetPropW(window, (LPCWSTR) (intptr_t) ATOM_OVERPANNING) != NULL;

	if (!GetWindowBand(window, &windowInfo.band))
		TraceLoggingWrite(traceloggingProvider, "windowBandError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	windowInfo.hasNonRudeHWNDProperty = GetPropW(window, L"NonRudeHWND") != NULL;
	
	windowInfo.hasLivePreviewWindowProperty = GetPropW(window, L"LivePreviewWindow") != NULL;

	windowInfo.hasTreatAsDesktopFullscreenProperty = GetPropW(window, L"TreatAsDesktopFullscreen") != NULL;

	windowInfo.isWindow = IsWindow(window);

	const HRESULT dwmIsCloakedResult = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &windowInfo.dwmIsCloaked, sizeof(windowInfo.dwmIsCloaked));
	if (!SUCCEEDED(dwmIsCloakedResult))
		TraceLoggingWrite(traceloggingProvider, "dwmIsCloakedError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexLong(dwmIsCloakedResult, "HRESULT"));

	windowInfo.isIconic = IsIconic(window);

	windowInfo.isVisible = IsWindowVisible(window);

	return windowInfo;
}

static void WindowMonitor_DiffWindowInfo(HWND window, const WindowMonitor_WindowInfo* oldWindowInfo, const WindowMonitor_WindowInfo* newWindowInfo) {
	if (wcscmp(oldWindowInfo->className, newWindowInfo->className) != 0)
		TraceLoggingWrite(traceloggingProvider, "WindowClassNameChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingWideString(oldWindowInfo->className, "OldClassName"), TraceLoggingWideString(newWindowInfo->className, "NewClassName"));

	if (oldWindowInfo->extendedStyles != newWindowInfo->extendedStyles)
		TraceLoggingWrite(traceloggingProvider, "WindowExtendedStylesChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->extendedStyles, "OldExtendedStyles"), TraceLoggingHexUInt32(newWindowInfo->extendedStyles, "NewExtendedStyles"));

	if (oldWindowInfo->styles != newWindowInfo->styles)
		TraceLoggingWrite(traceloggingProvider, "WindowStylesChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->styles, "OldStyles"), TraceLoggingHexUInt32(newWindowInfo->styles, "NewStyles"));

	if (!EqualRect(&oldWindowInfo->windowRect, &newWindowInfo->windowRect))
		TraceLoggingWrite(traceloggingProvider, "WindowRectChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->windowRect.left, "OldWindowRectLeft"), TraceLoggingLong(oldWindowInfo->windowRect.top, "OldWindowRectTop"),
			TraceLoggingLong(oldWindowInfo->windowRect.right, "OldWindowRectRight"), TraceLoggingLong(oldWindowInfo->windowRect.bottom, "OldWindowRectBottom"),
			TraceLoggingLong(newWindowInfo->windowRect.left, "NewWindowRectLeft"), TraceLoggingLong(newWindowInfo->windowRect.top, "NewWindowRectTop"),
			TraceLoggingLong(newWindowInfo->windowRect.right, "NewWindowRectRight"), TraceLoggingLong(newWindowInfo->windowRect.bottom, "NewWindowRectBottom"));

	if (!EqualRect(&oldWindowInfo->clientRect, &newWindowInfo->clientRect))
		TraceLoggingWrite(traceloggingProvider, "WindowClientRectChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->clientRect.left, "OldClientRectLeft"), TraceLoggingLong(oldWindowInfo->clientRect.top, "OldClientRectTop"),
			TraceLoggingLong(oldWindowInfo->clientRect.right, "OldClientRectRight"), TraceLoggingLong(oldWindowInfo->clientRect.bottom, "OldClientRectBottom"),
			TraceLoggingLong(newWindowInfo->clientRect.left, "NewClientRectLeft"), TraceLoggingLong(newWindowInfo->clientRect.top, "NewClientRectTop"),
			TraceLoggingLong(newWindowInfo->clientRect.right, "NewClientRectRight"), TraceLoggingLong(newWindowInfo->clientRect.bottom, "NewClientRectBottom"));

	if (wcscmp(oldWindowInfo->text, newWindowInfo->text) != 0)
		TraceLoggingWrite(traceloggingProvider, "WindowTextChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingWideString(oldWindowInfo->text, "OldWindowText"), TraceLoggingWideString(newWindowInfo->text, "NewWindowText"));

	if (oldWindowInfo->isShellManagedWindow != newWindowInfo->isShellManagedWindow)
		TraceLoggingWrite(traceloggingProvider, "WindowIsShellManagedWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isShellManagedWindow, "OldIsShellManagedWindow"), TraceLoggingBool(newWindowInfo->isShellManagedWindow, "NewIsShellManagedWindow"));

	if (oldWindowInfo->isShellFrameWindow != newWindowInfo->isShellFrameWindow)
		TraceLoggingWrite(traceloggingProvider, "WindowIsShellFrameWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isShellFrameWindow, "OldIsShellFrameWindow"), TraceLoggingBool(newWindowInfo->isShellFrameWindow, "NewIsShellFrameWindow"));

	if (oldWindowInfo->overpanning != newWindowInfo->overpanning)
		TraceLoggingWrite(traceloggingProvider, "WindowOverpanningChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->overpanning, "OldOverpanning"), TraceLoggingBool(newWindowInfo->overpanning, "NewOverpanning"));

	if (oldWindowInfo->band != newWindowInfo->band)
		TraceLoggingWrite(traceloggingProvider, "WindowBandChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingUInt32(oldWindowInfo->band, "OldBand"), TraceLoggingUInt32(newWindowInfo->band, "NewBand"));

	if (oldWindowInfo->hasNonRudeHWNDProperty != newWindowInfo->hasNonRudeHWNDProperty)
		TraceLoggingWrite(traceloggingProvider, "WindowHasNonRudeHWNDPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasNonRudeHWNDProperty, "OldHasNonRudeHWNDProperty"), TraceLoggingBool(newWindowInfo->hasNonRudeHWNDProperty, "NewHasNonRudeHWNDProperty"));

	if (oldWindowInfo->hasLivePreviewWindowProperty != newWindowInfo->hasLivePreviewWindowProperty)
		TraceLoggingWrite(traceloggingProvider, "WindowHasLivePreviewWindowPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasLivePreviewWindowProperty, "OldHasLivePreviewWindowProperty"), TraceLoggingBool(newWindowInfo->hasLivePreviewWindowProperty, "NewHasLivePreviewWindowProperty"));

	if (oldWindowInfo->hasTreatAsDesktopFullscreenProperty != newWindowInfo->hasTreatAsDesktopFullscreenProperty)
		TraceLoggingWrite(traceloggingProvider, "WindowHasTreatAsDesktopFullscreenPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasTreatAsDesktopFullscreenProperty, "OldHasTreatAsDesktopFullscreenProperty"), TraceLoggingBool(newWindowInfo->hasTreatAsDesktopFullscreenProperty, "NewHasTreatAsDesktopFullscreenProperty"));

	if (oldWindowInfo->isWindow != newWindowInfo->isWindow)
		TraceLoggingWrite(traceloggingProvider, "WindowIsWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isWindow, "OldIsWindow"), TraceLoggingBool(newWindowInfo->isWindow, "NewIsWindow"));

	if (oldWindowInfo->dwmIsCloaked != newWindowInfo->dwmIsCloaked)
		TraceLoggingWrite(traceloggingProvider, "WindowDwmIsCloakedChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingUInt32(oldWindowInfo->dwmIsCloaked, "OldDwmIsCloaked"), TraceLoggingUInt32(newWindowInfo->dwmIsCloaked, "NewDwmIsCloaked"));

	if (oldWindowInfo->isIconic != newWindowInfo->isIconic)
		TraceLoggingWrite(traceloggingProvider, "WindowIsIconicChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isIconic, "OldIsIconic"), TraceLoggingBool(newWindowInfo->isIconic, "NewIsIconic"));

	if (oldWindowInfo->isVisible != newWindowInfo->isVisible)
		TraceLoggingWrite(traceloggingProvider, "WindowIsVisibleChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isVisible, "OldIsVisible"), TraceLoggingBool(newWindowInfo->isVisible, "NewIsVisible"));
}

struct WindowMonitor_Window_s {
	HWND window;
	struct WindowMonitor_Window_s* next;
	BOOL seen;
	WindowMonitor_WindowInfo info;
};
typedef struct WindowMonitor_Window_s WindowMonitor_Window;

typedef struct {
	WindowMonitor_Window* foregroundWindow;
} State;

static void WindowMonitor_OnWindowCreate(HWND window, LPARAM lParam) {
	SetLastError(NO_ERROR);
	SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
	const DWORD setWindowLongError = GetLastError();
	if (setWindowLongError != NO_ERROR) {
		fprintf(stderr, "SetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", setWindowLongError);
		exit(EXIT_FAILURE);
	}
}

static State* WindowMonitor_GetWindowState(HWND window) {
	SetLastError(NO_ERROR);
	State* state = (State*)GetWindowLongPtrW(window, GWLP_USERDATA);
	const DWORD getWindowLongError = GetLastError();
	if (getWindowLongError != NO_ERROR) {
		fprintf(stderr, "GetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", getWindowLongError);
		exit(EXIT_FAILURE);
	}
	DWMWA_CLOAKED;
	return state;
}

static WindowMonitor_Window** WindowMonitor_SearchWindow(WindowMonitor_Window** window, HWND match) {
	for (; *window != NULL && (*window)->window != match; window = &(*window)->next);
	return window;
}

static void WindowMonitor_MarkAllWindowsUnseen(WindowMonitor_Window* window) {
	for (; window != NULL; window = window->next)
		window->seen = FALSE;
}

static void WindowMonitor_RemoveUnseenWindows(WindowMonitor_Window** nextWindow) {
	while (*nextWindow != NULL) {
		WindowMonitor_Window* const window = *nextWindow;
		if (window->seen) {
			nextWindow = &window->next;
			continue;
		}
		
		TraceLoggingWrite(traceloggingProvider, "WindowGone", TraceLoggingPointer(window->window, "HWND"));
		*nextWindow = window->next;
		free(window);
	}
}

typedef struct {
	WindowMonitor_Window** window;
	UINT32 zOrder;
} WindowMonitor_LogTopLevelWindows_State;

static BOOL CALLBACK WindowMonitor_LogTopLevelWindows_EnumWindowsProc(HWND window, LPARAM lParam) {
	if (!IsWindowVisible(window)) return TRUE;

	WindowMonitor_LogTopLevelWindows_State* const state = (WindowMonitor_LogTopLevelWindows_State *)lParam;

	BOOL isNewWindow = FALSE;
	if (*state->window == NULL || (*state->window)->window != window) {
		WindowMonitor_Window** const existingWindow = WindowMonitor_SearchWindow(state->window, window);
		WindowMonitor_Window* const previousWindow = *state->window;
		if (*existingWindow == NULL) {
			TraceLoggingWrite(traceloggingProvider, "NewWindow", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			WindowMonitor_Window* const newWindow = *state->window = malloc(sizeof(**state->window));
			if (newWindow == NULL) abort();
			newWindow->window = window;
			newWindow->seen = FALSE;
			isNewWindow = TRUE;
		}
		else {
			TraceLoggingWrite(traceloggingProvider, "WindowZOrderChanged", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			*state->window = *existingWindow;
			*existingWindow = (*existingWindow)->next;
		}
		(*state->window)->next = previousWindow;
	}

	WindowMonitor_Window* currentWindow = *state->window;

	const WindowMonitor_WindowInfo windowInfo = WindowMonitor_GetWindowInfo(window);
	if (!isNewWindow) WindowMonitor_DiffWindowInfo(window, &currentWindow->info, &windowInfo);
	currentWindow->info = windowInfo;

	if (currentWindow->seen) {
		fprintf(stderr, "Window 0x%p already seen!", window);
		exit(EXIT_FAILURE);
	}
	currentWindow->seen = TRUE;
	state->window = &(*state->window)->next;
	++state->zOrder;
	return TRUE;
}

static void WindowMonitor_LogTopLevelWindows(State* const state) {
	WindowMonitor_MarkAllWindowsUnseen(state->foregroundWindow);

	WindowMonitor_LogTopLevelWindows_State logTopLevelWindowsState;
	logTopLevelWindowsState.window = &state->foregroundWindow;
	logTopLevelWindowsState.zOrder = 0;
	if (EnumWindows(WindowMonitor_LogTopLevelWindows_EnumWindowsProc, (LPARAM)&logTopLevelWindowsState) == 0) {
		fprintf(stderr, "EnumWindows() failed [0x%x]\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	WindowMonitor_RemoveUnseenWindows(&state->foregroundWindow);
}

static LRESULT CALLBACK WindowMonitor_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(traceloggingProvider, "ReceivedMessage", TraceLoggingHexUInt32(uMsg, "uMsg"), TraceLoggingHexUInt64(wParam, "wParam"), TraceLoggingHexUInt64(lParam, "lParam"));

	if (uMsg == WM_CREATE) WindowMonitor_OnWindowCreate(hWnd, lParam);

	State* const state = WindowMonitor_GetWindowState(hWnd);
	if (state != NULL) WindowMonitor_LogTopLevelWindows(state);
	TraceLoggingWrite(traceloggingProvider, "Done");

	if (SetTimer(hWnd, 1, USER_TIMER_MINIMUM, NULL) == 0) {
		fprintf(stderr, "SetTimer failed() [0x%x]\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main(int argc, char** argv) {
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = WindowMonitor_WindowProcedure;
	windowClass.lpszClassName = L"WindowMonitor";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	State state;
	state.foregroundWindow = NULL;
	const HWND window = CreateWindowW(
		L"WindowMonitor",
		NULL,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		HWND_MESSAGE,
		NULL,
		NULL,
		&state
	);
	if (window == NULL) {
		fprintf(stderr, "CreateWindowW failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	if (!RegisterShellHookWindow(window)) {
		fprintf(stderr, "RegisterShellHookWindow failed\n");
		return EXIT_FAILURE;
	}

	APPBARDATA abd;
	abd.cbSize = sizeof(abd);
	abd.hWnd = window;
	abd.uCallbackMessage = WM_USER;
	if (!SHAppBarMessage(ABM_NEW, &abd)) {
		fprintf(stderr, "SHAppBarMessage(ABM_NEW) failed\n");
		return EXIT_FAILURE;
	}

	const HRESULT registerResult = TraceLoggingRegister(traceloggingProvider);
	if (!SUCCEEDED(registerResult)) {
		fprintf(stderr, "Unable to register tracing provider [0x%lx]\n", registerResult);
		return EXIT_FAILURE;
	}

	// Note: in practice TaskbarMon needs to run as administrator to get Realtime priority. Otherwise it gets silently demoted to High.
	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
		fprintf(stderr, "Unable to set realtime process priority [0x%lx]\n", GetLastError());

	const UINT shellhookMessage = RegisterWindowMessageW(L"SHELLHOOK");
	if (shellhookMessage == 0) {
		fprintf(stderr, "RegisterWindowMessageW(\"SHELLHOOK\") failed [0x%x]\n", GetLastError());
		return EXIT_FAILURE;
	}
	TraceLoggingWrite(traceloggingProvider, "Started", TraceLoggingHexUInt32(shellhookMessage));

	for (;;)
	{
		MSG message;
		BOOL result = GetMessage(&message, NULL, 0, 0);
		if (result == -1) {
			fprintf(stderr, "GetMessage failed [%x]\n", GetLastError());
			return EXIT_FAILURE;
		}
		if (result == 0)
			return EXIT_SUCCESS;
		DispatchMessage(&message);
	}
}
