/**
 * @file gm_scene_door_menu.c
 * @brief Everything you can do to a door other than open it.
 */
#include "../garagemate_i.h"

typedef enum {
    GmDoorMenuGuide,
    GmDoorMenuExport,
    GmDoorMenuRename,
    GmDoorMenuDelete,
} GmDoorMenuItem;

static void gm_door_menu_callback(void* context, uint32_t index) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

/** Show a one-button notice and wait for acknowledgement. */
static void gm_door_menu_notice(GarageMate* app, const char* header, const char* text) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, header, 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, text, 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, NULL, "OK", NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

static void gm_door_menu_export(GarageMate* app) {
    char path[GM_PATH_MAX];
    gm_door_sub_export_path(&app->draft, path, sizeof(path));

    GmTxStatus status =
        gm_radio_export(&app->radio, app->storage, &app->draft, app->draft_brand, path);

    if(status == GmTxOk) {
        gm_door_menu_notice(app, "Exported", path);
    } else {
        gm_door_menu_notice(app, "Export failed", gm_tx_status_text(status));
    }
}

/** @return true when the door was deleted and the scene should unwind. */
static bool gm_door_menu_delete(GarageMate* app) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Delete this door?", 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, app->draft.name, 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, "Cancel", NULL, "Delete");
    DialogMessageButton pressed = dialog_message_show(app->dialogs, message);
    dialog_message_free(message);

    if(pressed != DialogMessageButtonRight) return false;

    gm_store_delete(app->storage, &app->draft);
    gm_store_list_remove(&app->doors, app->door_index);
    return true;
}

void garagemate_scene_door_menu_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, app->draft.name);

    submenu_add_item(submenu, "Pairing guide", GmDoorMenuGuide, gm_door_menu_callback, app);

    // Exporting re-derives the payload, which only managed doors can do; an
    // imported door already is a .sub file.
    if(app->draft.managed && app->draft_brand != NULL) {
        submenu_add_item(submenu, "Export .sub", GmDoorMenuExport, gm_door_menu_callback, app);
    }

    submenu_add_item(submenu, "Rename", GmDoorMenuRename, gm_door_menu_callback, app);
    submenu_add_item(submenu, "Delete", GmDoorMenuDelete, gm_door_menu_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewSubmenu);
}

bool garagemate_scene_door_menu_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case GmDoorMenuGuide:
        if(app->draft_brand == NULL) {
            gm_door_menu_notice(app, "No guide", "This door was imported from a .sub file.");
            return true;
        }
        app->pair_step = 0;
        scene_manager_next_scene(app->scene_manager, GmScenePair);
        return true;

    case GmDoorMenuExport:
        gm_door_menu_export(app);
        return true;

    case GmDoorMenuRename:
        app->draft_is_new = false;
        scene_manager_next_scene(app->scene_manager, GmSceneName);
        return true;

    case GmDoorMenuDelete:
        if(gm_door_menu_delete(app)) {
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, GmSceneStart);
        }
        return true;

    default:
        return false;
    }
}

void garagemate_scene_door_menu_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
