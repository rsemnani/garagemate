/**
 * @file gm_radio.h
 * @brief Sub-GHz transmission: one door press in, one radio burst out.
 */
#pragma once

#include "../catalog/gm_brands.h"
#include "../model/gm_door.h"

#include <lib/subghz/devices/devices.h>
#include <lib/subghz/environment.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Outcome of a transmission or export. */
typedef enum {
    GmTxOk = 0,
    /** The brand has no generator; the walkthrough explains the alternative. */
    GmTxErrUnsupported,
    /** The frequency is outside this Flipper's region or hardware range. */
    GmTxErrFrequency,
    /** The payload could not be built or parsed. */
    GmTxErrPayload,
    /** The radio refused to start or never finished. */
    GmTxErrRadio,
    /** The imported .sub file is missing or unreadable. */
    GmTxErrFile,
} GmTxStatus;

/** Radio context, allocated once for the lifetime of the app. */
typedef struct {
    SubGhzEnvironment* environment;
    const SubGhzDevice* device;
} GmRadio;

/** Bring up the Sub-GHz environment and the internal CC1101. */
void gm_radio_alloc(GmRadio* radio);

/** Tear down everything gm_radio_alloc() set up. */
void gm_radio_free(GmRadio* radio);

/**
 * Send @p door's code @p presses times, as if pressing a real remote.
 *
 * For rolling-code brands each press consumes a counter value, and @p door's
 * counter is advanced in place. The caller is responsible for persisting the
 * door afterwards so the counter survives a reboot.
 */
GmTxStatus gm_radio_press(GmRadio* radio, GmDoor* door, const GmBrand* brand, uint8_t presses);

/** Write @p door's current payload to @p path as a stock-compatible .sub file. */
GmTxStatus gm_radio_export(
    GmRadio* radio,
    Storage* storage,
    const GmDoor* door,
    const GmBrand* brand,
    const char* path);

/** Transmit an existing .sub file verbatim, used by imported doors. */
GmTxStatus gm_radio_send_file(GmRadio* radio, Storage* storage, const char* sub_path);

/** @return true when @p frequency may legally be transmitted on this device. */
bool gm_radio_frequency_allowed(const GmRadio* radio, uint32_t frequency);

/** @return a short human-readable description of @p status. */
const char* gm_tx_status_text(GmTxStatus status);

#ifdef __cplusplus
}
#endif
