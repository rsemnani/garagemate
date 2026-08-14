#include "gm_door.h"
#include "gm_paths.h"

#include <furi.h>
#include <furi_hal_random.h>

void gm_door_init(GmDoor* door) {
    furi_assert(door);
    memset(door, 0, sizeof(GmDoor));

    // A 32-bit random value gives both the filename and the remote serial. The
    // id only has to be unique on this SD card, so collisions are a non-issue.
    uint32_t token = furi_hal_random_get();
    snprintf(door->id, sizeof(door->id), "door_%08lx", (unsigned long)token);

    // Serials are 28-bit for KeeLoq and 32-bit for Security+ 2.0; masking to 28
    // bits keeps one value valid for every generator we drive.
    door->serial = furi_hal_random_get() & 0x0FFFFFFFUL;
    door->counter = 0;
    door->managed = true;
    strlcpy(door->name, "New door", sizeof(door->name));
}

void gm_door_path(const GmDoor* door, char* out, size_t out_size) {
    furi_assert(door);
    snprintf(out, out_size, "%s/%s%s", GM_DOOR_DIR, door->id, GM_DOOR_EXT);
}

void gm_door_sub_export_path(const GmDoor* door, char* out, size_t out_size) {
    furi_assert(door);
    snprintf(out, out_size, "%s/%s.sub", GM_EXPORT_DIR, door->id);
}
