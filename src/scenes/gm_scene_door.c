/**
 * @file gm_scene_door.c
 * @brief A single door: what it is, and one button to open it.
 *
 * Entering this scene copies the selected door into GarageMate::draft. Every
 * action that follows -- opening, renaming, re-pairing, exporting -- works on
 * that copy, so the rest of the app never needs to know whether it is dealing
 * with a saved door or one the wizard is still building.
 */
#include "../garagemate_i.h"

static void gm_door_button_callback(GuiButtonType result, InputType type, void* context) {
    GarageMate* app = context;

    if(result == GuiButtonTypeRight) {
        if(type != InputTypeShort) return;
        view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPairNext);
        return;
    }

    if(result != GuiButtonTypeCenter) return;

    // "Hold to open" trades a little convenience for protection against
    // opening the garage with a pocket press.
    InputType wanted = app->settings.hold_to_open ? InputTypeLong : InputTypeShort;
    if(type != wanted) return;

    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventTxRun);
}

void garagemate_scene_door_on_enter(void* context) {
    GarageMate* app = context;

    GmDoor* door = garagemate_current_door(app);
    if(door == NULL) {
        // The door was deleted while we were away.
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, GmSceneStart);
        return;
    }

    app->draft = *door;
    app->draft_brand = gm_brand_by_id(door->brand_id);
    app->draft_is_new = false;

    Widget* widget = app->widget;
    widget_reset(widget);

    widget_add_string_element(
        widget, 0, 0, AlignLeft, AlignTop, FontPrimary, app->draft.name);
    widget_add_line_element(widget, 0, 11, 128, 11);

    const char* brand_name =
        (app->draft_brand != NULL) ? app->draft_brand->display : "Imported signal";
    widget_add_string_element(widget, 0, 15, AlignLeft, AlignTop, FontSecondary, brand_name);

    char detail[40];
    if(app->draft.all_bands && app->draft_brand != NULL) {
        snprintf(detail, sizeof(detail), "All %u bands", (unsigned)app->draft_brand->freq_count);
    } else {
        gm_format_frequency(app->draft.frequency, detail, sizeof(detail));
    }
    widget_add_string_element(widget, 0, 26, AlignLeft, AlignTop, FontSecondary, detail);

    if(app->draft.managed && app->draft_brand != NULL && app->draft_brand->rolling) {
        snprintf(detail, sizeof(detail), "Rolling code, sent %lu", (unsigned long)app->draft.counter);
    } else if(app->draft.managed) {
        snprintf(detail, sizeof(detail), "Fixed code");
    } else {
        snprintf(detail, sizeof(detail), "Replays a saved .sub");
    }
    widget_add_string_element(widget, 0, 37, AlignLeft, AlignTop, FontSecondary, detail);

    widget_add_button_element(
        widget,
        GuiButtonTypeCenter,
        app->settings.hold_to_open ? "Hold" : "OPEN",
        gm_door_button_callback,
        app);
    widget_add_button_element(widget, GuiButtonTypeRight, "More", gm_door_button_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewWidget);
}

bool garagemate_scene_door_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case GmCustomEventTxRun:
        scene_manager_next_scene(app->scene_manager, GmSceneOpen);
        return true;

    case GmCustomEventPairNext:
        scene_manager_next_scene(app->scene_manager, GmSceneDoorMenu);
        return true;

    default:
        return false;
    }
}

void garagemate_scene_door_on_exit(void* context) {
    GarageMate* app = context;
    widget_reset(app->widget);
}
