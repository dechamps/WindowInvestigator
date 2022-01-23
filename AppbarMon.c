#include <Windows.h>
#include <TraceLoggingProvider.h>
#include <stdio.h>
#include <stdlib.h>

TRACELOGGING_DEFINE_PROVIDER(traceloggingProvider, "AppbarMon", (0x500d9509, 0x6850, 0x440c, 0xad, 0x11, 0x6e, 0xa6, 0x25, 0xec, 0x91, 0xbc));

struct Window_s {
	HWND window;
	struct Window_s* next;
	BOOL seen;
};
typedef struct Window_s Window;

typedef struct {
	Window* foregroundWindow;
} State;

static void OnWindowCreate(HWND window, LPARAM lParam) {
	SetLastError(NO_ERROR);
	SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
	const DWORD setWindowLongError = GetLastError();
	if (setWindowLongError != NO_ERROR) {
		fprintf(stderr, "SetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", setWindowLongError);
		exit(EXIT_FAILURE);
	}
}

static State* GetWindowState(HWND window) {
	SetLastError(NO_ERROR);
	State* state = (State*)GetWindowLongPtrW(window, GWLP_USERDATA);
	const DWORD getWindowLongError = GetLastError();
	if (getWindowLongError != NO_ERROR) {
		fprintf(stderr, "GetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", getWindowLongError);
		exit(EXIT_FAILURE);
	}
	return state;
}

static Window** SearchWindow(Window** window, HWND match) {
	for (; *window != NULL && (*window)->window != match; window = &(*window)->next);
	return window;
}

static void MarkAllWindowsUnseen(Window* window) {
	for (; window != NULL; window = window->next)
		window->seen = FALSE;
}

static void RemoveUnseenWindows(Window** nextWindow) {
	while (*nextWindow != NULL) {
		Window* const window = *nextWindow;
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
	Window** window;
	UINT32 zOrder;
} LogTopLevelWindows_State;

static BOOL CALLBACK LogTopLevelWindows_EnumWindowsProc(HWND window, LPARAM lParam) {
	if (!IsWindowVisible(window)) return TRUE;

	LogTopLevelWindows_State* const state = (LogTopLevelWindows_State *)lParam;

	if (*state->window == NULL || (*state->window)->window != window) {
		Window** const existingWindow = SearchWindow(state->window, window);
		Window* const previousWindow = *state->window;
		if (*existingWindow == NULL) {
			TraceLoggingWrite(traceloggingProvider, "NewWindow", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			*state->window = malloc(sizeof(**state->window));
			if (*state->window == NULL) abort();
			(*state->window)->window = window;
		}
		else {
			TraceLoggingWrite(traceloggingProvider, "WindowZOrderChanged", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			*state->window = *existingWindow;
			*existingWindow = (*existingWindow)->next;
		}
		(*state->window)->next = previousWindow;
	}

	(*state->window)->seen = TRUE;
	state->window = &(*state->window)->next;
	++state->zOrder;
	return TRUE;
}

static void LogTopLevelWindows(State* const state) {
	MarkAllWindowsUnseen(state->foregroundWindow);

	LogTopLevelWindows_State logTopLevelWindowsState;
	logTopLevelWindowsState.window = &state->foregroundWindow;
	logTopLevelWindowsState.zOrder = 0;
	if (EnumWindows(LogTopLevelWindows_EnumWindowsProc, (LPARAM)&logTopLevelWindowsState) == 0) {
		fprintf(stderr, "EnumWindows() failed [0x%x]\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	RemoveUnseenWindows(&state->foregroundWindow);
}

static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(traceloggingProvider, "ReceivedMessage", TraceLoggingHexUInt32(uMsg, "uMsg"), TraceLoggingHexUInt64(wParam, "wParam"), TraceLoggingHexUInt64(lParam, "lParam"));

	if (uMsg == WM_CREATE) OnWindowCreate(hWnd, lParam);

	State* const state = GetWindowState(hWnd);
	if (state != NULL) LogTopLevelWindows(state);

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main(int argc, char** argv) {
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = WindowProcedure;
	windowClass.lpszClassName = L"AppbarMon";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	State state;
	state.foregroundWindow = NULL;
	const HWND window = CreateWindowW(
		L"AppbarMon",
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
