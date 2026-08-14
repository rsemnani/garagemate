/**
 * @file gm_scene_icon.c
 * @brief Choose the icon shown beside a door in the list.
 */
#include "../garagemate_i.h"

static void gm_scene_icon_callback(void* context, uint32_t index) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void garagemate_scene_icon_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Icon");

    for(uint32_t i = 0; i < GmIconCount; i++) {
        submenu_add_item(submenu, gm_icon_label(i), i, gm_scene_icon_callback, app);
    }
    submenu_set_selected_item(submenu, gm_icon_sanitize(app->draft.icon));

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewSubmenu);
}

bool garagemate_scene_icon_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event >= GmIconCount) return false;

    app->draft.icon = (uint8_t)event.event;
    garagemate_save_door(app, &app->draft);
    scene_manager_previous_scene(app->scene_manager);
    return true;
}

void garagemate_scene_icon_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
