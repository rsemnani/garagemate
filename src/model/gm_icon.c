#include "gm_icon.h"

/*
 * All three icons fit a 13x13 box. Coordinates are relative to the (x, y)
 * top-left the caller passes in.
 */

static void gm_icon_draw_garage(Canvas* canvas, uint8_t x, uint8_t y) {
    // Building outline with a peaked roof.
    canvas_draw_line(canvas, x, y + 4, x + 6, y); // left roof slope
    canvas_draw_line(canvas, x + 6, y, x + 12, y + 4); // right roof slope
    canvas_draw_line(canvas, x, y + 4, x, y + 12); // left wall
    canvas_draw_line(canvas, x + 12, y + 4, x + 12, y + 12); // right wall
    canvas_draw_line(canvas, x, y + 12, x + 12, y + 12); // ground
    // Panelled door: a frame with two horizontal slats.
    canvas_draw_frame(canvas, x + 2, y + 6, 9, 7);
    canvas_draw_line(canvas, x + 2, y + 8, x + 10, y + 8);
    canvas_draw_line(canvas, x + 2, y + 10, x + 10, y + 10);
}

static void gm_icon_draw_gate(Canvas* canvas, uint8_t x, uint8_t y) {
    // Two posts and a cross-braced panel between them: a driveway gate.
    canvas_draw_line(canvas, x, y + 1, x, y + 12); // left post
    canvas_draw_line(canvas, x + 12, y + 1, x + 12, y + 12); // right post
    canvas_draw_frame(canvas, x + 2, y + 3, 9, 8); // gate panel
    canvas_draw_line(canvas, x + 2, y + 7, x + 10, y + 7); // mid rail
    canvas_draw_line(canvas, x + 2, y + 10, x + 10, y + 3); // diagonal brace
}

static void gm_icon_draw_light(Canvas* canvas, uint8_t x, uint8_t y) {
    // Bulb envelope with a screw base and a couple of rays.
    canvas_draw_circle(canvas, x + 6, y + 5, 4);
    canvas_draw_line(canvas, x + 4, y + 9, x + 8, y + 9); // base top
    canvas_draw_line(canvas, x + 4, y + 11, x + 8, y + 11); // base bottom
    canvas_draw_line(canvas, x + 5, y + 12, x + 7, y + 12); // contact
    canvas_draw_line(canvas, x + 6, y - 1, x + 6, y); // top ray
    canvas_draw_line(canvas, x, y + 4, x + 1, y + 4); // left ray
    canvas_draw_line(canvas, x + 11, y + 4, x + 12, y + 4); // right ray
}

void gm_icon_draw(Canvas* canvas, GmIconId id, uint8_t x, uint8_t y) {
    switch(id) {
    case GmIconGate:
        gm_icon_draw_gate(canvas, x, y);
        break;
    case GmIconLight:
        gm_icon_draw_light(canvas, x, y);
        break;
    case GmIconGarage:
    default:
        gm_icon_draw_garage(canvas, x, y);
        break;
    }
}

const char* gm_icon_label(GmIconId id) {
    switch(id) {
    case GmIconGate:
        return "Gate";
    case GmIconLight:
        return "Light / other";
    case GmIconGarage:
    default:
        return "Garage door";
    }
}

GmIconId gm_icon_sanitize(uint32_t raw) {
    return (raw < GmIconCount) ? (GmIconId)raw : GmIconGarage;
}
