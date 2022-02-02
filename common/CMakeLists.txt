add_library(WindowInvestigator_tracing STATIC EXCLUDE_FROM_ALL "tracing.c")

add_library(WindowInvestigator_window_util STATIC EXCLUDE_FROM_ALL "window_util.c")

add_library(WindowInvestigator_user32_private SHARED EXCLUDE_FROM_ALL "user32_private.c" "user32_private.def")
