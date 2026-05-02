cmake_minimum_required(VERSION 3.16)
project(esp32demo_host_tests C)

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -O0 -g")

include_directories(
    unity
    host_shims
    ../components/fb/include
    ../components/gfx/include
    ../components/gfx3d/include
    ../components/director/include
    ../components/prefetch/include
    ../components/scenes/include
)

add_library(unity STATIC unity/unity.c)

# Helper for snapshot tests
add_library(ppm STATIC ppm.c)

# Shared host-build static libs for components — tests link against these
# instead of re-listing component sources in each add_executable.
add_library(gfx_host STATIC
    ../components/gfx/gfx.c
    ../components/gfx/font_5x7.c)
target_include_directories(gfx_host PUBLIC
    ../components/gfx/include
    ../components/fb/include
    host_shims)

add_library(gfx3d_host STATIC
    ../components/gfx3d/gfx3d.c)
target_include_directories(gfx3d_host PUBLIC
    ../components/gfx3d/include
    ../components/gfx/include
    ../components/fb/include
    host_shims)

# (Future tasks: similar director_host, prefetch_host, scenes_host libs.)

# Each test binary is added by later tasks. The smoke test exists from day one.
add_executable(test_smoke test_smoke.c)
target_link_libraries(test_smoke unity)

add_executable(test_fb test_fb.c)
target_link_libraries(test_fb unity)

add_executable(test_gfx test_gfx.c)
target_link_libraries(test_gfx unity ppm gfx_host)

add_executable(test_gfx3d test_gfx3d.c)
target_link_libraries(test_gfx3d unity gfx3d_host gfx_host m)

add_test(NAME smoke COMMAND test_smoke)
add_test(NAME fb COMMAND test_fb)
add_test(NAME gfx COMMAND test_gfx)
add_test(NAME gfx3d COMMAND test_gfx3d)
