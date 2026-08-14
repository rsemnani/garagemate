/**
 * @file gm_scene_name.c
 * @brief Step 3 of the wizard, and the rename action: give the door a name.
 */
#include "../garagemate_i.h"

static void gm_scene_name_callback(void* context) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventNameDone);
}

void garagemate_scene_name_on_enter(void* context) {
    GarageMate* app = context;
    TextInput* text_input = app->text_input;

    strlcpy(app->name_buf, app->draft.name, sizeof(app->name_buf));

    text_input_reset(text_input);
    text_input_set_header_text(text_input, "Name this door");
    text_input_set_result_callback(
        text_input,
        gm_scene_name_callback,
        app,
        app->name_buf,
        sizeof(app->name_buf),
        // A brand-new door carries the placeholder "New door", which should
        // vanish on the first keypress; a rename should keep the old name.
        app->draft_is_new);

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewTextInput);
}

bool garagemate_scene_name_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event != GmCustomEventNameDone) return false;

    strlcpy(app->draft.name, app->name_buf, sizeof(app->draft.name));

    if(!garagemate_save_door(app, &app->draft)) {
        garagemate_notify(app, false);
        return true;
    }

    if(app->draft_is_new) {
        app->pair_step = 0;
        scene_manager_next_scene(app->scene_manager, GmScenePair);
    } else {
        scene_manager_previous_scene(app->scene_manager);
    }
    return true;
}

void garagemate_scene_name_on_exit(void* context) {
    GarageMate* app = context;
    text_input_reset(app->text_input);
}
