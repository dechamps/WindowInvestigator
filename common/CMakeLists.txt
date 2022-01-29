add_library(WindowInvestigator_tracing STATIC EXCLUDE_FROM_ALL "tracing.c")

add_custom_command(
	OUTPUT user32_private.lib user32_private.exp
	COMMAND "${CMAKE_AR}" ${CMAKE_SHARED_LINKER_FLAGS} "/DEF:${CMAKE_CURRENT_LIST_DIR}/user32_private.def" /OUT:user32_private.lib
	DEPENDS "${CMAKE_CURRENT_LIST_DIR}/user32_private.def"
	VERBATIM
)
add_custom_target(WindowInvestigator_user32_private_target DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/user32_private.lib")

add_library(WindowInvestigator_user32_private SHARED IMPORTED GLOBAL)
add_dependencies(WindowInvestigator_user32_private WindowInvestigator_user32_private_target)
set_target_properties(WindowInvestigator_user32_private PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/user32_private.lib")
