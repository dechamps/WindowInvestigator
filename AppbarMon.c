#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

static LRESULT CALLBACK windowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg != WM_USER) {
		fprintf(stderr, "Got unknown message %x\n", uMsg);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	if (wParam != ABN_FULLSCREENAPP) {
		fprintf(stderr, "Got unknown notification code %llx\n", wParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	fprintf(stderr, "Got ABN_FULLSCREENAPP with param %llx\n", lParam);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main(int argc, char** argv) {
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = windowProcedure;
	windowClass.lpszClassName = L"AppbarMon";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

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
		NULL
	);
	if (window == NULL) {
		fprintf(stderr, "CreateWindowW failed [%x]\n", GetLastError());
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
