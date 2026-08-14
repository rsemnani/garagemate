/**
 * @file gm_scene_brand.c
 * @brief Step 1 of the wizard: which opener do you have?
 */
#include "../garagemate_i.h"

static void gm_scene_brand_callback(void* context, uint32_t index) {
    GarageMate* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void garagemate_scene_brand_on_enter(void* context) {
    GarageMate* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Pick your opener");

    for(size_t i = 0; i < gm_brand_count(); i++) {
        submenu_add_item(
            submenu, gm_brand_at(i)->display, i, gm_scene_brand_callback, app);
    }

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, GmSceneBrand));

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewSubmenu);
}

bool garagemate_scene_brand_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type != SceneManagerEventTypeCustom) return false;

    const GmBrand* brand = gm_brand_at(event.event);
    if(brand == NULL) return false;

    scene_manager_set_scene_state(app->scene_manager, GmSceneBrand, event.event);
    app->draft_brand = brand;
    app->pair_step = 0;

    if(brand->kind == GmProtoManual) {
        // Nothing to generate or capture, so skip straight to the explanation
        // instead of walking the user through a door that cannot transmit.
        app->draft_is_new = false;
        scene_manager_next_scene(app->scene_manager, GmScenePair);
        return true;
    }

    gm_door_init(&app->draft);
    gm_door_apply_brand(&app->draft, brand);

    scene_manager_next_scene(app->scene_manager, GmSceneFrequency);
    return true;
}

void garagemate_scene_brand_on_exit(void* context) {
    GarageMate* app = context;
    submenu_reset(app->submenu);
}
