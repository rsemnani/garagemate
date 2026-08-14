/**
 * @file gm_scene_frequency.c
 * @brief Step 2 of the wizard: which frequency does the opener listen on?
 *
 * Frequencies the current region forbids are still listed, but marked, so the
 * user learns why their opener's band is unavailable instead of wondering
 * where it went.
 */
#include "../garagemate_i.h"

/** "433.92 MHz (blocked)" plus slack. */
#define GM_FREQ_LABEL_MAX 32

static void gm_scene_frequency_callback(void* context, uint32_t index) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void garagemate_scene_frequency_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;
    const GmBrand* brand = app->draft_brand;
    furi_assert(brand);

    submenu_reset(submenu);
    submenu_set_header(submenu, "Frequency");

    for(uint8_t i = 0; i < brand->freq_count; i++) {
        char formatted[16];
        gm_format_frequency(brand->freqs[i], formatted, sizeof(formatted));

        bool allowed = gm_radio_frequency_allowed(&app->radio, brand->freqs[i]);

        // submenu_add_item copies the label, so a stack buffer is enough.
        char label[GM_FREQ_LABEL_MAX];
        snprintf(label, sizeof(label), "%s%s", formatted, allowed ? "" : " (blocked)");

        submenu_add_item(submenu, label, i, gm_scene_frequency_callback, app);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewSubmenu);
}

bool garagemate_scene_frequency_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event >= app->draft_brand->freq_count) return false;

    app->draft.frequency = app->draft_brand->freqs[event.event];
    scene_manager_next_scene(app->scene_manager, GmSceneName);
    return true;
}

void garagemate_scene_frequency_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
