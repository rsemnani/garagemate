/**
 * @file gm_door_store.h
 * @brief Loading, saving and deleting door records on the SD card.
 */
#pragma once

#include "gm_door.h"

#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create the app's directories if they do not exist yet. */
void gm_store_init(Storage* storage);

/**
 * Replace @p list with every valid door record found on disk.
 *
 * Unreadable or malformed files are skipped rather than aborting the scan, so
 * one corrupt record never hides the rest of the doors.
 */
void gm_store_load_all(Storage* storage, GmDoorList* list);

/** Write @p door to disk, overwriting any previous record with the same id. */
bool gm_store_save(Storage* storage, const GmDoor* door);

/** Remove @p door's record and its exported .sub file. */
bool gm_store_delete(Storage* storage, const GmDoor* door);

/** Remove the door at @p index from @p list, keeping the remaining order. */
void gm_store_list_remove(GmDoorList* list, size_t index);

/** Append @p door to @p list. @return false when the list is full. */
bool gm_store_list_add(GmDoorList* list, const GmDoor* door);

/** Replace the entry in @p list whose id matches @p door, or append it. */
void gm_store_list_upsert(GmDoorList* list, const GmDoor* door);

#ifdef __cplusplus
}
#endif
