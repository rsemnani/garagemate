/**
 * @file gm_scene.h
 * @brief Scene identifiers and handler declarations, generated from
 *        gm_scene_config.h.
 */
#pragma once

#include <gui/scene_manager.h>

/** Scene identifiers, one per entry in gm_scene_config.h. */
typedef enum {
#define ADD_SCENE(prefix, name, id) GmScene##id,
#include "gm_scene_config.h"
#undef ADD_SCENE
    GmSceneCount,
} GmSceneId;

extern const SceneManagerHandlers gm_scene_handlers;

#define ADD_SCENE(prefix, name, id)                                       \
    void prefix##_scene_##name##_on_enter(void* context);                 \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event); \
    void prefix##_scene_##name##_on_exit(void* context);
#include "gm_scene_config.h"
#undef ADD_SCENE
