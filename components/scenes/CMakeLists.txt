idf_component_register(
    SRCS "scene_01_boot_title.c"
         "scene_02_copper_scroller.c"
         "scene_03_starfield.c"
         "scene_04_cube.c"
         "scene_05_plasma.c"
         "scene_08_silhouette.c"
         "scene_08_frames.c"
    INCLUDE_DIRS "include"
    REQUIRES director gfx gfx3d fb)
