#include "../common/tracing.h"
#include "../common/window_util.h"

#include <Windows.h>
#include <stdio.h>

static LRESULT CALLBACK DelayedPosWindow_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "ReceivedMessage", TraceLoggingHexUInt32(uMsg, "uMsg"), TraceLoggingHexUInt64(wParam, "wParam"), TraceLoggingHexUInt64(lParam, "lParam"));

	if (uMsg == WM_CREATE) WindowInvestigator_SetWindowUserDataOnCreate(hWnd, lParam);

	if (uMsg == WM_WINDOWPOSCHANGING) {
		const WINDOWPOS* windowpos = (WINDOWPOS*)lParam;
		const DWORD delayMilliseconds = *(DWORD*)WindowInvestigator_GetWindowUserData(hWnd);
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowPosChanging",
			TraceLoggingPointer(windowpos->hwnd, "hwnd"), TraceLoggingPointer(windowpos->hwndInsertAfter, "hwndInsertAfter"),
			TraceLoggingInt32(windowpos->x, "x"), TraceLoggingInt32(windowpos->y, "y"), TraceLoggingInt32(windowpos->cx, "x"), TraceLoggingInt32(windowpos->cy, "cy"),
			TraceLoggingHexUInt32(windowpos->flags, "flags"), TraceLoggingUInt32(delayMilliseconds, "delayMilliseconds"));

		if (delayMilliseconds > 0)
			Sleep(delayMilliseconds);

		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "DelayElapsed");
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static int DelayedPosWindow(DWORD delayMilliseconds) {
	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = DelayedPosWindow_WindowProcedure;
	windowClass.lpszClassName = L"WindowInvestigator_DelayedPosWindow";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	const HWND window = CreateWindowExW(
		/*dwExtStyle=*/0,
		/*lpClassName=*/L"WindowInvestigator_DelayedPosWindow",
		/*lpWindowName=*/L"WindowInvestigator_DelayedPosWindow",
		/*dwStyle=*/WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		/*X=*/CW_USEDEFAULT,
		/*Y=*/CW_USEDEFAULT,
		/*nWidth=*/CW_USEDEFAULT,
		/*nHeight=*/CW_USEDEFAULT,
		/*hWndParent=*/NULL,
		/*hMenu=*/NULL,
		/*hInstance=*/NULL,
		/*lpParam=*/&delayMilliseconds
	);
	if (window == NULL) {
		fprintf(stderr, "CreateWindowW failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "DelayedPosWindowCreated", TraceLoggingPointer(window, "HWND"));

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

int wmain(int argc, const wchar_t* const* const argv, const wchar_t* const* const envp) {
	UNREFERENCED_PARAMETER(envp);

	const HRESULT registerResult = TraceLoggingRegister(WindowInvestigator_traceloggingProvider);
	if (!SUCCEEDED(registerResult)) {
		fprintf(stderr, "Unable to register tracing provider [0x%lx]\n", registerResult);
		return EXIT_FAILURE;
	}

	if (argc == 2) {
		DWORD delayMilliseconds;
		if (swscanf_s(argv[1], L"%lu", &delayMilliseconds) == 1)
			return DelayedPosWindow(delayMilliseconds);
	}

	fprintf(stderr, "usage: DelayedPosWindow <delay in milliseconds>\n");
}
