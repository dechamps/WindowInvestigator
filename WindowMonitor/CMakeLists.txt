add_executable(WindowInvestigator_WindowMonitor "WindowMonitor.c")
target_link_libraries(WindowInvestigator_WindowMonitor
	PRIVATE WindowInvestigator_tracing
	PRIVATE WindowInvestigator_user32_private
	PRIVATE WindowInvestigator_window_util
	PRIVATE dwmapi
	PRIVATE winmm
	PRIVATE avrt
)
set_property(TARGET WindowInvestigator_WindowMonitor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
install(TARGETS WindowInvestigator_WindowMonitor RUNTIME)
