/**
 * @file gm_scene_open.c
 * @brief Transmits the current door and reports what happened.
 *
 * Transmission blocks for up to a second or so. The popup is put on screen
 * first and the work is kicked off through a queued event, so the GUI service
 * has already drawn "Sending..." by the time the radio starts.
 */
#include "../garagemate_i.h"

/** How long the result stays on screen before returning, in milliseconds. */
#define GM_RESULT_TIMEOUT_MS 1400

static void gm_open_popup_callback(void* context) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPopupDone);
}

/** Send the door's code and persist whatever the transmission changed. */
static GmTxStatus gm_open_transmit(GarageMate* app) {
    if(!app->draft.managed) {
        // Imported doors are replayed byte for byte; there is no counter to
        // advance and nothing to write back.
        return gm_radio_send_file(&app->radio, app->storage, app->draft.sub_path);
    }

    const GmBrand* brand = app->draft_brand;
    if(brand == NULL) return GmTxErrUnsupported;

    if(brand->kind == GmProtoGenie) {
        // Genie doors replay a learned code, then advance. There is nothing to
        // generate, so this path is entirely separate from gm_radio_press.
        gm_genie_seq_load(app->storage, app->draft.id, &app->genie_seq);

        uint64_t code = 0;
        if(!gm_genie_seq_peek(&app->genie_seq, &code)) return GmTxErrNoCodes;

        uint32_t frequency = app->genie_seq.frequency ? app->genie_seq.frequency :
                                                        app->draft.frequency;
        GmTxStatus status = gm_radio_genie_send(&app->radio, code, frequency);

        if(status == GmTxOk) {
            gm_genie_seq_advance(&app->genie_seq);
            gm_genie_seq_save(app->storage, app->draft.id, &app->genie_seq);
        }
        return status;
    }

    uint8_t frame_repeat = (app->settings.frame_repeat != GM_FRAME_REPEAT_AUTO) ?
                               app->settings.frame_repeat :
                               brand->frame_repeat;
    GmTxStatus status = gm_radio_press(&app->radio, &app->draft, brand, frame_repeat);

    // The counter advances even when a later press fails, so persist it
    // regardless: re-sending an already-used code would be rejected anyway.
    if(brand->rolling) garagemate_save_door(app, &app->draft);

    return status;
}

void garagemate_scene_open_on_enter(void* context) {
    GarageMate* app = context;
    Popup* popup = app->popup;

    popup_reset(popup);
    popup_set_header(popup, "Sending", 64, 20, AlignCenter, AlignBottom);
    popup_set_text(popup, app->draft.name, 64, 32, AlignCenter, AlignTop);
    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewPopup);

    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventTxRun);
}

bool garagemate_scene_open_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case GmCustomEventTxRun: {
        app->last_status = gm_open_transmit(app);
        bool ok = (app->last_status == GmTxOk);

        // Walking the user forward is the whole point of the guide, so a send
        // that worked should land them on the next step without a keypress.
        if(ok && scene_manager_has_previous_scene(app->scene_manager, GmScenePair) &&
           app->draft_brand != NULL && app->pair_step + 1 < app->draft_brand->step_count) {
            app->pair_step++;
        }

        Popup* popup = app->popup;
        popup_set_header(popup, ok ? "Sent" : "Failed", 64, 20, AlignCenter, AlignBottom);
        popup_set_text(
            popup,
            ok ? app->draft.name : gm_tx_status_text(app->last_status),
            64,
            32,
            AlignCenter,
            AlignTop);
        popup_set_callback(popup, gm_open_popup_callback);
        popup_set_context(popup, app);
        popup_set_timeout(popup, GM_RESULT_TIMEOUT_MS);
        popup_enable_timeout(popup);

        // The popup arms its timer when the view is entered, and this view was
        // entered before the timeout was configured -- so re-enter it, or the
        // result would sit there until the user pressed something.
        view_dispatcher_switch_to_view(app->view_dispatcher, GmViewPopup);

        garagemate_notify(app, ok);
        return true;
    }

    case GmCustomEventPopupDone:
        scene_manager_previous_scene(app->scene_manager);
        return true;

    default:
        return false;
    }
}

void garagemate_scene_open_on_exit(void* context) {
    GarageMate* app = context;
    popup_reset(app->popup);
}
