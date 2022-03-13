#include "../common/tracing.h"
#include "../common/user32_private.h"
#include "../common/window_util.h"

#include <Windows.h>
#include <TraceLoggingProvider.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <dwmapi.h>
#include <avrt.h>

typedef struct {
	DWORD processId;
	DWORD threadId;

	// RudeWindowWin32Functions::GetClassNameW()
	wchar_t className[1024];

	// RudeWindowWin32Functions::GetExStyleFromWindow()
	DWORD extendedStyles;

	// RudeWindowWin32Functions::GetStyleFromWindow()
	DWORD styles;

	// RudeWindowWin32Functions::GetWindowRectForFullscreenCheck()
	RECT windowRect;
	RECT clientRect;
	RECT clientRectInScreenCoordinates;
	// This one is not part of RudeWindowWin32Functions, but included nonetheless because its output might be interesting.
	WINDOWPLACEMENT placement;

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
	BOOL hasNonRudeAddedByRudeWindowFixerProperty;
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

static void WindowMonitor_DumpWindowInfo(const WindowMonitor_WindowInfo* windowInfo) {
	printf("PID: %lu TID: %lu\n", windowInfo->processId, windowInfo->threadId);
	printf("Class name: \"%S\"\n", windowInfo->className);
	printf("Extended styles: 0x%08X\n", windowInfo->extendedStyles);
	printf("Styles: 0x%08lX\n", windowInfo->styles);
	printf("Window rect: (%ld, %ld, %ld, %ld)\n", windowInfo->windowRect.left, windowInfo->windowRect.top, windowInfo->windowRect.right, windowInfo->windowRect.bottom);
	printf("Client rect: (%ld, %ld, %ld, %ld)\n", windowInfo->clientRect.left, windowInfo->clientRect.top, windowInfo->clientRect.right, windowInfo->clientRect.bottom);
	printf("Client rect in screen coordinates: (%ld, %ld, %ld, %ld)\n", windowInfo->clientRectInScreenCoordinates.left, windowInfo->clientRectInScreenCoordinates.top, windowInfo->clientRectInScreenCoordinates.right, windowInfo->clientRectInScreenCoordinates.bottom);
	printf("Placement: showCmd %u minPosition (%ld, %ld) maxPosition (%ld, %ld), normalPosition (%ld, %ld, %ld, %ld)\n",
		windowInfo->placement.showCmd,
		windowInfo->placement.ptMinPosition.x, windowInfo->placement.ptMinPosition.y,
		windowInfo->placement.ptMaxPosition.x, windowInfo->placement.ptMaxPosition.y,
		windowInfo->placement.rcNormalPosition.left, windowInfo->placement.rcNormalPosition.top, windowInfo->placement.rcNormalPosition.right, windowInfo->placement.rcNormalPosition.bottom);
	printf("Text: \"%S\"\n", windowInfo->text);
	printf("Shell managed: %s\n", windowInfo->isShellManagedWindow ? "TRUE" : "FALSE");
	printf("Shell frame: %s\n", windowInfo->isShellFrameWindow ? "TRUE" : "FALSE");
	printf("Overpanning: %s\n", windowInfo->overpanning ? "TRUE" : "FALSE");
	printf("Band: %lu\n", windowInfo->band);
	printf("Has \"NonRudeHWND\" property: %s\n", windowInfo->hasNonRudeHWNDProperty ? "TRUE" : "FALSE");
	printf("Has non-rude added by RudeWindowFixer property: %s\n", windowInfo->hasNonRudeAddedByRudeWindowFixerProperty ? "TRUE" : "FALSE");
	printf("Has \"LivePreviewWindow\" property: %s\n", windowInfo->hasLivePreviewWindowProperty ? "TRUE" : "FALSE");
	printf("Has \"TreatAsDesktopFullscreen\" property: %s\n", windowInfo->hasTreatAsDesktopFullscreenProperty ? "TRUE" : "FALSE");
	printf("Is window: %s\n", windowInfo->isWindow ? "TRUE" : "FALSE");
	printf("DWM is cloaked: 0x%08lX\n", windowInfo->dwmIsCloaked);
	printf("Is iconic: %s\n", windowInfo->isIconic ? "TRUE" : "FALSE");
	printf("Is visible: %s\n", windowInfo->isVisible ? "TRUE" : "FALSE");
}

static WindowMonitor_WindowInfo WindowMonitor_GetWindowInfo(HWND window) {
	WindowMonitor_WindowInfo windowInfo;

	windowInfo.threadId = GetWindowThreadProcessId(window, &windowInfo.processId);

	SetLastError(NO_ERROR);
	GetClassNameW(window, windowInfo.className, sizeof(windowInfo.className) / sizeof(*windowInfo.className));
	const DWORD classNameError = GetLastError();
	if (classNameError != NO_ERROR)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "classNameError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(classNameError, "ErrorCode"));

	SetLastError(NO_ERROR);
	windowInfo.extendedStyles = (DWORD)GetWindowLongPtrW(window, GWL_EXSTYLE);
	const DWORD extendedStylesError = GetLastError();
	if (extendedStylesError != NO_ERROR)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "extendedStylesError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(extendedStylesError, "ErrorCode"));

	SetLastError(NO_ERROR);
	windowInfo.styles = (DWORD)GetWindowLongPtrW(window, GWL_STYLE);
	const DWORD stylesError = GetLastError();
	if (stylesError != NO_ERROR)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "stylesError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(stylesError, "ErrorCode"));

	if (!GetWindowRect(window, &windowInfo.windowRect))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "windowRectError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	if (!GetClientRect(window, &windowInfo.clientRect))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "clientRectError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));
	else {
		SetLastError(NO_ERROR);
		RECT rect = windowInfo.clientRect;
		MapWindowPoints(window, NULL, (LPPOINT)&rect, 2);
		const DWORD clientRectMapWindowPointsError = GetLastError();
		if (clientRectMapWindowPointsError != NO_ERROR)
			TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "clientRectMapWindowPointsError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(clientRectMapWindowPointsError, "ErrorCode"));
		else
			windowInfo.clientRectInScreenCoordinates = rect;
	}

	windowInfo.placement.length = sizeof(windowInfo.placement);
	if (!GetWindowPlacement(window, &windowInfo.placement))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "placementError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	SetLastError(NO_ERROR);
	InternalGetWindowText(window, windowInfo.text, sizeof(windowInfo.text) / sizeof(*windowInfo.text));
	const DWORD textError = GetLastError();
	if (textError != NO_ERROR)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "textError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(textError, "ErrorCode"));

	windowInfo.isShellManagedWindow = IsShellManagedWindow(window);

	windowInfo.isShellFrameWindow = IsShellFrameWindow(window);

	windowInfo.overpanning = GetPropW(window, (LPCWSTR) (intptr_t) ATOM_OVERPANNING) != NULL;

	if (!GetWindowBand(window, &windowInfo.band))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "windowBandError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexUInt32(GetLastError(), "ErrorCode"));

	windowInfo.hasNonRudeHWNDProperty = GetPropW(window, L"NonRudeHWND") != NULL;

	windowInfo.hasNonRudeAddedByRudeWindowFixerProperty = GetPropW(window, L"NonRudeHWND was set by https://github.com/dechamps/RudeWindowFixer") != NULL;
	
	windowInfo.hasLivePreviewWindowProperty = GetPropW(window, L"LivePreviewWindow") != NULL;

	windowInfo.hasTreatAsDesktopFullscreenProperty = GetPropW(window, L"TreatAsDesktopFullscreen") != NULL;

	windowInfo.isWindow = IsWindow(window);

	const HRESULT dwmIsCloakedResult = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &windowInfo.dwmIsCloaked, sizeof(windowInfo.dwmIsCloaked));
	if (!SUCCEEDED(dwmIsCloakedResult))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "dwmIsCloakedError", TraceLoggingPointer(window, "HWND"), TraceLoggingHexLong(dwmIsCloakedResult, "HRESULT"));

	windowInfo.isIconic = IsIconic(window);

	windowInfo.isVisible = IsWindowVisible(window);

	return windowInfo;
}

static void WindowMonitor_LogWindowInfo(HWND window, const WindowMonitor_WindowInfo* windowInfo) {
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowLogStart", TraceLoggingPointer(window, "HWND"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowProcessId", TraceLoggingPointer(window, "HWND"),
		TraceLoggingHexUInt32(windowInfo->processId, "ProcessId"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowThreadId", TraceLoggingPointer(window, "HWND"),
		TraceLoggingHexUInt32(windowInfo->threadId, "ThreadId"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClassName", TraceLoggingPointer(window, "HWND"),
		TraceLoggingWideString(windowInfo->className, "ClassName"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowExtendedStyles", TraceLoggingPointer(window, "HWND"),
		 TraceLoggingHexUInt32(windowInfo->extendedStyles, "ExtendedStyles"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowStyles", TraceLoggingPointer(window, "HWND"),
		TraceLoggingHexUInt32(windowInfo->styles, "Styles"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowRect", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->windowRect.left, "WindowRectLeft"), TraceLoggingLong(windowInfo->windowRect.top, "WindowRectTop"),
		TraceLoggingLong(windowInfo->windowRect.right, "WindowRectRight"), TraceLoggingLong(windowInfo->windowRect.bottom, "WindowRectBottom"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClientRect", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->clientRect.left, "ClientRectLeft"), TraceLoggingLong(windowInfo->clientRect.top, "ClientRectTop"),
		TraceLoggingLong(windowInfo->clientRect.right, "ClientRectRight"), TraceLoggingLong(windowInfo->clientRect.bottom, "ClientRectBottom"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClientRectInScreenCoordinates", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->clientRectInScreenCoordinates.left, "ClientRectInScreenCoordinatesLeft"), TraceLoggingLong(windowInfo->clientRectInScreenCoordinates.top, "ClientRectInScreenCoordinatesTop"),
		TraceLoggingLong(windowInfo->clientRectInScreenCoordinates.right, "ClientRectInScreenCoordinatesRight"), TraceLoggingLong(windowInfo->clientRectInScreenCoordinates.bottom, "ClientRectInScreenCoordinatesBottom"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementShowCmd", TraceLoggingPointer(window, "HWND"),
		TraceLoggingUInt32(windowInfo->placement.showCmd, "ShowCmd"));
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementMinPosition", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->placement.ptMinPosition.x, "MinPositionX"), TraceLoggingLong(windowInfo->placement.ptMinPosition.y, "MinPositionY"),
		TraceLoggingLong(windowInfo->placement.ptMinPosition.x, "MinPositionX"), TraceLoggingLong(windowInfo->placement.ptMinPosition.y, "MinPositionY"));
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementMaxPosition", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->placement.ptMaxPosition.x, "MaxPositionX"), TraceLoggingLong(windowInfo->placement.ptMaxPosition.y, "MaxPositionY"));
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementClientRect", TraceLoggingPointer(window, "HWND"),
		TraceLoggingLong(windowInfo->placement.rcNormalPosition.left, "ClientRectLeft"), TraceLoggingLong(windowInfo->placement.rcNormalPosition.top, "ClientRectTop"),
		TraceLoggingLong(windowInfo->placement.rcNormalPosition.right, "ClientRectRight"), TraceLoggingLong(windowInfo->placement.rcNormalPosition.bottom, "ClientRectBottom"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowText", TraceLoggingPointer(window, "HWND"),
		TraceLoggingWideString(windowInfo->text, "WindowText"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsShellManagedWindow", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->isShellManagedWindow, "IsShellManagedWindow"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsShellFrameWindow", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->isShellFrameWindow, "IsShellFrameWindow"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowOverpanning", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->overpanning, "Overpanning"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowBand", TraceLoggingPointer(window, "HWND"),
		TraceLoggingUInt32(windowInfo->band, "Band"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasNonRudeHWNDProperty", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->hasNonRudeHWNDProperty, "HasNonRudeHWNDProperty"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasNonRudeAddedByRudeWindowFixerProperty", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->hasNonRudeAddedByRudeWindowFixerProperty, "HasNonRudeAddedByRudeWindowFixerProperty"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasLivePreviewWindowProperty", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->hasLivePreviewWindowProperty, "HasLivePreviewWindowProperty"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasTreatAsDesktopFullscreenProperty", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->hasTreatAsDesktopFullscreenProperty, "HasTreatAsDesktopFullscreenProperty"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsWindow", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->isWindow, "IsWindow"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowDwmIsCloaked", TraceLoggingPointer(window, "HWND"),
		TraceLoggingUInt32(windowInfo->dwmIsCloaked, "DwmIsCloaked"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsIconic", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->isIconic, "IsIconic"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsVisible", TraceLoggingPointer(window, "HWND"),
		TraceLoggingBool(windowInfo->isVisible, "IsVisible"));

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowLogEnd", TraceLoggingPointer(window, "HWND"));
}

static void WindowMonitor_DiffWindowInfo(HWND window, const WindowMonitor_WindowInfo* oldWindowInfo, const WindowMonitor_WindowInfo* newWindowInfo) {
	if (oldWindowInfo->processId != newWindowInfo->processId)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowProcessIdChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->processId, "OldProcessId"), TraceLoggingHexUInt32(newWindowInfo->processId, "NewProcessId"));

	if (oldWindowInfo->threadId != newWindowInfo->threadId)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowThreadIdChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->threadId, "OldThreadId"), TraceLoggingHexUInt32(newWindowInfo->threadId, "NewThreadId"));
	
	if (wcscmp(oldWindowInfo->className, newWindowInfo->className) != 0)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClassNameChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingWideString(oldWindowInfo->className, "OldClassName"), TraceLoggingWideString(newWindowInfo->className, "NewClassName"));

	if (oldWindowInfo->extendedStyles != newWindowInfo->extendedStyles)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowExtendedStylesChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->extendedStyles, "OldExtendedStyles"), TraceLoggingHexUInt32(newWindowInfo->extendedStyles, "NewExtendedStyles"));

	if (oldWindowInfo->styles != newWindowInfo->styles)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowStylesChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingHexUInt32(oldWindowInfo->styles, "OldStyles"), TraceLoggingHexUInt32(newWindowInfo->styles, "NewStyles"));

	if (!EqualRect(&oldWindowInfo->windowRect, &newWindowInfo->windowRect))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowRectChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->windowRect.left, "OldWindowRectLeft"), TraceLoggingLong(oldWindowInfo->windowRect.top, "OldWindowRectTop"),
			TraceLoggingLong(oldWindowInfo->windowRect.right, "OldWindowRectRight"), TraceLoggingLong(oldWindowInfo->windowRect.bottom, "OldWindowRectBottom"),
			TraceLoggingLong(newWindowInfo->windowRect.left, "NewWindowRectLeft"), TraceLoggingLong(newWindowInfo->windowRect.top, "NewWindowRectTop"),
			TraceLoggingLong(newWindowInfo->windowRect.right, "NewWindowRectRight"), TraceLoggingLong(newWindowInfo->windowRect.bottom, "NewWindowRectBottom"));

	if (!EqualRect(&oldWindowInfo->clientRect, &newWindowInfo->clientRect))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClientRectChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->clientRect.left, "OldClientRectLeft"), TraceLoggingLong(oldWindowInfo->clientRect.top, "OldClientRectTop"),
			TraceLoggingLong(oldWindowInfo->clientRect.right, "OldClientRectRight"), TraceLoggingLong(oldWindowInfo->clientRect.bottom, "OldClientRectBottom"),
			TraceLoggingLong(newWindowInfo->clientRect.left, "NewClientRectLeft"), TraceLoggingLong(newWindowInfo->clientRect.top, "NewClientRectTop"),
			TraceLoggingLong(newWindowInfo->clientRect.right, "NewClientRectRight"), TraceLoggingLong(newWindowInfo->clientRect.bottom, "NewClientRectBottom"));

	if (!EqualRect(&oldWindowInfo->clientRectInScreenCoordinates, &newWindowInfo->clientRectInScreenCoordinates))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowClientRectInScreenCoordinatesChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->clientRectInScreenCoordinates.left, "OldClientRectInScreenCoordinatesLeft"), TraceLoggingLong(oldWindowInfo->clientRectInScreenCoordinates.top, "OldClientRectInScreenCoordinatesTop"),
			TraceLoggingLong(oldWindowInfo->clientRectInScreenCoordinates.right, "OldClientRectInScreenCoordinatesRight"), TraceLoggingLong(oldWindowInfo->clientRectInScreenCoordinates.bottom, "OldClientRectInScreenCoordinatesBottom"),
			TraceLoggingLong(newWindowInfo->clientRectInScreenCoordinates.left, "NewClientRectInScreenCoordinatesLeft"), TraceLoggingLong(newWindowInfo->clientRectInScreenCoordinates.top, "NewClientRectInScreenCoordinatesTop"),
			TraceLoggingLong(newWindowInfo->clientRectInScreenCoordinates.right, "NewClientRectInScreenCoordinatesRight"), TraceLoggingLong(newWindowInfo->clientRectInScreenCoordinates.bottom, "NewClientRectInScreenCoordinatesBottom"));

	if (oldWindowInfo->placement.showCmd != newWindowInfo->placement.showCmd)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementShowCmdChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingUInt32(oldWindowInfo->placement.showCmd, "OldShowCmd"), TraceLoggingUInt32(newWindowInfo->placement.showCmd, "NewShowCmd"));
	if (oldWindowInfo->placement.ptMinPosition.x != newWindowInfo->placement.ptMinPosition.x ||
		oldWindowInfo->placement.ptMinPosition.y != newWindowInfo->placement.ptMinPosition.y)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementMinPositionChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->placement.ptMinPosition.x, "OldMinPositionX"), TraceLoggingLong(oldWindowInfo->placement.ptMinPosition.y, "OldMinPositionY"),
			TraceLoggingLong(newWindowInfo->placement.ptMinPosition.x, "NewMinPositionX"), TraceLoggingLong(newWindowInfo->placement.ptMinPosition.y, "NewMinPositionY"));
	if (oldWindowInfo->placement.ptMaxPosition.x != newWindowInfo->placement.ptMaxPosition.x ||
		oldWindowInfo->placement.ptMaxPosition.y != newWindowInfo->placement.ptMaxPosition.y)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementMaxPositionChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->placement.ptMaxPosition.x, "OldMaxPositionX"), TraceLoggingLong(oldWindowInfo->placement.ptMaxPosition.y, "OldMaxPositionY"),
			TraceLoggingLong(newWindowInfo->placement.ptMaxPosition.x, "NewMaxPositionX"), TraceLoggingLong(newWindowInfo->placement.ptMaxPosition.y, "NewMaxPositionY"));
	if (!EqualRect(&oldWindowInfo->placement.rcNormalPosition, &newWindowInfo->placement.rcNormalPosition))
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "PlacementClientRectChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingLong(oldWindowInfo->placement.rcNormalPosition.left, "OldClientRectLeft"), TraceLoggingLong(oldWindowInfo->placement.rcNormalPosition.top, "OldClientRectTop"),
			TraceLoggingLong(oldWindowInfo->placement.rcNormalPosition.right, "OldClientRectRight"), TraceLoggingLong(oldWindowInfo->placement.rcNormalPosition.bottom, "OldClientRectBottom"),
			TraceLoggingLong(newWindowInfo->placement.rcNormalPosition.left, "NewClientRectLeft"), TraceLoggingLong(newWindowInfo->placement.rcNormalPosition.top, "NewClientRectTop"),
			TraceLoggingLong(newWindowInfo->placement.rcNormalPosition.right, "NewClientRectRight"), TraceLoggingLong(newWindowInfo->placement.rcNormalPosition.bottom, "NewClientRectBottom"));

	if (wcscmp(oldWindowInfo->text, newWindowInfo->text) != 0)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowTextChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingWideString(oldWindowInfo->text, "OldWindowText"), TraceLoggingWideString(newWindowInfo->text, "NewWindowText"));

	if (oldWindowInfo->isShellManagedWindow != newWindowInfo->isShellManagedWindow)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsShellManagedWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isShellManagedWindow, "OldIsShellManagedWindow"), TraceLoggingBool(newWindowInfo->isShellManagedWindow, "NewIsShellManagedWindow"));

	if (oldWindowInfo->isShellFrameWindow != newWindowInfo->isShellFrameWindow)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsShellFrameWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isShellFrameWindow, "OldIsShellFrameWindow"), TraceLoggingBool(newWindowInfo->isShellFrameWindow, "NewIsShellFrameWindow"));

	if (oldWindowInfo->overpanning != newWindowInfo->overpanning)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowOverpanningChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->overpanning, "OldOverpanning"), TraceLoggingBool(newWindowInfo->overpanning, "NewOverpanning"));

	if (oldWindowInfo->band != newWindowInfo->band)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowBandChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingUInt32(oldWindowInfo->band, "OldBand"), TraceLoggingUInt32(newWindowInfo->band, "NewBand"));

	if (oldWindowInfo->hasNonRudeHWNDProperty != newWindowInfo->hasNonRudeHWNDProperty)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasNonRudeHWNDPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasNonRudeHWNDProperty, "OldHasNonRudeHWNDProperty"), TraceLoggingBool(newWindowInfo->hasNonRudeHWNDProperty, "NewHasNonRudeHWNDProperty"));

	if (oldWindowInfo->hasNonRudeAddedByRudeWindowFixerProperty != newWindowInfo->hasNonRudeAddedByRudeWindowFixerProperty)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasNonRudeAddedByRudeWindowFixerPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasNonRudeAddedByRudeWindowFixerProperty, "OldHasNonRudeAddedByRudeWindowFixerProperty"), TraceLoggingBool(newWindowInfo->hasNonRudeAddedByRudeWindowFixerProperty, "NewHasNonRudeAddedByRudeWindowFixerProperty"));

	if (oldWindowInfo->hasLivePreviewWindowProperty != newWindowInfo->hasLivePreviewWindowProperty)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasLivePreviewWindowPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasLivePreviewWindowProperty, "OldHasLivePreviewWindowProperty"), TraceLoggingBool(newWindowInfo->hasLivePreviewWindowProperty, "NewHasLivePreviewWindowProperty"));

	if (oldWindowInfo->hasTreatAsDesktopFullscreenProperty != newWindowInfo->hasTreatAsDesktopFullscreenProperty)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowHasTreatAsDesktopFullscreenPropertyChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->hasTreatAsDesktopFullscreenProperty, "OldHasTreatAsDesktopFullscreenProperty"), TraceLoggingBool(newWindowInfo->hasTreatAsDesktopFullscreenProperty, "NewHasTreatAsDesktopFullscreenProperty"));

	if (oldWindowInfo->isWindow != newWindowInfo->isWindow)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsWindowChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isWindow, "OldIsWindow"), TraceLoggingBool(newWindowInfo->isWindow, "NewIsWindow"));

	if (oldWindowInfo->dwmIsCloaked != newWindowInfo->dwmIsCloaked)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowDwmIsCloakedChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingUInt32(oldWindowInfo->dwmIsCloaked, "OldDwmIsCloaked"), TraceLoggingUInt32(newWindowInfo->dwmIsCloaked, "NewDwmIsCloaked"));

	if (oldWindowInfo->isIconic != newWindowInfo->isIconic)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsIconicChanged", TraceLoggingPointer(window, "HWND"),
			TraceLoggingBool(oldWindowInfo->isIconic, "OldIsIconic"), TraceLoggingBool(newWindowInfo->isIconic, "NewIsIconic"));

	if (oldWindowInfo->isVisible != newWindowInfo->isVisible)
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowIsVisibleChanged", TraceLoggingPointer(window, "HWND"),
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
	time_t lastLog;
} State;


static void WindowMonitor_LogTopLevelWindows(const WindowMonitor_Window* window) {
	while (window != NULL) {
		WindowMonitor_LogWindowInfo(window->window, &window->info);
		window = window->next;
	}
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
		
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowGone", TraceLoggingPointer(window->window, "HWND"));
		*nextWindow = window->next;
		free(window);
	}
}

typedef struct {
	WindowMonitor_Window** window;
	UINT32 zOrder;
} WindowMonitor_DiffTopLevelWindows_State;

static BOOL CALLBACK WindowMonitor_DiffTopLevelWindows_EnumWindowsProc(HWND window, LPARAM lParam) {
	if (!IsWindowVisible(window)) return TRUE;

	WindowMonitor_DiffTopLevelWindows_State* const state = (WindowMonitor_DiffTopLevelWindows_State*)lParam;

	BOOL isNewWindow = FALSE;
	if (*state->window == NULL || (*state->window)->window != window) {
		WindowMonitor_Window** const existingWindow = WindowMonitor_SearchWindow(state->window, window);
		WindowMonitor_Window* const previousWindow = *state->window;
		if (*existingWindow == NULL) {
			TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "NewWindow", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			WindowMonitor_Window* const newWindow = *state->window = malloc(sizeof(**state->window));
			if (newWindow == NULL) abort();
			newWindow->window = window;
			newWindow->seen = FALSE;
			isNewWindow = TRUE;
		}
		else {
			TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "WindowZOrderChanged", TraceLoggingPointer(window, "HWND"), TraceLoggingUInt32(state->zOrder, "newZOrder"));
			*state->window = *existingWindow;
			*existingWindow = (*existingWindow)->next;
		}
		(*state->window)->next = previousWindow;
	}

	WindowMonitor_Window* currentWindow = *state->window;

	const WindowMonitor_WindowInfo windowInfo = WindowMonitor_GetWindowInfo(window);
	if (isNewWindow) WindowMonitor_LogWindowInfo(window, &windowInfo);
	else WindowMonitor_DiffWindowInfo(window, &currentWindow->info, &windowInfo);
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

static void WindowMonitor_DiffTopLevelWindows(State* const state) {
	WindowMonitor_MarkAllWindowsUnseen(state->foregroundWindow);

	WindowMonitor_DiffTopLevelWindows_State diffTopLevelWindowsState;
	diffTopLevelWindowsState.window = &state->foregroundWindow;
	diffTopLevelWindowsState.zOrder = 0;
	if (EnumWindows(WindowMonitor_DiffTopLevelWindows_EnumWindowsProc, (LPARAM)&diffTopLevelWindowsState) == 0) {
		fprintf(stderr, "EnumWindows() failed [0x%x]\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	WindowMonitor_RemoveUnseenWindows(&state->foregroundWindow);
}

static LRESULT CALLBACK WindowMonitor_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "ReceivedMessage", TraceLoggingHexUInt32(uMsg, "uMsg"), TraceLoggingHexUInt64(wParam, "wParam"), TraceLoggingHexUInt64(lParam, "lParam"));

	if (uMsg == WM_CREATE) {
		WindowInvestigator_SetWindowUserDataOnCreate(hWnd, lParam);

		if (SetTimer(hWnd, 1, USER_TIMER_MINIMUM, NULL) == 0) {
			fprintf(stderr, "SetTimer failed() [0x%x]\n", GetLastError());
			exit(EXIT_FAILURE);
		}
	}

	State* const state = (State*)WindowInvestigator_GetWindowUserData(hWnd);
	if (state != NULL) {
		WindowMonitor_DiffTopLevelWindows(state);

		const time_t now = time(NULL);
		if (now > state->lastLog + 5) {
			WindowMonitor_LogTopLevelWindows(state->foregroundWindow);
			state->lastLog = now;
		}
	}
	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "Done");

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static void WindowMonitor_DumpWindow(HWND window, const WindowMonitor_WindowInfo* windowInfo) {
	printf("HWND: 0x%p\n", window);

	const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, windowInfo->processId);
	if (process == NULL)
		printf("(could not open process [0x%x])", GetLastError());
	else {
		wchar_t imageName[4096];
		DWORD imageNameSize = sizeof(imageName) / sizeof(*imageName);
		if (!QueryFullProcessImageNameW(process, /*dwFlags=*/0, imageName, &imageNameSize))
			printf("(could not get process image name[0x % x])", GetLastError());
		else
			printf("Process name: \"%S\"", imageName);
	}
	printf("\n");

	WindowMonitor_DumpWindowInfo(windowInfo);

	printf("\n");
}

static BOOL CALLBACK WindowMonitor_DumpTopLevelWindows_EnumWindowsProc(HWND window, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	if (!IsWindowVisible(window)) return TRUE;

	const WindowMonitor_WindowInfo windowInfo = WindowMonitor_GetWindowInfo(window);
	WindowMonitor_DumpWindow(window, &windowInfo);

	return TRUE;
}

static void WindowMonitor_DumpTopLevelWindows(void) {
	if (EnumWindows(WindowMonitor_DumpTopLevelWindows_EnumWindowsProc, 0) == 0) {
		fprintf(stderr, "EnumWindows() failed [0x%x]\n", GetLastError());
		exit(EXIT_FAILURE);
	}
}

static void WindowMonitor_SetProcessPriority(void) {
	DWORD taskIndex = 0;
	if (AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex) == 0)
		fprintf(stderr, "AvSetMmThreadCharacteristicsW() failed [0x%lx]\n", GetLastError());

	// Note: in practice TaskbarMon needs to run as administrator to get Realtime priority. Otherwise it gets silently demoted to High.
	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
		fprintf(stderr, "Unable to set realtime process priority [0x%lx]\n", GetLastError());

	const MMRESULT timeBeginPeriodResult = timeBeginPeriod(1);
	if (timeBeginPeriodResult != TIMERR_NOERROR)
		fprintf(stderr, "timeBeginPeriod() returned error %u\n", timeBeginPeriodResult);
}

static int WindowMonitor_MonitorAllWindows(void) {
	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = WindowMonitor_WindowProcedure;
	windowClass.lpszClassName = L"WindowInvestigator_WindowMonitor";

	if (RegisterClassExW(&windowClass) == 0) {
		fprintf(stderr, "RegisterClassEx failed [%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	State state;
	state.foregroundWindow = NULL;
	state.lastLog = time(NULL);
	const HWND window = CreateWindowW(
		/*lpClassName=*/L"WindowInvestigator_WindowMonitor",
		/*lpWindowName=*/L"WindowInvestigator_WindowMonitor",
		/*dwStyle=*/0,
		/*X=*/CW_USEDEFAULT,
		/*Y=*/CW_USEDEFAULT,
		/*nWidth=*/CW_USEDEFAULT,
		/*nHeight=*/CW_USEDEFAULT,
		/*hWndParent=*/HWND_MESSAGE,
		/*hMenu=*/NULL,
		/*hInstance=*/NULL,
		/*lpParam=*/&state
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

	WindowMonitor_SetProcessPriority();

	const UINT shellhookMessage = RegisterWindowMessageW(L"SHELLHOOK");
	if (shellhookMessage == 0) {
		fprintf(stderr, "RegisterWindowMessageW(\"SHELLHOOK\") failed [0x%x]\n", GetLastError());
		return EXIT_FAILURE;
	}

	WindowMonitor_DumpTopLevelWindows();

	TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "Started", TraceLoggingHexUInt32(shellhookMessage));

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

static int WindowMonitor_MonitorSingleWindow(HWND window) {
	if (!IsWindow(window)) {
		fprintf(stderr, "ERROR: handle 0x%p does not refer to a window", window);
		return EXIT_FAILURE;
	}

	WindowMonitor_SetProcessPriority();

	WindowMonitor_WindowInfo windowInfo = WindowMonitor_GetWindowInfo(window);
	WindowMonitor_DumpWindow(window, &windowInfo);

	for (;;) {
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "Start");
		const WindowMonitor_WindowInfo newWindowInfo = WindowMonitor_GetWindowInfo(window);
		TraceLoggingWrite(WindowInvestigator_traceloggingProvider, "Done");
		WindowMonitor_DiffWindowInfo(window, &windowInfo, &newWindowInfo);

		windowInfo = newWindowInfo;
		Sleep(2);
	}
}

int wmain(int argc, const wchar_t* const* const argv, const wchar_t* const* const envp) {
	UNREFERENCED_PARAMETER(envp);

	const HRESULT registerResult = TraceLoggingRegister(WindowInvestigator_traceloggingProvider);
	if (!SUCCEEDED(registerResult)) {
		fprintf(stderr, "Unable to register tracing provider [0x%lx]\n", registerResult);
		return EXIT_FAILURE;
	}

	if (argc < 2)
		return WindowMonitor_MonitorAllWindows();
	else if (argc == 2) {
		HWND window;
		if (swscanf_s(argv[1], L"0x%p", &window) == 1)
			return WindowMonitor_MonitorSingleWindow(window);
	}
	
	fprintf(stderr, "usage: WindowMonitor [<HWND, e.g. 0x0123ABCD>]\n");
	fprintf(stderr, "If an HWND is specified, monitors that specific window; otherwise, monitors all visible top-level windows.\n");
}
