#include "gm_scene.h"

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
static void (*const gm_scene_on_enter_handlers[])(void*) = {
#include "gm_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
static bool (*const gm_scene_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "gm_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
static void (*const gm_scene_on_exit_handlers[])(void* context) = {
#include "gm_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers gm_scene_handlers = {
    .on_enter_handlers = gm_scene_on_enter_handlers,
    .on_event_handlers = gm_scene_on_event_handlers,
    .on_exit_handlers = gm_scene_on_exit_handlers,
    .scene_num = GmSceneCount,
};
