/**
 * @file gm_settings.h
 * @brief User preferences, persisted as a hand-editable text file.
 */
#pragma once

#include "gm_door.h"

#include <stdbool.h>
#include <stdint.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Follow the brand's own frame repeat rather than a fixed override. */
#define GM_FRAME_REPEAT_AUTO 0

/** Largest frame repeat offered in the settings menu. */
#define GM_FRAME_REPEAT_MAX 30

/** Application preferences. */
typedef struct {
    /**
     * How many times the frame repeats within one transmission, or
     * GM_FRAME_REPEAT_AUTO to follow the brand.
     *
     * This lengthens a single press; it never sends a second code, which would
     * make the door reverse.
     */
    uint8_t frame_repeat;
    /** Play the LED/speaker/vibro feedback when transmitting. */
    bool feedback;
    /** Require a long press on OPEN, guarding against accidental opens. */
    bool hold_to_open;
    /**
     * Widen the Sub-GHz region table to the CC1101's full range while the app
     * runs, so bands the provisioned region omits (notably 390 MHz) can be
     * transmitted on. See radio/gm_region.h.
     */
    bool unlock_frequencies;
    /**
     * Id of the door opened most recently, or an empty string when there is
     * none yet.
     *
     * The app jumps straight to this door's screen on launch, so that walking
     * up to the garage is: quick-access button, then OK. Back still steps out
     * to the full list.
     */
    char last_door_id[GM_ID_MAX];
} GmSettings;

/** Populate @p settings with defaults. */
void gm_settings_default(GmSettings* settings);

/** Load settings, falling back to defaults when the file is missing or bad. */
void gm_settings_load(Storage* storage, GmSettings* settings);

/** Persist @p settings. @return true on success. */
bool gm_settings_save(Storage* storage, const GmSettings* settings);

#ifdef __cplusplus
}
#endif
