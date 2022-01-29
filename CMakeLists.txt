cmake_minimum_required(VERSION 3.20)
project(WindowInvestigator)

add_compile_options(/WX /W4 /permissive- /analyze)

add_subdirectory(TransparentFullscreenWindow)
add_subdirectory(WindowMonitor)
