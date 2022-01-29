add_executable(WindowInvestigator_WindowMonitor "WindowMonitor.c")
target_link_libraries(WindowInvestigator_WindowMonitor
	PRIVATE WindowInvestigator_tracing
	PRIVATE WindowInvestigator_user32_private
	PRIVATE dwmapi
	PRIVATE winmm
	PRIVATE avrt
)
