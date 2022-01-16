cmake_minimum_required(VERSION 3.20)
project(AppbarMon)

add_compile_options(/WX /W4 /permissive- /analyze)

add_executable(AppbarMon "AppbarMon.c")
