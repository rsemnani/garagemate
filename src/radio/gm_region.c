#include "gm_region.h"

#include <furi.h>

#define TAG "GarageMate"

/**
 * The bands the CC1101 can actually tune, matching
 * furi_hal_subghz_is_frequency_valid(). Deliberately not 0-1 GHz: there is no
 * point advertising frequencies the radio cannot reach, and staying inside the
 * hardware's range keeps the widened table honest.
 */
static const FuriHalRegionBand gm_region_bands[] = {
    {.start = 300000000, .end = 348000000, .power_limit = 12, .duty_cycle = 50},
    {.start = 387000000, .end = 464000000, .power_limit = 12, .duty_cycle = 50},
    {.start = 779000000, .end = 928000000, .power_limit = 12, .duty_cycle = 50},
};

/** Allocate a region table with @p count bands copied from @p bands. */
static FuriHalRegion*
    gm_region_alloc(const char* code, const FuriHalRegionBand* bands, size_t count) {
    size_t size = sizeof(FuriHalRegion) + count * sizeof(FuriHalRegionBand);
    FuriHalRegion* region = malloc(size);

    memset(region, 0, size);
    strlcpy(region->country_code, code, sizeof(region->country_code));
    region->bands_count = (uint16_t)count;
    memcpy(region->bands, bands, count * sizeof(FuriHalRegionBand));

    return region;
}

void gm_region_guard_init(GmRegionGuard* guard) {
    furi_assert(guard);
    guard->saved = NULL;
    guard->active = false;
}

bool gm_region_unlock(GmRegionGuard* guard) {
    furi_assert(guard);
    if(guard->active) return true;

    // Copy the current table before replacing it: furi_hal_region_set() frees
    // the table it displaces, so keeping the original pointer would leave us
    // holding freed memory.
    const FuriHalRegion* current = furi_hal_region_get();
    if(current != NULL) {
        guard->saved =
            gm_region_alloc(current->country_code, current->bands, current->bands_count);
    }

    furi_hal_region_set(gm_region_alloc("00", gm_region_bands, COUNT_OF(gm_region_bands)));
    guard->active = true;

    FURI_LOG_I(TAG, "Region widened to the full CC1101 range");
    return true;
}

void gm_region_restore(GmRegionGuard* guard) {
    furi_assert(guard);
    if(!guard->active) return;

    if(guard->saved != NULL) {
        // Hands ownership of the copy back to the HAL, which frees ours.
        furi_hal_region_set(guard->saved);
        guard->saved = NULL;
    } else {
        // The Flipper had no region provisioned to begin with. There is no way
        // to hand NULL back, so the widened table stays until the next reboot.
        FURI_LOG_W(TAG, "No original region to restore; clears on reboot");
    }

    guard->active = false;
}

const char* gm_region_current_name(void) {
    return furi_hal_region_get_name();
}
