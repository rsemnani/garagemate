/**
 * @file garagemate.c
 * @brief Application entry point, wiring and shared helpers.
 */
#include "garagemate_i.h"

#define TAG "GarageMate"

GmDoor* garagemate_current_door(GarageMate* app) {
    furi_assert(app);
    if(app->door_index >= app->doors.count) return NULL;
    return &app->doors.items[app->door_index];
}

bool garagemate_save_door(GarageMate* app, const GmDoor* door) {
    furi_assert(app);
    furi_assert(door);

    bool ok = gm_store_save(app->storage, door);
    if(ok) gm_store_list_upsert(&app->doors, door);
    return ok;
}

void garagemate_notify(GarageMate* app, bool success) {
    furi_assert(app);
    if(!app->settings.feedback) return;
    notification_message(
        app->notifications, success ? &sequence_success : &sequence_error);
}

void garagemate_apply_region(GarageMate* app) {
    furi_assert(app);
    if(app->settings.unlock_frequencies) {
        gm_region_unlock(&app->region_guard);
    } else {
        gm_region_restore(&app->region_guard);
    }
}

static bool garagemate_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    GarageMate* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool garagemate_back_event_callback(void* context) {
    furi_assert(context);
    GarageMate* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static GarageMate* garagemate_alloc(void) {
    GarageMate* app = malloc(sizeof(GarageMate));
    memset(app, 0, sizeof(GarageMate));

    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->dialogs = furi_record_open(RECORD_DIALOGS);

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&gm_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, garagemate_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, garagemate_back_event_callback);

    app->submenu = submenu_alloc();
    app->widget = widget_alloc();
    app->popup = popup_alloc();
    app->text_input = text_input_alloc();
    app->var_item_list = variable_item_list_alloc();

    view_dispatcher_add_view(
        app->view_dispatcher, GmViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, GmViewWidget, widget_get_view(app->widget));
    view_dispatcher_add_view(app->view_dispatcher, GmViewPopup, popup_get_view(app->popup));
    view_dispatcher_add_view(
        app->view_dispatcher, GmViewTextInput, text_input_get_view(app->text_input));
    view_dispatcher_add_view(
        app->view_dispatcher,
        GmViewVarItemList,
        variable_item_list_get_view(app->var_item_list));

    gm_store_init(app->storage);
    gm_settings_load(app->storage, &app->settings);
    gm_store_load_all(app->storage, &app->doors);
    gm_radio_alloc(&app->radio);

    gm_region_guard_init(&app->region_guard);
    garagemate_apply_region(app);

    return app;
}

static void garagemate_free(GarageMate* app) {
    furi_assert(app);

    // Leave the radio exactly as we found it, so nothing else on the Flipper
    // inherits a widened region table.
    gm_region_restore(&app->region_guard);
    gm_radio_free(&app->radio);

    view_dispatcher_remove_view(app->view_dispatcher, GmViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, GmViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, GmViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, GmViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, GmViewVarItemList);

    submenu_free(app->submenu);
    widget_free(app->widget);
    popup_free(app->popup);
    text_input_free(app->text_input);
    variable_item_list_free(app->var_item_list);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t garagemate_app(void* p) {
    UNUSED(p);

    GarageMate* app = garagemate_alloc();

    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(app->scene_manager, GmSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    garagemate_free(app);
    return 0;
}
