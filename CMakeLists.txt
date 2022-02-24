cmake_minimum_required(VERSION 3.20)
project(WindowInvestigator)

add_compile_options(/WX /W4 /permissive- /analyze)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(common)
add_subdirectory(BroadcastShellHookMessage)
add_subdirectory(DelayedPosWindow)
add_subdirectory(TransparentFullscreenWindow)
add_subdirectory(WindowMonitor)
