add_executable(WindowInvestigator_DelayedPosWindow "DelayedPosWindow.c")
target_link_libraries(WindowInvestigator_DelayedPosWindow
	PRIVATE WindowInvestigator_tracing
	PRIVATE WindowInvestigator_window_util
)
set_property(TARGET WindowInvestigator_DelayedPosWindow PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
