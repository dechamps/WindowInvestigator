#include <Windows.h>
#include <TraceLoggingProvider.h>
#include <stdio.h>
#include <stdlib.h>

TRACELOGGING_DEFINE_PROVIDER(traceloggingProvider, "AppbarMon", (0x500d9509, 0x6850, 0x440c, 0xad, 0x11, 0x6e, 0xa6, 0x25, 0xec, 0x91, 0xbc));

static LRESULT CALLBACK windowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(traceloggingProvider, "ReceivedMessage", TraceLoggingUInt32(uMsg, "uMsg"), TraceLoggingUInt64(wParam, "wParam"), TraceLoggingUInt64(lParam, "lParam"));
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
