/**
 * @file gm_scene_start.c
 * @brief Home screen: your saved doors, then the things you can do.
 */
#include "../garagemate_i.h"

/**
 * Action item ids.
 *
 * Door entries use their list index as the item id, so actions start above
 * GM_DOORS_MAX to keep the two ranges apart.
 */
typedef enum {
    GmStartActionAdd = GM_DOORS_MAX,
    GmStartActionImport,
    GmStartActionSettings,
    GmStartActionHelp,
} GmStartAction;

static void gm_scene_start_callback(void* context, uint32_t index) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void garagemate_scene_start_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, app->doors.count > 0 ? "Your doors" : GM_APP_NAME);

    for(size_t i = 0; i < app->doors.count; i++) {
        submenu_add_item(
            submenu, app->doors.items[i].name, i, gm_scene_start_callback, app);
    }

    submenu_add_item(submenu, "Add a door", GmStartActionAdd, gm_scene_start_callback, app);
    submenu_add_item(
        submenu, "Import saved .sub", GmStartActionImport, gm_scene_start_callback, app);
    submenu_add_item(submenu, "Settings", GmStartActionSettings, gm_scene_start_callback, app);
    submenu_add_item(submenu, "How this works", GmStartActionHelp, gm_scene_start_callback, app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, GmSceneStart));

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewSubmenu);
}

bool garagemate_scene_start_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;

    scene_manager_set_scene_state(app->scene_manager, GmSceneStart, event.event);

    switch(event.event) {
    case GmStartActionAdd:
        app->draft_is_new = true;
        scene_manager_next_scene(app->scene_manager, GmSceneBrand);
        return true;

    case GmStartActionImport:
        scene_manager_next_scene(app->scene_manager, GmSceneImport);
        return true;

    case GmStartActionSettings:
        scene_manager_next_scene(app->scene_manager, GmSceneSettings);
        return true;

    case GmStartActionHelp:
        scene_manager_next_scene(app->scene_manager, GmSceneHelp);
        return true;

    default:
        if(event.event < app->doors.count) {
            app->door_index = event.event;
            scene_manager_next_scene(app->scene_manager, GmSceneDoor);
            return true;
        }
        return false;
    }
}

void garagemate_scene_start_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
