/**
 * @file gm_scene_pair.c
 * @brief The guided pairing walkthrough.
 *
 * Steps come from the brand catalogue, so this scene contains no knowledge of
 * any particular opener. A step flagged as a transmit step gets a Send button
 * wired to the radio; every other step just moves forward or back.
 */
#include "../garagemate_i.h"

static void gm_pair_button_callback(GuiButtonType result, InputType type, void* context) {
    GarageMate* app = context;
    if(type != InputTypeShort) return;

    switch(result) {
    case GuiButtonTypeLeft:
        view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPairBack);
        break;
    case GuiButtonTypeCenter:
        view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPairSend);
        break;
    case GuiButtonTypeRight:
        view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPairNext);
        break;
    default:
        break;
    }
}

/** Draw the current step. Called again whenever the step changes. */
static void gm_pair_render(GarageMate* app) {
    const GmBrand* brand = app->draft_brand;
    furi_assert(brand);
    furi_assert(app->pair_step < brand->step_count);

    const GmPairStep* step = &brand->steps[app->pair_step];
    Widget* widget = app->widget;

    widget_reset(widget);

    char title[40];
    snprintf(
        title,
        sizeof(title),
        "%s  (%u/%u)",
        step->title,
        (unsigned)(app->pair_step + 1),
        (unsigned)brand->step_count);
    widget_add_string_element(widget, 0, 0, AlignLeft, AlignTop, FontPrimary, title);
    widget_add_line_element(widget, 0, 11, 128, 11);

    widget_add_text_scroll_element(widget, 0, 13, 128, 38, step->body);

    if(app->pair_step > 0) {
        widget_add_button_element(
            widget, GuiButtonTypeLeft, "Back", gm_pair_button_callback, app);
    }

    if(step->transmit) {
        widget_add_button_element(
            widget, GuiButtonTypeCenter, "Send", gm_pair_button_callback, app);
    }

    bool last = (app->pair_step + 1 >= brand->step_count);
    widget_add_button_element(
        widget, GuiButtonTypeRight, last ? "Done" : "Next", gm_pair_button_callback, app);
}

void garagemate_scene_pair_on_enter(void* context) {
    GarageMate* app = context;
    gm_pair_render(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewWidget);
}

bool garagemate_scene_pair_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;
    const GmBrand* brand = app->draft_brand;

    // BACK walks the walkthrough backwards before it leaves the scene, so a
    // mistyped step is one press away from being corrected.
    if(event.type == SceneManagerEventTypeBack && app->pair_step > 0) {
        app->pair_step--;
        gm_pair_render(app);
        return true;
    }

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case GmCustomEventPairBack:
        if(app->pair_step > 0) {
            app->pair_step--;
            gm_pair_render(app);
        }
        return true;

    case GmCustomEventPairNext:
        if(app->pair_step + 1 < brand->step_count) {
            app->pair_step++;
            gm_pair_render(app);
        } else {
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, GmSceneStart);
        }
        return true;

    case GmCustomEventPairSend:
        // Genie has nothing to transmit yet -- its "Send" step means "go learn
        // codes from the remote" rather than "fire a generated code".
        if(brand->kind == GmProtoGenie) {
            scene_manager_next_scene(app->scene_manager, GmSceneCapture);
        } else {
            scene_manager_next_scene(app->scene_manager, GmSceneOpen);
        }
        return true;

    default:
        return false;
    }
}

void garagemate_scene_pair_on_exit(void* context) {
    GarageMate* app = context;
    widget_reset(app->widget);
}
