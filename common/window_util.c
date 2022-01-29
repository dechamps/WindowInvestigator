#include "window_util.h"

#include <stdio.h>

void WindowInvestigator_SetWindowUserDataOnCreate(HWND window, LPARAM lParam) {
	SetLastError(NO_ERROR);
	SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
	const DWORD setWindowLongError = GetLastError();
	if (setWindowLongError != NO_ERROR) {
		fprintf(stderr, "SetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", setWindowLongError);
		exit(EXIT_FAILURE);
	}
}

LONG_PTR WindowInvestigator_GetWindowUserData(HWND window) {
	SetLastError(NO_ERROR);
	const LONG_PTR windowUserData = GetWindowLongPtrW(window, GWLP_USERDATA);
	const DWORD getWindowLongError = GetLastError();
	if (getWindowLongError != NO_ERROR) {
		fprintf(stderr, "GetWindowLongPtrW(GWLP_USERDATA) failed [0x%x]\n", getWindowLongError);
		exit(EXIT_FAILURE);
	}
	return windowUserData;
}
