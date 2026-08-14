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

/** Item id for the multi-band choice, kept clear of the per-frequency ids. */
#define GM_FREQ_ITEM_ALL GM_FREQ_MAX

void garagemate_scene_frequency_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;
    const GmBrand* brand = app->draft_brand;
    furi_assert(brand);

    submenu_reset(submenu);
    submenu_set_header(submenu, "Frequency");

    // Tri-band receivers listen on all of their frequencies, and a real remote
    // transmits on all of them, so offer that first and by default.
    if(brand->multiband) {
        size_t usable = 0;
        for(uint8_t i = 0; i < brand->freq_count; i++) {
            if(gm_radio_frequency_allowed(&app->radio, brand->freqs[i])) usable++;
        }

        char label[GM_FREQ_LABEL_MAX];
        snprintf(
            label,
            sizeof(label),
            "All bands (%u of %u)",
            (unsigned)usable,
            (unsigned)brand->freq_count);
        submenu_add_item(submenu, label, GM_FREQ_ITEM_ALL, gm_scene_frequency_callback, app);
    }

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

    if(event.event == GM_FREQ_ITEM_ALL) {
        app->draft.all_bands = true;
        app->draft.frequency = app->draft_brand->freqs[0];
    } else if(event.event < app->draft_brand->freq_count) {
        app->draft.all_bands = false;
        app->draft.frequency = app->draft_brand->freqs[event.event];
    } else {
        return false;
    }

    scene_manager_next_scene(app->scene_manager, GmSceneName);
    return true;
}

void garagemate_scene_frequency_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
