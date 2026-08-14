/**
 * @file gm_scene_import.c
 * @brief Adopt an existing .sub file as a named door.
 *
 * Useful for signals made by the stock Sub-GHz app -- including Security+ 1.0
 * remotes from "Add Manually", which GarageMate cannot generate itself.
 */
#include "../garagemate_i.h"

#include <toolbox/path.h>

static void gm_import_notice(GarageMate* app, const char* header, const char* text) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, header, 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, text, 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, NULL, "OK", NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

/**
 * Read the parts of a .sub file GarageMate needs to list and replay it.
 *
 * @return false when the file is not a Sub-GHz key file at all.
 */
static bool gm_import_inspect(
    GarageMate* app,
    const char* path,
    uint32_t* frequency,
    bool* is_raw) {
    FlipperFormat* ff = flipper_format_file_alloc(app->storage);
    FuriString* buffer = furi_string_alloc();
    bool ok = false;
    *is_raw = false;

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, buffer, &version)) break;
        if(furi_string_cmp_str(buffer, SUBGHZ_KEY_FILE_TYPE) != 0) break;
        if(!flipper_format_read_uint32(ff, "Frequency", frequency, 1)) break;

        if(flipper_format_read_string(ff, "Protocol", buffer)) {
            *is_raw = (furi_string_cmp_str(buffer, "RAW") == 0);
        }
        ok = true;
    } while(false);

    furi_string_free(buffer);
    flipper_format_free(ff);
    return ok;
}

void garagemate_scene_import_on_enter(void* context) {
    GarageMate* app = context;

    FuriString* path = furi_string_alloc_set(GM_SUBGHZ_DIR);
    FuriString* name = furi_string_alloc();

    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, GM_SUB_EXT, NULL);
    options.base_path = GM_SUBGHZ_DIR;

    if(dialog_file_browser_show(app->dialogs, path, path, &options)) {
        const char* chosen = furi_string_get_cstr(path);
        uint32_t frequency = 0;
        bool is_raw = false;

        if(!gm_import_inspect(app, chosen, &frequency, &is_raw)) {
            gm_import_notice(app, "Cannot import", "That is not a Sub-GHz signal file.");
        } else if(is_raw) {
            // RAW files are sample streams; replaying them needs the stock
            // app's encoder worker, which GarageMate does not drive.
            gm_import_notice(
                app, "RAW not supported", "Open RAW captures in the stock Sub-GHz app.");
        } else if(app->doors.count >= GM_DOORS_MAX) {
            gm_import_notice(app, "List full", "Delete a door before importing another.");
        } else {
            GmDoor door;
            gm_door_init(&door);
            door.managed = false;
            door.frequency = frequency;
            strlcpy(door.sub_path, chosen, sizeof(door.sub_path));

            path_extract_filename(path, name, true);
            strlcpy(door.name, furi_string_get_cstr(name), sizeof(door.name));

            if(garagemate_save_door(app, &door)) {
                garagemate_notify(app, true);
            } else {
                gm_import_notice(app, "Import failed", "Could not write the door record.");
            }
        }
    }

    furi_string_free(name);
    furi_string_free(path);

    // The browser owned the screen; hand control back to the door list.
    view_dispatcher_send_custom_event(app->view_dispatcher, GmCustomEventPopupDone);
}

bool garagemate_scene_import_on_event(void* context, SceneManagerEvent event) {
    GarageMate* app = context;

    if(event.type == SceneManagerEventTypeCustom && event.event == GmCustomEventPopupDone) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

void garagemate_scene_import_on_exit(void* context) {
    UNUSED(context);
}
