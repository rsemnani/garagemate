/**
 * @file gm_scene_settings.c
 * @brief Preferences, saved on the way out.
 */
#include "../garagemate_i.h"

/** "Auto" plus one entry per allowed repeat count. */
#define GM_REPEAT_CHOICES (GM_REPEATS_MAX + 1)

static const char* const gm_toggle_labels[] = {"OFF", "ON"};

static void gm_settings_repeats_changed(VariableItem* item) {
    GarageMate* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->settings.repeats = index;

    char text[8];
    if(index == GM_REPEATS_AUTO) {
        strlcpy(text, "Auto", sizeof(text));
    } else {
        snprintf(text, sizeof(text), "%u", (unsigned)index);
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
        list, "Presses per open", GM_REPEAT_CHOICES, gm_settings_repeats_changed, app);
    variable_item_set_current_value_index(item, app->settings.repeats);
    gm_settings_repeats_changed(item);

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
