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

    // Left unconstrained here; gm_door_apply_brand() narrows it to whatever
    // pattern the chosen brand requires.
    door->serial = furi_hal_random_get();
    door->counter = 0;
    door->managed = true;
    strlcpy(door->name, "New door", sizeof(door->name));
}

void gm_door_apply_brand(GmDoor* door, const GmBrand* brand) {
    furi_assert(door);
    furi_assert(brand);

    strlcpy(door->brand_id, brand->id, sizeof(door->brand_id));
    door->frequency = brand->freqs[0];
    door->button = brand->button;
    door->all_bands = brand->multiband;

    if(brand->serial_mask != 0) door->serial &= brand->serial_mask;
    door->counter = brand->counter_start;
}

void gm_door_path(const GmDoor* door, char* out, size_t out_size) {
    furi_assert(door);
    snprintf(out, out_size, "%s/%s%s", GM_DOOR_DIR, door->id, GM_DOOR_EXT);
}

void gm_door_sub_export_path(const GmDoor* door, char* out, size_t out_size) {
    furi_assert(door);
    snprintf(out, out_size, "%s/%s.sub", GM_EXPORT_DIR, door->id);
}
