#include <Windows.h>
#include <stdio.h>

int main() {
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
	windowClass.lpfnWndProc = DefWindowProcW;
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
