/**
 * @file gm_icon.h
 * @brief The per-door icons and how to draw them.
 *
 * A clicker can open more than a garage: swing/slide gates, and RF relays for
 * shop lights, pumps and outlets are all common. Each door carries one of these
 * icons so the list is scannable at a glance. Icons are drawn from canvas
 * primitives rather than bitmap assets, so adding one is just another draw
 * function -- no image pipeline.
 */
#pragma once

#include <gui/canvas.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GmIconGarage = 0, // default
    GmIconGate,
    GmIconLight,
    GmIconCount,
} GmIconId;

/** Draw icon @p id with its top-left at (@p x, @p y). Icons are 13x13. */
void gm_icon_draw(Canvas* canvas, GmIconId id, uint8_t x, uint8_t y);

/** @return the menu label for @p id ("Garage door", ...). */
const char* gm_icon_label(GmIconId id);

/** @return @p raw clamped to a valid GmIconId. */
GmIconId gm_icon_sanitize(uint32_t raw);

#ifdef __cplusplus
}
#endif
