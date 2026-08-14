/**
 * @file gm_settings.h
 * @brief User preferences, persisted as a hand-editable text file.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Use the brand's own repeat count rather than a fixed override. */
#define GM_REPEATS_AUTO 0

/** Largest repeat override offered in the settings menu. */
#define GM_REPEATS_MAX 8

/** Application preferences. */
typedef struct {
    /** Burst repeats per press, or GM_REPEATS_AUTO to follow the brand. */
    uint8_t repeats;
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
