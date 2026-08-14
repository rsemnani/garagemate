/**
 * @file gm_scene_capture.c
 * @brief Learn Genie codes over the air from a remote you own.
 *
 * Starts a receiver on the door's frequency and shows a live count as codes
 * arrive. A short timer nudges the view to redraw, since codes land on the
 * radio worker thread rather than in response to a keypress. BACK stops the
 * radio, saves what was captured, and returns to the walkthrough.
 */
#include "../garagemate_i.h"

/** How often the on-screen count refreshes, in milliseconds. */
#define GM_CAPTURE_REFRESH_MS 400

static void gm_capture_timer_callback(void* context) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventCaptureTick);
}

static void gm_capture_render(GarageMate* app) {
    Widget* widget = app->widget;
    widget_reset(widget);

    widget_add_string_element(
        widget, 0, 0, AlignLeft, AlignTop, FontPrimary, "Learning from remote");
    widget_add_line_element(widget, 0, 12, 128, 12);

    uint16_t total = app->capture ? gm_capture_total(app->capture) : 0;
    uint16_t stored = app->genie_seq.count;

    char line[32];
    snprintf(line, sizeof(line), "Codes: %u", (unsigned)stored);
    widget_add_string_element(widget, 64, 22, AlignCenter, AlignTop, FontBigNumbers, line);

    const char* hint = (total == 0) ?
                           "Press your Genie remote\nnear the Flipper" :
                           "Keep pressing, or BACK\nwhen you have 5-10";
    widget_add_string_multiline_element(
        widget, 64, 44, AlignCenter, AlignTop, FontSecondary, hint);
}

void garagemate_scene_capture_on_enter(void* context) {
    GarageMate* app = context;

    // Start from whatever is already stored for this door, so a second learn
    // session tops up the same run rather than replacing it.
    gm_genie_seq_load(app->storage, app->draft.id, &app->genie_seq);
    if(app->genie_seq.frequency == 0) app->genie_seq.frequency = app->draft.frequency;

    app->capture = gm_capture_alloc(app->radio.device);

    if(!gm_capture_start(app->capture, app->draft.frequency, &app->genie_seq)) {
        // Frequency unusable here (e.g. region-blocked and unlock is off).
        gm_capture_free(app->capture);
        app->capture = NULL;
    }

    app->capture_timer =
        furi_timer_alloc(gm_capture_timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->capture_timer, GM_CAPTURE_REFRESH_MS);

    gm_capture_render(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewWidget);
}

bool garagemate_scene_capture_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type == SceneManagerEventTypeCustom &&
       event.event == GmCustomEventCaptureTick) {
        gm_capture_render(app);
        return true;
    }
    return false;
}

void garagemate_scene_capture_on_exit(void* context) {
    GarageMate* app = context;

    if(app->capture_timer) {
        furi_timer_stop(app->capture_timer);
        furi_timer_free(app->capture_timer);
        app->capture_timer = NULL;
    }

    if(app->capture) {
        gm_capture_stop(app->capture);
        gm_capture_free(app->capture);
        app->capture = NULL;
    }

    // Persist whatever was learned; an empty run is fine and just means the
    // door has nothing to send yet.
    gm_genie_seq_save(app->storage, app->draft.id, &app->genie_seq);

    widget_reset(app->widget);
}
