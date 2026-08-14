/**
 * @file gm_generator.h
 * @brief Turning a saved door into a transmittable Sub-GHz payload.
 *
 * Rolling-code remotes must send a different code every press, so GarageMate
 * never stores a finished payload for them. It stores the ingredients (serial,
 * button, counter) and re-derives the payload immediately before each
 * transmission. That keeps the counter authoritative in one place -- the .door
 * file -- and makes a door survivable across copies and hand edits.
 */
#pragma once

#include "../catalog/gm_brands.h"
#include "../model/gm_door.h"

#include <flipper_format/flipper_format.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Preset every brand in the catalogue transmits with, in the two spellings the
 * firmware uses.
 *
 * A SubGhzRadioPreset carries the short name; the serialiser expands it to the
 * long name when writing a file. Passing the long name into the struct makes
 * the serialiser fall through to "custom preset" and emit an unusable file.
 */
#define GM_PRESET_NAME  "FuriHalSubGhzPresetOok650Async"
#define GM_PRESET_SHORT "AM650"

/**
 * Build a complete Sub-GHz key payload for @p door at @p counter.
 *
 * @param transmitter Transmitter allocated for the brand's protocol. Required
 *                    for generated rolling codes; ignored for fixed codes.
 * @param ff          Empty FlipperFormat that receives the payload.
 * @param door        Door supplying the serial.
 * @param brand       Brand describing the protocol to emit.
 * @param counter     Rolling counter value to encode.
 * @param frequency   Band this payload is for. Passed explicitly rather than
 *                    read from @p door, because a multi-band door emits the
 *                    same counter on several frequencies.
 * @return true when @p ff holds a valid payload.
 */
bool gm_generator_build(
    SubGhzTransmitter* transmitter,
    FlipperFormat* ff,
    const GmDoor* door,
    const GmBrand* brand,
    uint32_t counter,
    uint32_t frequency);

/** @return true when GarageMate can synthesise a remote for @p brand. */
bool gm_generator_supports(const GmBrand* brand);

#ifdef __cplusplus
}
#endif
