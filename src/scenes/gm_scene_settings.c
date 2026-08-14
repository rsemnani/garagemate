/**
 * @file gm_scene_settings.c
 * @brief Preferences, saved on the way out.
 */
#include "../garagemate_i.h"

static const char* const gm_toggle_labels[] = {"OFF", "ON"};

/**
 * Offered signal lengths. Index 0 follows the brand; the rest lengthen a single
 * press. There is deliberately no way to send more than one code per open --
 * that reverses the door rather than opening it further.
 */
static const uint8_t gm_frame_repeat_choices[] = {GM_FRAME_REPEAT_AUTO, 5, 10, 15, 20, 30};

static void gm_settings_frame_repeat_changed(VariableItem* item) {
    GarageMate* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->settings.frame_repeat = gm_frame_repeat_choices[index];

    char text[8];
    if(app->settings.frame_repeat == GM_FRAME_REPEAT_AUTO) {
        strlcpy(text, "Auto", sizeof(text));
    } else {
        snprintf(text, sizeof(text), "%u", (unsigned)app->settings.frame_repeat);
    }
    variable_item_set_current_value_text(item, text);
}

static void gm_settings_feedback_changed(VariableItem* item) {
    GarageMate* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->settings.feedback = (index == 1);
    variable_item_set_current_value_text(item, gm_toggle_labels[index]);
}

static void gm_settings_hold_changed(VariableItem* item) {
    GarageMate* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->settings.hold_to_open = (index == 1);
    variable_item_set_current_value_text(item, gm_toggle_labels[index]);
}

static void gm_settings_unlock_changed(VariableItem* item) {
    GarageMate* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->settings.unlock_frequencies = (index == 1);
    variable_item_set_current_value_text(item, gm_toggle_labels[index]);

    // Apply straight away rather than on exit: the frequency picker asks the
    // radio what is permitted, so the change must be live before it is opened.
    garagemate_apply_region(app);
}

void garagemate_scene_settings_on_enter(void* context) {
    GarageMate* app = context;
    VariableItemList* list = app->var_item_list;

    variable_item_list_reset(list);

    VariableItem* item = variable_item_list_add(
        list,
        "Signal length",
        COUNT_OF(gm_frame_repeat_choices),
        gm_settings_frame_repeat_changed,
        app);

    uint8_t selected = 0;
    for(size_t i = 0; i < COUNT_OF(gm_frame_repeat_choices); i++) {
        if(gm_frame_repeat_choices[i] == app->settings.frame_repeat) selected = i;
    }
    variable_item_set_current_value_index(item, selected);
    gm_settings_frame_repeat_changed(item);

    item = variable_item_list_add(list, "Buzz on send", 2, gm_settings_feedback_changed, app);
    variable_item_set_current_value_index(item, app->settings.feedback ? 1 : 0);
    variable_item_set_current_value_text(item, gm_toggle_labels[app->settings.feedback ? 1 : 0]);

    item = variable_item_list_add(list, "Hold to open", 2, gm_settings_hold_changed, app);
    variable_item_set_current_value_index(item, app->settings.hold_to_open ? 1 : 0);
    variable_item_set_current_value_text(
        item, gm_toggle_labels[app->settings.hold_to_open ? 1 : 0]);

    item = variable_item_list_add(list, "All frequencies", 2, gm_settings_unlock_changed, app);
    variable_item_set_current_value_index(item, app->settings.unlock_frequencies ? 1 : 0);
    variable_item_set_current_value_text(
        item, gm_toggle_labels[app->settings.unlock_frequencies ? 1 : 0]);

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewVarItemList);
}

bool garagemate_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void garagemate_scene_settings_on_exit(void* context) {
    GarageMate* app = context;
    variable_item_list_reset(app->var_item_list);
    gm_settings_save(app->storage, &app->settings);
}
