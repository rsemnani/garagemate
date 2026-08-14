/**
 * @file gm_door.h
 * @brief A saved door and its on-disk representation.
 *
 * Each door is one small FlipperFormat file under /ext/apps_data/garagemate/doors.
 * The format is plain text on purpose: you can open a .door file on the SD card
 * and hand-edit the counter, rename it, or copy it to another Flipper.
 *
 * Doors come in two flavours:
 *  - managed  -- GarageMate generated the remote and owns serial/button/counter,
 *                so it re-derives the payload before every transmission.
 *  - imported -- the door points at an existing .sub file, which is sent as-is.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../catalog/gm_brands.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum doors held in memory (and therefore listed in the UI). */
#define GM_DOORS_MAX 24

/** Buffer size for a door's display name, including the terminator. */
#define GM_NAME_MAX 28

/** Buffer size for a door identifier ("door_1a2b3c4d"). */
#define GM_ID_MAX 24

/** Buffer size for filesystem paths handled by the app. */
#define GM_PATH_MAX 128

/** A single saved door. */
typedef struct {
    char id[GM_ID_MAX];
    char name[GM_NAME_MAX];
    char brand_id[GM_BRAND_ID_MAX];
    uint32_t frequency;
    /** Remote serial number. Meaningful only when @c managed is true. */
    uint32_t serial;
    /** Button/channel code. Meaningful only when @c managed is true. */
    uint8_t button;
    /** Rolling counter; incremented and persisted after every transmission. */
    uint32_t counter;
    /** True when GarageMate generated this remote and re-derives its payload. */
    bool managed;
    /**
     * Send every press on all of the brand's frequencies rather than just
     * @c frequency, matching how a tri-band remote behaves.
     */
    bool all_bands;
    /** Source .sub file for imported doors; empty for managed ones. */
    char sub_path[GM_PATH_MAX];
    /** List icon (GmIconId): garage door by default. */
    uint8_t icon;
} GmDoor;

/** An in-memory list of doors, loaded at startup. */
typedef struct {
    GmDoor items[GM_DOORS_MAX];
    size_t count;
} GmDoorList;

/** Reset @p door to a blank managed door with a fresh random id and serial. */
void gm_door_init(GmDoor* door);

/**
 * Configure @p door for @p brand: identity, default frequency and button, plus
 * the brand's serial pattern and starting counter.
 *
 * Call this after gm_door_init() when the user picks a brand. Getting the
 * serial pattern wrong is not harmless -- see GmBrand::serial_mask.
 */
void gm_door_apply_brand(GmDoor* door, const GmBrand* brand);

/** Fill @p out with the absolute path of @p door's .door file. */
void gm_door_path(const GmDoor* door, char* out, size_t out_size);

/** Fill @p out with the absolute path of @p door's exported .sub file. */
void gm_door_sub_export_path(const GmDoor* door, char* out, size_t out_size);

#ifdef __cplusplus
}
#endif
