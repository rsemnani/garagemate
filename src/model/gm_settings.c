#include "gm_settings.h"
#include "gm_paths.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>

#define TAG "GarageMate"

#define GM_SETTINGS_FILETYPE "GarageMate Settings"
#define GM_SETTINGS_VERSION  1

void gm_settings_default(GmSettings* settings) {
    furi_assert(settings);
    settings->frame_repeat = GM_FRAME_REPEAT_AUTO;
    settings->feedback = true;
    settings->hold_to_open = false;
    settings->unlock_frequencies = false;
}

void gm_settings_load(Storage* storage, GmSettings* settings) {
    furi_assert(storage);
    furi_assert(settings);
    gm_settings_default(settings);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* buffer = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(ff, GM_SETTINGS_PATH)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, buffer, &version)) break;
        if(furi_string_cmp_str(buffer, GM_SETTINGS_FILETYPE) != 0) break;
        if(version != GM_SETTINGS_VERSION) break;

        // Every field is optional, and each read rewinds first: FlipperFormat
        // scans forward from the cursor, so without this a key that is missing
        // or out of order would swallow every field after it.
        uint32_t frame_repeat = 0;
        flipper_format_rewind(ff);
        if(flipper_format_read_uint32(ff, "FrameRepeat", &frame_repeat, 1)) {
            settings->frame_repeat = (frame_repeat > GM_FRAME_REPEAT_MAX) ?
                                         GM_FRAME_REPEAT_MAX :
                                         (uint8_t)frame_repeat;
        }

        flipper_format_rewind(ff);
        flipper_format_read_bool(ff, "Feedback", &settings->feedback, 1);

        flipper_format_rewind(ff);
        flipper_format_read_bool(ff, "HoldToOpen", &settings->hold_to_open, 1);

        flipper_format_rewind(ff);
        flipper_format_read_bool(ff, "UnlockFrequencies", &settings->unlock_frequencies, 1);
    } while(false);

    furi_string_free(buffer);
    flipper_format_free(ff);
}

bool gm_settings_save(Storage* storage, const GmSettings* settings) {
    furi_assert(storage);
    furi_assert(settings);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool ok = false;

    do {
        if(!flipper_format_file_open_always(ff, GM_SETTINGS_PATH)) break;
        if(!flipper_format_write_header_cstr(ff, GM_SETTINGS_FILETYPE, GM_SETTINGS_VERSION)) break;

        uint32_t frame_repeat = settings->frame_repeat;
        if(!flipper_format_write_uint32(ff, "FrameRepeat", &frame_repeat, 1)) break;

        bool feedback = settings->feedback;
        if(!flipper_format_write_bool(ff, "Feedback", &feedback, 1)) break;

        bool hold = settings->hold_to_open;
        if(!flipper_format_write_bool(ff, "HoldToOpen", &hold, 1)) break;

        bool unlock = settings->unlock_frequencies;
        if(!flipper_format_write_bool(ff, "UnlockFrequencies", &unlock, 1)) break;

        ok = true;
    } while(false);

    flipper_format_free(ff);

    if(!ok) FURI_LOG_E(TAG, "Failed to save settings");
    return ok;
}
