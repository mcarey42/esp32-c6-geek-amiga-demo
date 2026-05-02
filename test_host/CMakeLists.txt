cmake_minimum_required(VERSION 3.16)
project(esp32demo_host_tests C)

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

# Each test binary is added by later tasks. The smoke test exists from day one.
add_executable(test_smoke test_smoke.c)
target_link_libraries(test_smoke unity)

add_executable(test_fb test_fb.c)
target_link_libraries(test_fb unity)

# Add gfx sources to the host build (need the .c, not just headers)
add_executable(test_gfx test_gfx.c ../components/gfx/gfx.c ../components/gfx/font_5x7.c)
target_link_libraries(test_gfx unity ppm)
add_test(NAME gfx COMMAND test_gfx)

enable_testing()
add_test(NAME smoke COMMAND test_smoke)
add_test(NAME fb COMMAND test_fb)
