add_executable(WindowInvestigator_WindowMonitor "WindowMonitor.c" "WindowMonitor.manifest")
target_link_libraries(WindowInvestigator_WindowMonitor
	PRIVATE WindowInvestigator_tracing
	PRIVATE WindowInvestigator_user32_private
	PRIVATE WindowInvestigator_window_util
	PRIVATE dwmapi
	PRIVATE winmm
	PRIVATE avrt
)
install(TARGETS WindowInvestigator_WindowMonitor RUNTIME)
