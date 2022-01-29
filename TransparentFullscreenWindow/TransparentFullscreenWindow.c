#include "../common/tracing.h"

#include <Windows.h>
#include <stdio.h>

static LRESULT CALLBACK TransparentFullscreenWindow_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "ReceivedMessage", TraceLoggingHexUInt32(uMsg, "uMsg"), TraceLoggingHexUInt64(wParam, "wParam"), TraceLoggingHexUInt64(lParam, "lParam"));

	if (uMsg == WM_CREATE) {
		SetLastError(NO_ERROR);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
		const DWORD setWindowLongError = GetLastError();
		if (setWindowLongError != NO_ERROR) {
			fprintf(stderr, "SetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", setWindowLongError);
			exit(EXIT_FAILURE);
		}
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

int main() {
	const HRESULT registerResult = TraceLoggingRegister(WindowInvestigator_traceloggingProvider);
	if (!SUCCEEDED(registerResult)) {
		fprintf(stderr, "Unable to register tracing provider [0x%lx]\n", registerResult);
		return EXIT_FAILURE;
	}

	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	if (screenWidth == 0) {
		fprintf(stderr, "GetSystemMetrics(SM_CXFULLSCREEN) failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (screenHeight == 0) {
		fprintf(stderr, "GetSystemMetrics(SM_CYFULLSCREEN) failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = TransparentFullscreenWindow_WindowProcedure;
	windowClass.lpszClassName = L"WindowInvestigator_TransparentFullscreenWindow";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	const HWND window = CreateWindowExW(
		/*dwExtStyle=*/WS_EX_LAYERED | WS_EX_TRANSPARENT,
		/*lpClassName=*/L"WindowInvestigator_TransparentFullscreenWindow",
		/*lpWindowName=*/L"WindowInvestigator_TransparentFullscreenWindow",
		/*dwStyle=*/WS_VISIBLE,
		/*X=*/0,
		/*Y=*/0,
		/*nWidth=*/screenWidth,
		/*nHeight=*/screenHeight,
		/*hWndParent=*/NULL,
		/*hMenu=*/NULL,
		/*hInstance=*/NULL,
		/*lpParam=*/NULL
	);
	if (window == NULL) {
		fprintf(stderr, "CreateWindowW failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "TransparentFullscreenWindowCreated", TraceLoggingPointer(window, "HWND"));

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
