/**
 * @file gm_scene_help.c
 * @brief The "how this works" screen.
 */
#include "../garagemate_i.h"

static const char* const gm_help_text =
    "GarageMate makes a NEW remote and teaches your opener to accept it. It "
    "does not copy an existing remote.\n"
    "\n"
    "Why: modern openers use rolling codes that change every press, so a copy "
    "is useless. Pairing is how real remotes are added, and it needs you at "
    "the motor unit pressing LEARN.\n"
    "\n"
    "To add a door:\n"
    "1. Add a door\n"
    "2. Pick your opener brand\n"
    "3. Pick the frequency\n"
    "4. Name it\n"
    "5. Follow the pairing steps\n"
    "\n"
    "Frequency: it is printed on the back of your old remote. 315 and 390 MHz "
    "are the usual US garage bands.\n"
    "\n"
    "Marked (blocked)? Your Flipper's region forbids that band. Settings > All "
    "frequencies opens up everything the radio supports, including 390 MHz. It "
    "applies only while this app runs.\n"
    "\n"
    "Not working? Try the other frequency for your brand, get within a few "
    "feet of the motor, and make sure you pressed LEARN just before sending.\n"
    "\n"
    "Careful: holding LEARN for six seconds erases every remote paired to the "
    "opener, including the ones in your car.\n"
    "\n"
    "Files: doors live in apps_data/garagemate and exported signals in "
    "subghz/garagemate. Both are plain text you can edit or back up.";

void garagemate_scene_help_on_enter(void* context) {
    GarageMate* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);
    widget_add_string_element(
        widget, 0, 0, AlignLeft, AlignTop, FontPrimary, "How this works");
    widget_add_line_element(widget, 0, 11, 128, 11);
    widget_add_text_scroll_element(widget, 0, 13, 128, 51, gm_help_text);

    view_dispatcher_switch_to_view(app->view_dispatcher, GmViewWidget);
}

bool garagemate_scene_help_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void garagemate_scene_help_on_exit(void* context) {
    GarageMate* app = context;
    widget_reset(app->widget);
}
