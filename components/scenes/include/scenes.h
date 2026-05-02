#pragma once
#include "scene.h"
extern const scene_t SCENE_02_COPPER_SCROLLER;
extern const scene_t SCENE_04_CUBE;
extern const scene_t SCENE_08_SILHOUETTE;

/* Set the boot-time pre-loaded silhouette frame buffer. Call from
 * app_main between prefetch_load_all() and director_start(). Pass NULL
 * to leave the scene rendering its placeholder background. */
void scene_08_silhouette_set_frames(const void *frames);
