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

add_library(director_host STATIC
    ../components/director/director.c
    ../components/director/transitions.c)
target_include_directories(director_host PUBLIC
    ../components/director/include
    ../components/fb/include
    host_shims)

add_library(scenes_host STATIC
    ../components/scenes/scene_01_boot_title.c
    ../components/scenes/scene_02_copper_scroller.c
    ../components/scenes/scene_03_starfield.c
    ../components/scenes/scene_04_cube.c
    ../components/scenes/scene_05_plasma.c
    ../components/scenes/scene_08_silhouette.c
    ../components/scenes/scene_08_frames.c)
target_include_directories(scenes_host PUBLIC
    ../components/scenes/include
    ../components/director/include
    ../components/gfx/include
    ../components/gfx3d/include
    ../components/fb/include
    host_shims)

add_library(prefetch_host STATIC
    ../components/prefetch/prefetch.c)
target_include_directories(prefetch_host PUBLIC
    ../components/prefetch/include
    host_shims)
target_compile_definitions(prefetch_host PUBLIC PREFETCH_HOST_TEST=1)

# Each test binary is added by later tasks. The smoke test exists from day one.
add_executable(test_smoke test_smoke.c)
target_link_libraries(test_smoke unity)

add_executable(test_fb test_fb.c)
target_link_libraries(test_fb unity)

add_executable(test_gfx test_gfx.c)
target_link_libraries(test_gfx unity ppm gfx_host)

add_executable(test_gfx3d test_gfx3d.c)
target_link_libraries(test_gfx3d unity gfx3d_host gfx_host m)

add_executable(test_director test_director.c)
target_link_libraries(test_director unity director_host)

add_executable(test_scene_02 test_scene_02.c)
target_link_libraries(test_scene_02 unity ppm scenes_host gfx_host m)

add_executable(test_scene_04 test_scene_04.c)
target_link_libraries(test_scene_04 unity ppm scenes_host gfx3d_host gfx_host m)

add_executable(test_prefetch test_prefetch.c)
target_link_libraries(test_prefetch unity prefetch_host)

add_executable(test_scene_08 test_scene_08.c)
target_link_libraries(test_scene_08 unity ppm scenes_host gfx_host)

add_executable(test_scene_01 test_scene_01.c)
target_link_libraries(test_scene_01 unity ppm scenes_host gfx_host m)

add_executable(test_scene_03 test_scene_03.c)
target_link_libraries(test_scene_03 unity ppm scenes_host gfx3d_host gfx_host m)

add_executable(test_scene_05 test_scene_05.c)
target_link_libraries(test_scene_05 unity ppm scenes_host gfx_host m)

add_test(NAME smoke COMMAND test_smoke)
add_test(NAME fb COMMAND test_fb)
add_test(NAME gfx COMMAND test_gfx)
add_test(NAME gfx3d COMMAND test_gfx3d)
add_test(NAME director COMMAND test_director)
add_test(NAME scene_01 COMMAND test_scene_01)
add_test(NAME scene_02 COMMAND test_scene_02)
add_test(NAME scene_03 COMMAND test_scene_03)
add_test(NAME scene_04 COMMAND test_scene_04)
add_test(NAME scene_05 COMMAND test_scene_05)
add_test(NAME prefetch COMMAND test_prefetch)
add_test(NAME scene_08 COMMAND test_scene_08)
