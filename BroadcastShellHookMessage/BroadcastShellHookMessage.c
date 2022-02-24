#include <Windows.h>

#include <stdio.h>
#include <inttypes.h>

static __declspec(noreturn) void BroadcastShellHookMessage_Usage() {
	fprintf(stderr, "usage: BroadcastShellHookMessage <WPARAM> <LPARAM>\n");
	fprintf(stderr, "e.g. BroadcastShellHookMessage 0x35 0x4242\n");
	exit(EXIT_FAILURE);
}

int wmain(int argc, const wchar_t* const* const argv, const wchar_t* const* const envp) {
	UNREFERENCED_PARAMETER(envp);

	if (argc != 3) BroadcastShellHookMessage_Usage();

	WPARAM wParam;
	if (swscanf_s(argv[1], L"0x%" SCNxPTR, &wParam) != 1) BroadcastShellHookMessage_Usage();
	LPARAM lParam;
	if (swscanf_s(argv[2], L"0x%" SCNxPTR, &lParam) != 1) BroadcastShellHookMessage_Usage();

	const UINT shellhookMessage = RegisterWindowMessageW(L"SHELLHOOK");
	if (shellhookMessage == 0) {
		fprintf(stderr, "RegisterWindowMessageW(\"SHELLHOOK\") failed [0x%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	DWORD recipients = BSM_APPLICATIONS;
	if (BroadcastSystemMessage(BSF_POSTMESSAGE, &recipients, shellhookMessage, wParam, lParam) < 0) {
		fprintf(stderr, "BroadcastSystemMessage() failed [0x%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
