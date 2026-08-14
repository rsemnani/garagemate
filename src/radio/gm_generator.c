#include "gm_generator.h"

#include <furi.h>
#include <lib/subghz/protocols/public_api.h>

#define TAG "GarageMate"

bool gm_generator_supports(const GmBrand* brand) {
    if(brand == NULL) return false;
    return brand->kind == GmProtoSecPlusV2 || brand->kind == GmProtoKeeLoq ||
           brand->kind == GmProtoFixed;
}

/** Build the radio preset the generators serialise into the payload. */
static void gm_preset_init(SubGhzRadioPreset* preset, uint32_t frequency) {
    preset->name = furi_string_alloc_set(GM_PRESET_SHORT);
    preset->frequency = frequency;
    preset->data = NULL;
    preset->data_size = 0;
}

static void gm_preset_reset(SubGhzRadioPreset* preset) {
    furi_string_free(preset->name);
}

/**
 * Emit a fixed-code payload by hand.
 *
 * The firmware exposes no generator for DIP-switch protocols, but they need
 * none: the payload is just an N-bit number, and the receiver stores whatever
 * it hears when you press LEARN. We derive that number from the door's serial
 * so a given door always transmits the same code.
 */
static bool gm_generator_build_fixed(
    FlipperFormat* ff,
    const GmDoor* door,
    const GmBrand* brand,
    uint32_t frequency) {
    furi_check(brand->bits > 0 && brand->bits <= 64);

    uint64_t mask = (brand->bits >= 64) ? UINT64_MAX : ((1ULL << brand->bits) - 1);
    uint64_t key = (uint64_t)door->serial & mask;

    // An all-zero or all-ones code is ignored by some receivers as noise, so
    // nudge it to a value that is unambiguously a real button press.
    if(key == 0 || key == mask) key = mask / 2;

    uint8_t key_bytes[sizeof(uint64_t)];
    for(size_t i = 0; i < sizeof(key_bytes); i++) {
        key_bytes[i] = (uint8_t)(key >> (56 - 8 * i));
    }

    uint32_t bits = brand->bits;

    bool ok = false;
    do {
        if(!flipper_format_write_header_cstr(ff, SUBGHZ_KEY_FILE_TYPE, SUBGHZ_KEY_FILE_VERSION))
            break;
        if(!flipper_format_write_uint32(ff, "Frequency", &frequency, 1)) break;
        if(!flipper_format_write_string_cstr(ff, "Preset", GM_PRESET_NAME)) break;
        if(!flipper_format_write_string_cstr(ff, "Protocol", brand->protocol)) break;
        if(!flipper_format_write_uint32(ff, "Bit", &bits, 1)) break;
        if(!flipper_format_write_hex(ff, "Key", key_bytes, sizeof(key_bytes))) break;

        // Only protocols with a configurable symbol period carry TE.
        if(brand->te > 0) {
            uint32_t te = brand->te;
            if(!flipper_format_write_uint32(ff, "TE", &te, 1)) break;
        }

        ok = true;
    } while(false);

    return ok;
}

bool gm_generator_build(
    SubGhzTransmitter* transmitter,
    FlipperFormat* ff,
    const GmDoor* door,
    const GmBrand* brand,
    uint32_t counter,
    uint32_t frequency) {
    furi_assert(ff);
    furi_assert(door);
    furi_assert(brand);

    if(brand->kind == GmProtoFixed) {
        return gm_generator_build_fixed(ff, door, brand, frequency);
    }

    if(transmitter == NULL) {
        FURI_LOG_E(TAG, "No transmitter for protocol %s", brand->protocol);
        return false;
    }

    SubGhzRadioPreset preset;
    gm_preset_init(&preset, frequency);
    void* encoder = subghz_transmitter_get_protocol_instance(transmitter);
    bool ok = false;

    switch(brand->kind) {
    case GmProtoSecPlusV2:
        // Security+ 2.0 counters are 28-bit; wrapping matches a real remote.
        ok = subghz_protocol_secplus_v2_create_data(
            encoder, ff, door->serial, brand->button, counter & 0x0FFFFFFFUL, &preset);
        break;

    case GmProtoKeeLoq:
        // KeeLoq serials are 28-bit and counters 16-bit.
        ok = subghz_protocol_keeloq_create_data(
            encoder,
            ff,
            door->serial & 0x0FFFFFFFUL,
            brand->button,
            (uint16_t)counter,
            brand->keeloq_mfg,
            &preset);
        break;

    default:
        FURI_LOG_E(TAG, "Brand %s has no generator", brand->id);
        break;
    }

    gm_preset_reset(&preset);

    if(!ok) FURI_LOG_E(TAG, "Payload generation failed for %s", brand->id);
    return ok;
}
