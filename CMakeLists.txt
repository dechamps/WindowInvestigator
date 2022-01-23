cmake_minimum_required(VERSION 3.20)
project(AppbarMon)

add_compile_options(/WX /W4 /permissive- /analyze)

add_custom_command(
	OUTPUT user32_private.lib user32_private.exp
	COMMAND "${CMAKE_AR}" ${CMAKE_SHARED_LINKER_FLAGS} "/DEF:${CMAKE_CURRENT_LIST_DIR}/user32_private.def" /OUT:user32_private.lib
	DEPENDS "${CMAKE_CURRENT_LIST_DIR}/user32_private.def"
	VERBATIM
)
add_custom_target(user32_private_target DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/user32_private.lib")

add_library(user32_private SHARED IMPORTED)
add_dependencies(user32_private user32_private_target)
set_target_properties(user32_private PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/user32_private.lib")

add_executable(AppbarMon "AppbarMon.c")
target_link_libraries(AppbarMon
	PRIVATE dwmapi
	PRIVATE user32_private
)
