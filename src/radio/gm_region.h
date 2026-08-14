/**
 * @file gm_region.h
 * @brief Opt-in removal of the Sub-GHz region restriction.
 *
 * The firmware decides whether a frequency may be transmitted on by walking a
 * band table, and nothing else: furi_hal_subghz_set_frequency() consults
 * furi_hal_region_is_frequency_allowed() and drops the radio to receive-only
 * when it says no. That table is replaceable at runtime through the exported
 * furi_hal_region_set(), so an app can widen it to everything the CC1101 can
 * physically tune.
 *
 * This matters because a US-provisioned Flipper allows roughly 304-322,
 * 433.05-434.79 and 915-928 MHz, which leaves out 390 MHz -- a band plenty of
 * Chamberlain and LiftMaster openers use, and one those openers are themselves
 * licensed to transmit on.
 *
 * The change lives in RAM only. It is never written to flash, GarageMate puts
 * the original table back when it exits, and a reboot would clear it anyway.
 */
#pragma once

#include <furi_hal_region.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ownership note: furi_hal_region_set() frees whatever region it is replacing,
 * so this holds a *copy* of the original table rather than a pointer to it.
 * Once handed back to the HAL the copy belongs to the HAL, not to us.
 */
typedef struct {
    /** Heap copy of the table in force before unlocking, or NULL. */
    FuriHalRegion* saved;
    /** True while the widened table is installed. */
    bool active;
} GmRegionGuard;

/** Prepare @p guard. Does not touch the radio. */
void gm_region_guard_init(GmRegionGuard* guard);

/**
 * Replace the region table with every band the CC1101 supports.
 *
 * @return true when the widened table is in force (including when it already
 *         was).
 */
bool gm_region_unlock(GmRegionGuard* guard);

/** Put the original region table back. Safe to call when never unlocked. */
void gm_region_restore(GmRegionGuard* guard);

/** @return the two-letter code of the region currently in force. */
const char* gm_region_current_name(void);

#ifdef __cplusplus
}
#endif
